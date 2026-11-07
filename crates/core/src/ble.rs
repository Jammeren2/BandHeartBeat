//! Heart-rate watch connection with continuous background auto-search.
//!
//! Any device exposing the standard Heart Rate Service works: Huawei bands in
//! "HR data broadcast" mode, Xiaomi bands during a workout, chest straps.

use crate::heart::{parse_measurement, Heart, Link};
use anyhow::{bail, Context, Result};
use btleplug::api::{Central, CharPropFlags, Manager as _, Peripheral as _, ScanFilter};
use btleplug::platform::{Adapter, Manager, Peripheral};
use futures_util::StreamExt;
use std::time::Duration;
use tokio::sync::watch;
use tokio::time::{sleep, timeout};

const HR_SERVICE: uuid::Uuid = uuid::uuid!("0000180d-0000-1000-8000-00805f9b34fb");
const HR_MEASUREMENT: uuid::Uuid = uuid::uuid!("00002a37-0000-1000-8000-00805f9b34fb");

/// Devices that may expose heart rate without advertising the service UUID.
const NAME_HINTS: &[&str] = &["huawei", "honor band", "mi band", "smart band", "xiaomi", "amazfit"];

const SCAN_TIME: Duration = Duration::from_secs(6);
const STEP_TIMEOUT: Duration = Duration::from_secs(10);
const SILENCE_TIMEOUT: Duration = Duration::from_secs(15);
const RETRY_DELAY: Duration = Duration::from_secs(3);

#[derive(Clone)]
pub struct BleConfig {
    pub enabled: bool,
    pub preferred: Option<String>,
}

pub struct Ble {
    config: watch::Sender<BleConfig>,
}

impl Ble {
    /// Starts the supervisor on `rt`. `on_connected` receives the address of a
    /// watch that delivered heart rate, so it can be remembered.
    pub fn spawn(
        rt: &tokio::runtime::Handle,
        heart: Heart,
        config: BleConfig,
        on_connected: impl Fn(String) + Send + Sync + 'static,
    ) -> Self {
        let (tx, rx) = watch::channel(config);
        rt.spawn(supervise(heart, rx, on_connected));
        Self { config: tx }
    }

    pub fn configure(&self, config: BleConfig) {
        self.config.send_replace(config);
    }

    /// Drops the current attempt and starts a fresh search right away.
    pub fn retry(&self) {
        self.config.send_modify(|_| {});
    }
}

async fn supervise(heart: Heart, mut rx: watch::Receiver<BleConfig>, on_connected: impl Fn(String)) {
    let mut adapter: Option<Adapter> = None;
    loop {
        let config = rx.borrow_and_update().clone();
        if !config.enabled {
            heart.set_link(Link::Off);
            let _ = rx.changed().await;
            continue;
        }
        if adapter.is_none() {
            match first_adapter().await {
                Ok(a) => adapter = Some(a),
                Err(e) => heart.set_link(Link::Error { message: format!("{e:#}") }),
            }
        }
        if let Some(adapter) = &adapter {
            tokio::select! {
                result = session(adapter, &heart, config.preferred.as_deref(), &on_connected) => {
                    if let Err(e) = result {
                        log::info!("BLE session ended: {e:#}");
                        heart.set_link(Link::Scanning);
                    }
                }
                _ = rx.changed() => continue,
            }
        }
        tokio::select! {
            _ = sleep(RETRY_DELAY) => {}
            _ = rx.changed() => {}
        }
    }
}

async fn first_adapter() -> Result<Adapter> {
    let manager = Manager::new().await.context("Bluetooth недоступен")?;
    manager
        .adapters()
        .await?
        .into_iter()
        .next()
        .context("Bluetooth-адаптер не найден")
}

async fn session(
    adapter: &Adapter,
    heart: &Heart,
    preferred: Option<&str>,
    on_connected: &impl Fn(String),
) -> Result<()> {
    heart.set_link(Link::Scanning);
    for (peripheral, name) in candidates(adapter, preferred).await? {
        heart.set_link(Link::Connecting { name: name.clone() });
        match stream(&peripheral, heart, &name, on_connected).await {
            Ok(()) => return Ok(()),
            Err(e) => log::info!("{name}: {e:#}"),
        }
        let _ = timeout(Duration::from_secs(2), peripheral.disconnect()).await;
    }
    bail!("подходящие часы не найдены")
}

/// Scans and returns plausible heart-rate devices, best match first.
async fn candidates(adapter: &Adapter, preferred: Option<&str>) -> Result<Vec<(Peripheral, String)>> {
    adapter.start_scan(ScanFilter::default()).await?;
    sleep(SCAN_TIME).await;
    let mut found = Vec::new();
    for p in adapter.peripherals().await? {
        let Some(props) = p.properties().await? else { continue };
        let address = props.address.to_string();
        let name = props.local_name.clone().unwrap_or_else(|| address.clone());
        let lower = name.to_lowercase();
        let rank = if preferred.is_some_and(|a| a.eq_ignore_ascii_case(&address)) {
            0
        } else if props.services.contains(&HR_SERVICE) {
            1
        } else if NAME_HINTS.iter().any(|h| lower.contains(h)) {
            2
        } else {
            continue;
        };
        found.push((rank, p, name));
    }
    let _ = adapter.stop_scan().await;
    found.sort_by_key(|(rank, ..)| *rank);
    Ok(found.into_iter().map(|(_, p, n)| (p, n)).collect())
}

async fn stream(peripheral: &Peripheral, heart: &Heart, name: &str, on_connected: &impl Fn(String)) -> Result<()> {
    if !peripheral.is_connected().await.unwrap_or(false) {
        timeout(STEP_TIMEOUT, peripheral.connect()).await.context("таймаут подключения")??;
    }
    timeout(STEP_TIMEOUT, peripheral.discover_services()).await.context("таймаут чтения сервисов")??;
    let characteristic = peripheral
        .characteristics()
        .into_iter()
        .find(|c| c.uuid == HR_MEASUREMENT && c.properties.contains(CharPropFlags::NOTIFY))
        .context("нет характеристики пульса")?;
    let mut notifications = peripheral.notifications().await?;
    peripheral.subscribe(&characteristic).await?;

    let mut announced = false;
    loop {
        let Ok(Some(n)) = timeout(SILENCE_TIMEOUT, notifications.next()).await else {
            let _ = peripheral.unsubscribe(&characteristic).await;
            bail!("часы перестали присылать пульс");
        };
        if n.uuid != HR_MEASUREMENT {
            continue;
        }
        if let Some(bpm) = parse_measurement(&n.value) {
            heart.push(bpm);
            if !announced {
                announced = true;
                heart.set_link(Link::Connected { name: name.to_string() });
                on_connected(peripheral.address().to_string());
            }
        }
    }
}
