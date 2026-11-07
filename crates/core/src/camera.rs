//! Webcam capture through Media Foundation in the camera's native format.

use crate::resample::Source;
use anyhow::{bail, Context, Result};
use serde::Serialize;
use windows::core::{Interface, GUID, PWSTR};
use windows::Win32::Media::MediaFoundation::*;
use windows::Win32::System::Com::CoTaskMemFree;

#[derive(Clone, Debug, Serialize)]
pub struct CameraInfo {
    /// Stable identifier (symbolic link), stored in settings.
    pub id: String,
    pub name: String,
}

#[derive(Clone, Copy, Debug, Serialize)]
pub struct Mode {
    pub width: u32,
    pub height: u32,
    pub fps: f32,
}

pub fn startup() -> Result<()> {
    unsafe { MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET) }.context("MFStartup")
}

fn devices() -> Result<Vec<IMFActivate>> {
    unsafe {
        let mut attrs = None;
        MFCreateAttributes(&mut attrs, 1)?;
        let attrs = attrs.context("attributes")?;
        attrs.SetGUID(&MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID)?;
        let mut list = std::ptr::null_mut();
        let mut count = 0;
        MFEnumDeviceSources(&attrs, &mut list, &mut count)?;
        let items = std::slice::from_raw_parts(list, count as usize).iter().flatten().cloned().collect();
        CoTaskMemFree(Some(list as _));
        Ok(items)
    }
}

fn string(activate: &IMFActivate, key: &GUID) -> Result<String> {
    unsafe {
        let mut ptr = PWSTR::null();
        let mut len = 0;
        activate.GetAllocatedString(key, &mut ptr, &mut len)?;
        let s = ptr.to_string()?;
        CoTaskMemFree(Some(ptr.0 as _));
        Ok(s)
    }
}

pub fn list() -> Result<Vec<CameraInfo>> {
    devices()?
        .iter()
        .map(|d| {
            Ok(CameraInfo {
                id: string(d, &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK)?,
                name: string(d, &MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME)?,
            })
        })
        .collect()
}

pub struct Camera {
    reader: IMFSourceReader,
    source: IMFMediaSource,
    pub mode: Mode,
    /// Row pitch for buffers without `IMF2DBuffer`; negative = bottom-up.
    stride: i32,
}

const STREAM: u32 = MF_SOURCE_READER_FIRST_VIDEO_STREAM.0 as u32;

/// Prefers ~720p–1080p at 30 fps; 16:9 wins ties so nothing has to be trimmed.
fn score(m: &Mode) -> f32 {
    let size = match m.height {
        720 => 100.0,
        1080 => 90.0,
        h if h > 1080 => 40.0,
        h => h as f32 / 10.0,
    };
    let fps = m.fps.min(30.0) * 2.0;
    let wide = if m.width * 9 == m.height * 16 { 20.0 } else { 0.0 };
    size + fps + wide
}

impl Camera {
    pub fn open(id: &str) -> Result<Self> {
        let activate = devices()?
            .into_iter()
            .find(|d| string(d, &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK).is_ok_and(|s| s == id))
            .context("камера не найдена")?;
        unsafe {
            let source: IMFMediaSource = activate.ActivateObject()?;
            let mut attrs = None;
            MFCreateAttributes(&mut attrs, 2)?;
            let attrs = attrs.context("attributes")?;
            attrs.SetUINT32(&MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, 1)?;
            attrs.SetUINT32(&MF_READWRITE_DISABLE_CONVERTERS, 0)?;
            let reader = MFCreateSourceReaderFromMediaSource(&source, &attrs)?;

            let mut best: Option<(IMFMediaType, Mode)> = None;
            for i in 0.. {
                let Ok(t) = reader.GetNativeMediaType(STREAM, i) else { break };
                let size = t.GetUINT64(&MF_MT_FRAME_SIZE).unwrap_or(0);
                let rate = t.GetUINT64(&MF_MT_FRAME_RATE).unwrap_or(0);
                let (num, den) = ((rate >> 32) as u32, rate as u32);
                let mode = Mode {
                    width: (size >> 32) as u32,
                    height: size as u32,
                    fps: if den == 0 { 0.0 } else { num as f32 / den as f32 },
                };
                if mode.width == 0 || mode.height == 0 {
                    continue;
                }
                if best.as_ref().map_or(true, |(_, b)| score(&mode) > score(b)) {
                    best = Some((t, mode));
                }
            }
            let (native, mode) = best.context("камера не сообщает форматы")?;
            reader.SetCurrentMediaType(STREAM, None, &native)?;

            // Same size, converted to RGB32 by the reader.
            let out = MFCreateMediaType()?;
            out.SetGUID(&MF_MT_MAJOR_TYPE, &MFMediaType_Video)?;
            out.SetGUID(&MF_MT_SUBTYPE, &MFVideoFormat_RGB32)?;
            out.SetUINT64(&MF_MT_FRAME_SIZE, ((mode.width as u64) << 32) | mode.height as u64)?;
            reader.SetCurrentMediaType(STREAM, None, &out).context("камера не отдаёт RGB")?;
            reader.SetStreamSelection(MF_SOURCE_READER_ALL_STREAMS.0 as u32, false)?;
            reader.SetStreamSelection(STREAM, true)?;
            let stride = reader
                .GetCurrentMediaType(STREAM)?
                .GetUINT32(&MF_MT_DEFAULT_STRIDE)
                .map(|s| s as i32)
                .unwrap_or((mode.width * 4) as i32);
            Ok(Self { reader, source, mode, stride })
        }
    }

    /// Blocks until the next frame and passes it to `f`.
    pub fn read<R>(&mut self, f: impl FnOnce(&Source) -> R) -> Result<Option<R>> {
        unsafe {
            let mut flags = 0u32;
            let mut sample = None;
            self.reader.ReadSample(STREAM, 0, None, Some(&mut flags), None, Some(&mut sample))?;
            if flags & (MF_SOURCE_READERF_ENDOFSTREAM.0 as u32 | MF_SOURCE_READERF_ERROR.0 as u32) != 0 {
                bail!("камера отключилась");
            }
            let Some(sample) = sample else { return Ok(None) };
            let buffer = sample.GetBufferByIndex(0)?;
            let (w, h) = (self.mode.width, self.mode.height);

            if let Ok(b2d) = buffer.cast::<IMF2DBuffer>() {
                let mut line0 = std::ptr::null_mut();
                let mut pitch = 0i32;
                b2d.Lock2D(&mut line0, &mut pitch)?;
                // Cover the whole allocation whichever way the rows run.
                let span = pitch.unsigned_abs() as usize * h as usize;
                let (start, origin) = if pitch < 0 {
                    (line0.offset(pitch as isize * (h as isize - 1)), span - pitch.unsigned_abs() as usize)
                } else {
                    (line0, 0)
                };
                let data = std::slice::from_raw_parts(start, span);
                let r = f(&Source { data, width: w, height: h, stride: pitch as isize, origin });
                b2d.Unlock2D()?;
                return Ok(Some(r));
            }

            let mut ptr = std::ptr::null_mut();
            let mut len = 0;
            buffer.Lock(&mut ptr, None, Some(&mut len))?;
            let pitch = self.stride.unsigned_abs() as usize;
            let r = if pitch < (w * 4) as usize || (len as usize) < pitch * h as usize {
                None
            } else {
                let data = std::slice::from_raw_parts(ptr, len as usize);
                let origin = if self.stride < 0 { pitch * (h as usize - 1) } else { 0 };
                Some(f(&Source { data, width: w, height: h, stride: self.stride as isize, origin }))
            };
            buffer.Unlock()?;
            Ok(r)
        }
    }
}

impl Drop for Camera {
    fn drop(&mut self) {
        unsafe {
            let _ = self.source.Shutdown();
        }
    }
}
