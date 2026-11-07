//! Shared-memory frame read by the virtual camera DLL (see `vcam/shared_frame.h`).
//!
//! Layout: a 64-byte header followed by BGRA pixels, top-down. The writer uses
//! a sequence lock: `seq` is odd while pixels are being written.

use anyhow::{Context, Result};
use std::sync::atomic::{AtomicU64, Ordering};
use windows::core::w;
use windows::Win32::Foundation::{CloseHandle, HANDLE, INVALID_HANDLE_VALUE};
use windows::Win32::System::Memory::{
    CreateFileMappingW, MapViewOfFile, UnmapViewOfFile, FILE_MAP_ALL_ACCESS, MEMORY_MAPPED_VIEW_ADDRESS, PAGE_READWRITE,
};

pub const MAGIC: u32 = u32::from_le_bytes(*b"BHB2");
pub const HEADER_SIZE: usize = 64;

#[repr(C)]
struct Header {
    magic: u32,
    width: u32,
    height: u32,
    stride: u32,
    seq: AtomicU64,
    /// `GetTickCount64` of the last complete frame; lets the DLL notice a dead app.
    tick: AtomicU64,
}

pub struct FrameWriter {
    mapping: HANDLE,
    view: MEMORY_MAPPED_VIEW_ADDRESS,
    width: u32,
    height: u32,
}

// The mapping is only touched through `&mut self`.
unsafe impl Send for FrameWriter {}

impl FrameWriter {
    pub fn create(width: u32, height: u32) -> Result<Self> {
        let size = HEADER_SIZE + (width * height * 4) as usize;
        unsafe {
            let mapping = CreateFileMappingW(
                INVALID_HANDLE_VALUE,
                None,
                PAGE_READWRITE,
                0,
                size as u32,
                w!("Local\\BandHeartBeat.Frame.v2"),
            )
            .context("CreateFileMapping")?;
            let view = MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, size);
            if view.Value.is_null() {
                let _ = CloseHandle(mapping);
                anyhow::bail!("MapViewOfFile failed");
            }
            let header = &mut *(view.Value as *mut Header);
            header.magic = MAGIC;
            header.width = width;
            header.height = height;
            header.stride = width * 4;
            Ok(Self { mapping, view, width, height })
        }
    }

    /// Publishes one BGRA frame of exactly `width * height * 4` bytes.
    pub fn write(&mut self, bgra: &[u8]) {
        assert_eq!(bgra.len(), (self.width * self.height * 4) as usize);
        unsafe {
            let base = self.view.Value as *mut u8;
            let header = &*(base as *const Header);
            header.seq.fetch_add(1, Ordering::AcqRel);
            std::ptr::copy_nonoverlapping(bgra.as_ptr(), base.add(HEADER_SIZE), bgra.len());
            header.seq.fetch_add(1, Ordering::Release);
            header.tick.store(windows::Win32::System::SystemInformation::GetTickCount64(), Ordering::Release);
        }
    }
}

impl Drop for FrameWriter {
    fn drop(&mut self) {
        unsafe {
            let _ = UnmapViewOfFile(self.view);
            let _ = CloseHandle(self.mapping);
        }
    }
}
