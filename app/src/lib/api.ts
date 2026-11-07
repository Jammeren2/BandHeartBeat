// Typed wrappers around the Rust commands in src-tauri/src/main.rs.
import { Channel, invoke } from "@tauri-apps/api/core";

export type Style = "capsule" | "heart" | "ecg" | "ring" | "minimal" | "card";
export type Placement = "track" | "fixed";
export type Fit = "contain" | "cover";

export interface Settings {
  camera: string | null;
  mirror: boolean;
  fit: Fit;
  style: Style;
  placement: Placement;
  anchor: [number, number];
  size: number;
  smoothing: number;
  pulse_animation: boolean;
  auto_scan: boolean;
  last_device: string | null;
}

export type Link =
  | { kind: "off" }
  | { kind: "scanning" }
  | { kind: "connecting"; name: string }
  | { kind: "connected"; name: string }
  | { kind: "error"; message: string };

export interface CameraStatus {
  camera: string | null;
  mode: { width: number; height: number; fps: number } | null;
  fps: number;
  face: boolean;
  error: string | null;
}

export type VcamStatus = { kind: "installed" } | { kind: "outdated"; path: string } | { kind: "missing" };

export interface Snapshot {
  settings: Settings;
  link: Link;
  bpm: number | null;
  camera: CameraStatus;
  vcam: VcamStatus;
}

export interface CameraInfo {
  id: string;
  name: string;
}

export const api = {
  snapshot: () => invoke<Snapshot>("snapshot"),
  cameras: () => invoke<CameraInfo[]>("cameras"),
  updateSettings: (settings: Settings) => invoke<Settings>("update_settings", { settings }),
  retryWatch: () => invoke<void>("retry_watch"),
  installCamera: () => invoke<VcamStatus>("install_camera"),
  removeCamera: () => invoke<VcamStatus>("remove_camera"),
  watchPreview(onFrame: (jpeg: ArrayBuffer) => void) {
    const channel = new Channel<ArrayBuffer>();
    channel.onmessage = onFrame;
    return invoke<void>("watch_preview", { channel });
  },
};

export const STYLES: { id: Style; label: string }[] = [
  { id: "capsule", label: "Капсула" },
  { id: "heart", label: "Сердце" },
  { id: "ecg", label: "ЭКГ" },
  { id: "ring", label: "Кольцо" },
  { id: "minimal", label: "Минимал" },
  { id: "card", label: "Карточка" },
];
