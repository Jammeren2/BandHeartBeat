// Typed wrappers around the Rust commands in src-tauri/src/main.rs.
import { Channel, invoke } from "@tauri-apps/api/core";

export type Style = "capsule" | "heart" | "ecg" | "ring" | "minimal" | "card";
export type Placement = "track" | "fixed";
export type Fit = "contain" | "cover";
export type FacePoint = "auto" | "above" | "left" | "right" | "chin" | "left_shoulder" | "right_shoulder";

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
  badge_background: boolean;
  face_point: FacePoint;
  /** Offset from the face point, in face widths. */
  point_offset: [number, number];
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
  /** x, y, w, h in 0..1 of the output frame. */
  face_box: [number, number, number, number] | null;
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

/** Must match `FacePoint::position` in crates/core/src/settings.rs. */
export const FACE_POINTS: { id: Exclude<FacePoint, "auto">; label: string; at: [number, number] }[] = [
  { id: "above", label: "Над головой", at: [0.5, -0.35] },
  { id: "left", label: "Слева", at: [-0.45, 0.35] },
  { id: "right", label: "Справа", at: [1.45, 0.35] },
  { id: "chin", label: "Под подбородком", at: [0.5, 1.3] },
  { id: "left_shoulder", label: "Левое плечо", at: [-0.6, 1.6] },
  { id: "right_shoulder", label: "Правое плечо", at: [1.6, 1.6] },
];

/** Styles whose plate can be switched off. */
export const PLATED: Style[] = ["capsule", "ecg", "card"];

export const STYLES: { id: Style; label: string }[] = [
  { id: "capsule", label: "Капсула" },
  { id: "heart", label: "Сердце" },
  { id: "ecg", label: "ЭКГ" },
  { id: "ring", label: "Кольцо" },
  { id: "minimal", label: "Минимал" },
  { id: "card", label: "Карточка" },
];
