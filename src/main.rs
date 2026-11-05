use anyhow::{Context, Result};
use btleplug::api::{
    Central, Characteristic, Manager as _, Peripheral as _, ScanFilter, ValueNotification,
};
use btleplug::platform::Manager;
use futures_util::StreamExt;
use std::time::Duration;

const HEART_RATE_SERVICE: uuid::Uuid = uuid::uuid!("0000180d-0000-1000-8000-00805f9b34fb");
const HEART_RATE_MEASUREMENT: uuid::Uuid = uuid::uuid!("00002a37-0000-1000-8000-00805f9b34fb");

#[tokio::main]
async fn main() -> Result<()> {
    println!("BandHeartBeat BLE heart-rate proof of concept");
    println!(
        "Enable HR Data Broadcasts on the band (Settings > HR Data Broadcasts) before scanning."
    );
    let manager = Manager::new()
        .await
        .context("initialise Bluetooth manager")?;
    let adapters = manager
        .adapters()
        .await
        .context("enumerate Bluetooth adapters")?;
    let adapter = adapters
        .into_iter()
        .next()
        .context("no Bluetooth adapter found")?;
    println!(
        "Scanning on {} for Heart Rate Service (0x180D)...",
        adapter.adapter_info().await?
    );
    adapter
        .start_scan(ScanFilter {
            services: vec![HEART_RATE_SERVICE],
        })
        .await?;
    tokio::time::sleep(Duration::from_secs(8)).await;
    let mut found = None;
    for p in adapter.peripherals().await? {
        if let Some(props) = p.properties().await? {
            if props.services.contains(&HEART_RATE_SERVICE) {
                found = Some(p);
                break;
            }
        }
    }
    let peripheral = found
        .context("no HR device found; verify HR Data Broadcasts is enabled and Bluetooth is on")?;
    println!("Connecting to {:?}...", peripheral.id());
    peripheral.connect().await.context("connect to HR device")?;
    peripheral.discover_services().await?;
    let characteristic: Characteristic = peripheral
        .characteristics()
        .into_iter()
        .find(|c| c.uuid == HEART_RATE_MEASUREMENT)
        .context("device lacks Heart Rate Measurement characteristic (0x2A37)")?;
    peripheral
        .subscribe(&characteristic)
        .await
        .context("subscribe to HR notifications")?;
    println!("Subscribed; waiting for BPM notifications (Ctrl+C to stop)...");
    let mut notifications = peripheral.notifications().await?;
    while let Some(ValueNotification { value, .. }) = notifications.next().await {
        if let Some(bpm) = parse_bpm(&value) {
            println!("Heart rate: {bpm} BPM");
        } else {
            eprintln!("Ignoring malformed measurement ({})", value.len());
        }
    }
    Ok(())
}

fn parse_bpm(data: &[u8]) -> Option<u16> {
    let flags = *data.first()?;
    if flags & 1 == 0 {
        Some(*data.get(1)? as u16)
    } else {
        Some(u16::from_le_bytes([*data.get(1)?, *data.get(2)?]))
    }
}
