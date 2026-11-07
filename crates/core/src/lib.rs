//! BandHeartBeat core: heart-rate source, camera capture, head tracking,
//! overlay rendering and the shared frame consumed by the virtual camera DLL.

pub mod ble;
pub mod camera;
pub mod frame;
pub mod girls;
pub mod heart;
pub mod overlay;
pub mod pipeline;
pub mod resample;
pub mod settings;
pub mod tracking;
pub mod vcam;

/// Size of the frame published to the virtual camera. Discord tiles are 16:9,
/// so the output is always 16:9 and the camera image is fitted into it.
pub const OUT_W: u32 = 1280;
pub const OUT_H: u32 = 720;
