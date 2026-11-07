use serde::{Deserialize, Serialize};
use std::path::PathBuf;

#[derive(Clone, Copy, Debug, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "lowercase")]
pub enum OverlayStyle {
    Capsule,
    Heart,
    Ecg,
    Ring,
    Minimal,
    Card,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "lowercase")]
pub enum Placement {
    /// Follow the detected head, falling back to `anchor` when no face is seen.
    Track,
    /// Stay at `anchor`.
    Fixed,
}

/// Where the badge sits relative to the tracked face.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum FacePoint {
    /// Beside the head, switching sides near the frame edge.
    Auto,
    Above,
    Left,
    Right,
    Chin,
    LeftShoulder,
    RightShoulder,
}

impl FacePoint {
    pub const ALL: [FacePoint; 6] = [
        FacePoint::Above,
        FacePoint::Left,
        FacePoint::Right,
        FacePoint::Chin,
        FacePoint::LeftShoulder,
        FacePoint::RightShoulder,
    ];

    /// Position in face-box units: (0,0) is the box's top-left, (1,1) its bottom-right.
    pub fn position(self) -> Option<(f32, f32)> {
        Some(match self {
            FacePoint::Auto => return None,
            FacePoint::Above => (0.5, -0.35),
            FacePoint::Left => (-0.45, 0.35),
            FacePoint::Right => (1.45, 0.35),
            FacePoint::Chin => (0.5, 1.3),
            FacePoint::LeftShoulder => (-0.6, 1.6),
            FacePoint::RightShoulder => (1.6, 1.6),
        })
    }
}

/// How a camera whose aspect ratio is not 16:9 fills the output.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "lowercase")]
pub enum Fit {
    /// Whole image visible, bars on the sides.
    Contain,
    /// Fill the frame, trimming edges.
    Cover,
}

#[derive(Clone, Debug, PartialEq, Serialize, Deserialize)]
#[serde(default)]
pub struct Settings {
    /// Symbolic link of the chosen camera; `None` = no camera running.
    pub camera: Option<String>,
    pub mirror: bool,
    pub fit: Fit,
    pub style: OverlayStyle,
    pub placement: Placement,
    /// Overlay centre in the fixed mode, normalised 0..1.
    pub anchor: (f32, f32),
    /// Overlay scale, 0.5..2.0.
    pub size: f32,
    /// Tracking smoothing, 0 (snappy) ..1 (very smooth).
    pub smoothing: f32,
    pub pulse_animation: bool,
    /// Draw the plate behind capsule, ECG and card badges.
    pub badge_background: bool,
    /// Face point the badge follows in the tracking mode.
    pub face_point: FacePoint,
    /// Badge offset from `face_point`, in face widths.
    pub point_offset: (f32, f32),
    pub auto_scan: bool,
    /// Address of the last watch that delivered heart rate; tried first.
    pub last_device: Option<String>,
}

impl Default for Settings {
    fn default() -> Self {
        Self {
            camera: None,
            mirror: false,
            fit: Fit::Contain,
            style: OverlayStyle::Capsule,
            placement: Placement::Track,
            anchor: (0.82, 0.18),
            size: 1.0,
            smoothing: 0.6,
            pulse_animation: true,
            badge_background: true,
            face_point: FacePoint::Auto,
            point_offset: (0.0, 0.0),
            auto_scan: true,
            last_device: None,
        }
    }
}

impl Settings {
    pub fn path() -> PathBuf {
        let base = std::env::var_os("APPDATA").map(PathBuf::from).unwrap_or_else(|| ".".into());
        base.join("BandHeartBeat").join("settings.json")
    }

    /// Missing or unreadable settings fall back to defaults.
    pub fn load() -> Self {
        std::fs::read(Self::path())
            .ok()
            .and_then(|bytes| serde_json::from_slice(&bytes).ok())
            .map(Self::clamped)
            .unwrap_or_default()
    }

    pub fn save(&self) -> anyhow::Result<()> {
        let path = Self::path();
        if let Some(dir) = path.parent() {
            std::fs::create_dir_all(dir)?;
        }
        let tmp = path.with_extension("json.tmp");
        std::fs::write(&tmp, serde_json::to_vec_pretty(self)?)?;
        std::fs::rename(tmp, path)?;
        Ok(())
    }

    pub fn clamped(mut self) -> Self {
        self.size = self.size.clamp(0.5, 2.0);
        self.smoothing = self.smoothing.clamp(0.0, 1.0);
        self.anchor = (self.anchor.0.clamp(0.0, 1.0), self.anchor.1.clamp(0.0, 1.0));
        self.point_offset = (self.point_offset.0.clamp(-3.0, 3.0), self.point_offset.1.clamp(-3.0, 3.0));
        self
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn partial_json_keeps_defaults() {
        let s: Settings = serde_json::from_str(r#"{"style":"ring","size":9}"#).unwrap();
        let s = s.clamped();
        assert_eq!(s.style, OverlayStyle::Ring);
        assert_eq!(s.size, 2.0);
        assert!(s.auto_scan);
    }
}
