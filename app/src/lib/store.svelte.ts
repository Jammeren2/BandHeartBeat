// App state: a live snapshot polled from Rust plus optimistic settings edits.
import { api, type CameraInfo, type Settings, type Snapshot } from "./api";

const POLL_MS = 500;
const SAVE_DELAY_MS = 120;

class Store {
  snap = $state<Snapshot | null>(null);
  cameras = $state<CameraInfo[]>([]);
  /** Local copy edited by the UI; sent to Rust shortly after each change. */
  settings = $state<Settings | null>(null);
  error = $state<string | null>(null);

  private saveTimer: ReturnType<typeof setTimeout> | undefined;
  private dirty = false;

  async start() {
    await this.refreshCameras();
    await this.poll();
    setInterval(() => this.poll(), POLL_MS);
  }

  async refreshCameras() {
    try {
      this.cameras = await api.cameras();
    } catch (e) {
      this.error = String(e);
    }
  }

  private async poll() {
    try {
      const snap = await api.snapshot();
      this.snap = snap;
      if (!this.dirty) this.settings = snap.settings;
    } catch (e) {
      this.error = String(e);
    }
  }

  /** Applies a change locally at once and persists it after a short pause. */
  set<K extends keyof Settings>(key: K, value: Settings[K]) {
    if (!this.settings) return;
    this.settings = { ...this.settings, [key]: value };
    this.dirty = true;
    this.edits++;
    clearTimeout(this.saveTimer);
    this.saveTimer = setTimeout(() => this.flush(), SAVE_DELAY_MS);
  }

  private async flush() {
    if (!this.settings) return;
    const edit = ++this.edits;
    try {
      const saved = await api.updateSettings($state.snapshot(this.settings));
      this.error = null;
      // Newer edits made while saving win over the echoed copy.
      if (edit === this.edits) this.settings = saved;
    } catch (e) {
      this.error = String(e);
    } finally {
      if (edit === this.edits) this.dirty = false;
    }
  }

  private edits = 0;
}

export const store = new Store();
