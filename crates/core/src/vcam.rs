//! Per-user (HKCU) registration of the DirectShow virtual camera DLL.
//! No administrator rights are needed and HKLM is never touched.

use anyhow::{Context, Result};
use serde::Serialize;
use std::path::Path;
use windows::core::{HSTRING, PCWSTR};
use windows::Win32::Foundation::ERROR_FILE_NOT_FOUND;
use windows::Win32::System::Registry::*;

/// Must match `CLSID_BandHeartBeatCamera` in `vcam/camera.cpp`.
pub const CLSID: &str = "{8A3E5B1C-2F47-4D9B-A6C1-5E0B7D9F3A21}";
pub const FRIENDLY_NAME: &str = "BandHeartBeat Camera";
const VIDEO_INPUT_CATEGORY: &str = "{860BB310-5D01-11D0-BD3B-00A0C911CE86}";

fn class_key() -> String {
    format!("Software\\Classes\\CLSID\\{CLSID}")
}

fn instance_key() -> String {
    format!("Software\\Classes\\CLSID\\{VIDEO_INPUT_CATEGORY}\\Instance\\{CLSID}")
}

#[derive(Clone, Debug, PartialEq, Serialize)]
#[serde(tag = "kind", rename_all = "lowercase")]
pub enum Status {
    Installed,
    /// Registered, but for a DLL at another path (old copy of the app).
    Outdated { path: String },
    Missing,
}

fn set(key: &str, name: Option<&str>, value: &str) -> Result<()> {
    unsafe {
        let mut h = HKEY::default();
        RegCreateKeyExW(HKEY_CURRENT_USER, &HSTRING::from(key), None, None, REG_OPTION_NON_VOLATILE, KEY_WRITE, None, &mut h, None)
            .ok()
            .with_context(|| format!("создание ключа {key}"))?;
        let data: Vec<u8> = value.encode_utf16().chain([0]).flat_map(u16::to_le_bytes).collect();
        let name = name.map(HSTRING::from);
        let r = RegSetValueExW(h, name.as_ref().map_or(PCWSTR::null(), |n| PCWSTR(n.as_ptr())), None, REG_SZ, Some(&data));
        let _ = RegCloseKey(h);
        r.ok().with_context(|| format!("запись {key}"))
    }
}

fn get(key: &str, name: Option<&str>) -> Option<String> {
    unsafe {
        let mut buf = [0u16; 1024];
        let mut len = (buf.len() * 2) as u32;
        let name = name.map(HSTRING::from);
        RegGetValueW(
            HKEY_CURRENT_USER,
            &HSTRING::from(key),
            name.as_ref().map_or(PCWSTR::null(), |n| PCWSTR(n.as_ptr())),
            RRF_RT_REG_SZ,
            None,
            Some(buf.as_mut_ptr().cast()),
            Some(&mut len),
        )
        .ok()
        .ok()?;
        let chars = (len as usize / 2).saturating_sub(1);
        Some(String::from_utf16_lossy(&buf[..chars]))
    }
}

/// COM loads DLLs with plain paths; drop the `\\?\` prefix canonicalisation adds.
fn plain(dll: &Path) -> String {
    let s = dll.to_string_lossy();
    s.strip_prefix(r"\\?\").unwrap_or(&s).to_string()
}

#[cfg(test)]
mod tests {
    #[test]
    fn strips_extended_prefix() {
        assert_eq!(super::plain(r"\\?\D:\a\b.dll".as_ref()), r"D:\a\b.dll");
        assert_eq!(super::plain(r"D:\a\b.dll".as_ref()), r"D:\a\b.dll");
    }
}

pub fn status(dll: &Path) -> Status {
    let dll = plain(dll);
    let registered = get(&format!("{}\\InprocServer32", class_key()), None);
    let listed = get(&instance_key(), Some("CLSID")).is_some();
    match registered {
        Some(path) if listed && path.eq_ignore_ascii_case(&dll) => Status::Installed,
        Some(path) if listed => Status::Outdated { path },
        _ => Status::Missing,
    }
}

pub fn install(dll: &Path) -> Result<()> {
    anyhow::ensure!(dll.exists(), "не найден {}", dll.display());
    let dll = plain(dll);
    let class = class_key();
    set(&class, None, FRIENDLY_NAME)?;
    set(&format!("{class}\\InprocServer32"), None, &dll)?;
    set(&format!("{class}\\InprocServer32"), Some("ThreadingModel"), "Both")?;
    let instance = instance_key();
    set(&instance, Some("FriendlyName"), FRIENDLY_NAME)?;
    set(&instance, Some("CLSID"), CLSID)?;
    Ok(())
}

pub fn remove() -> Result<()> {
    for key in [instance_key(), class_key()] {
        let r = unsafe { RegDeleteTreeW(HKEY_CURRENT_USER, &HSTRING::from(key.as_str())) };
        if r.is_err() && r != ERROR_FILE_NOT_FOUND {
            r.ok().with_context(|| format!("удаление {key}"))?;
        }
    }
    Ok(())
}
