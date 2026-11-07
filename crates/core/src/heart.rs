use serde::Serialize;
use std::collections::VecDeque;
use std::sync::{Arc, Mutex};
use std::time::{Duration, Instant};

/// A reading older than this is shown as "no data".
pub const STALE_AFTER: Duration = Duration::from_secs(6);
const HISTORY: Duration = Duration::from_secs(60);

#[derive(Clone, Debug, PartialEq, Serialize)]
#[serde(tag = "kind", rename_all = "lowercase")]
pub enum Link {
    Off,
    Scanning,
    Connecting { name: String },
    Connected { name: String },
    Error { message: String },
}

#[derive(Default)]
struct Inner {
    link: Option<Link>,
    last: Option<(u16, Instant)>,
    history: VecDeque<(Instant, u16)>,
}

/// Heart-rate state shared between the BLE task, the renderer and the UI.
#[derive(Clone, Default)]
pub struct Heart(Arc<Mutex<Inner>>);

impl Heart {
    pub fn set_link(&self, link: Link) {
        self.0.lock().unwrap().link = Some(link);
    }

    pub fn link(&self) -> Link {
        self.0.lock().unwrap().link.clone().unwrap_or(Link::Off)
    }

    pub fn push(&self, bpm: u16) {
        let now = Instant::now();
        let mut inner = self.0.lock().unwrap();
        inner.last = Some((bpm, now));
        inner.history.push_back((now, bpm));
        while inner.history.front().is_some_and(|(t, _)| now - *t > HISTORY) {
            inner.history.pop_front();
        }
    }

    /// Current BPM, or `None` when the last reading is stale.
    pub fn bpm(&self) -> Option<u16> {
        let inner = self.0.lock().unwrap();
        inner.last.filter(|(_, t)| t.elapsed() < STALE_AFTER).map(|(b, _)| b)
    }

    /// Readings of the last minute, oldest first.
    pub fn history(&self) -> Vec<u16> {
        self.0.lock().unwrap().history.iter().map(|(_, b)| *b).collect()
    }
}

/// Parses a Heart Rate Measurement (0x2A37) notification.
pub fn parse_measurement(data: &[u8]) -> Option<u16> {
    let flags = *data.first()?;
    let bpm = if flags & 1 == 0 {
        *data.get(1)? as u16
    } else {
        u16::from_le_bytes([*data.get(1)?, *data.get(2)?])
    };
    (bpm > 0).then_some(bpm)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parses_u8_and_u16_formats() {
        assert_eq!(parse_measurement(&[0x00, 72]), Some(72));
        assert_eq!(parse_measurement(&[0x01, 0x2c, 0x01]), Some(300));
        assert_eq!(parse_measurement(&[0x10, 80, 0x00, 0x04]), Some(80));
        assert_eq!(parse_measurement(&[0x00, 0]), None);
        assert_eq!(parse_measurement(&[0x01, 5]), None);
    }
}
