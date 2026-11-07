<script lang="ts">
  import { onMount } from "svelte";
  import { Camera, ChevronDown, FlipHorizontal2, HeartPulse, Pin, ScanFace, Watch } from "@lucide/svelte";
  import Preview from "./lib/Preview.svelte";
  import { store } from "./lib/store.svelte";
  import OverlayPanel from "./panels/Overlay.svelte";
  import WatchPanel from "./panels/Watch.svelte";
  import CameraPanel from "./panels/Camera.svelte";

  type Section = "overlay" | "watch" | "camera";
  const NAV: { id: Section; label: string; icon: typeof Camera }[] = [
    { id: "overlay", label: "Эфир", icon: HeartPulse },
    { id: "watch", label: "Часы", icon: Watch },
    { id: "camera", label: "Камера", icon: Camera },
  ];
  let section = $state<Section>("overlay");

  onMount(() => store.start());

  const s = $derived(store.settings);
  const snap = $derived(store.snap);
  const live = $derived(!!s?.camera && !!snap?.camera.mode);
  const watchTone = $derived(
    snap?.link.kind === "connected" ? "ok" : snap?.link.kind === "error" ? "bad" : snap?.link.kind === "off" ? "" : "warn",
  );
  const watchLabel = $derived.by(() => {
    const link = snap?.link;
    if (!link) return "…";
    if (link.kind === "connected") return snap?.bpm ? `${link.name} · ${snap.bpm} BPM` : link.name;
    if (link.kind === "connecting") return `Подключение: ${link.name}`;
    if (link.kind === "scanning") return "Ищу часы…";
    if (link.kind === "error") return "Bluetooth недоступен";
    return "Часы не ищутся";
  });
</script>

<div class="shell">
  <nav class="glass" aria-label="Разделы">
    <div class="logo" aria-hidden="true"><HeartPulse size={24} color="#fff" /></div>
    {#each NAV as item (item.id)}
      <button
        class="nav-item"
        class:active={section === item.id}
        onclick={() => (section = item.id)}
        aria-label={item.label}
        aria-current={section === item.id ? "page" : undefined}
      >
        <item.icon size={22} />
        <span>{item.label}</span>
      </button>
    {/each}
  </nav>

  <main>
    <header>
      <h1>BandHeartBeat</h1>
      <div class="pill"><span class="dot {watchTone}"></span>{watchLabel}</div>
      <div class="pill"><span class="dot {live ? 'live' : ''}"></span>{live ? "Камера в эфире · 1280×720" : "Камера выключена"}</div>
    </header>

    <div class="stage">
      <Preview />
    </div>

    {#if s}
      <div class="toolbar glass">
        <label class="camera-select">
          <Camera size={18} />
          <select
            value={s.camera ?? ""}
            onfocus={() => store.refreshCameras()}
            onchange={(e) => store.set("camera", e.currentTarget.value || null)}
            aria-label="Камера"
          >
            <option value="">Без камеры</option>
            {#each store.cameras as c (c.id)}
              <option value={c.id}>{c.name}</option>
            {/each}
          </select>
          <ChevronDown size={14} />
        </label>
        <span class="sep"></span>
        <button class="tool" class:on={s.placement === "track"} onclick={() => store.set("placement", "track")}>
          <ScanFace size={18} />Следить за головой
        </button>
        <button class="tool" class:on={s.placement === "fixed"} onclick={() => store.set("placement", "fixed")}>
          <Pin size={18} />Закрепить
        </button>
        <span class="sep"></span>
        <button
          class="tool icon"
          class:on={s.mirror}
          onclick={() => store.set("mirror", !s.mirror)}
          aria-label="Отразить"
          title="Отразить по горизонтали"
        >
          <FlipHorizontal2 size={18} />
        </button>
      </div>
    {/if}
  </main>

  <aside class="glass">
    {#if s}
      {#if section === "overlay"}
        <OverlayPanel />
      {:else if section === "watch"}
        <WatchPanel />
      {:else}
        <CameraPanel />
      {/if}
      <div class="grow"></div>
      {#if store.error}
        <span class="hint error">{store.error}</span>
      {:else}
        <span class="hint">Настройки сохраняются автоматически</span>
      {/if}
    {/if}
  </aside>
</div>

<style>
  .shell {
    height: 100%;
    display: flex;
    gap: 16px;
    padding: 16px;
  }

  nav {
    width: 76px;
    flex: none;
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 6px;
    padding: 16px 0;
  }
  .logo {
    width: 44px;
    height: 44px;
    border-radius: 14px;
    background: var(--accent);
    display: grid;
    place-items: center;
    margin-bottom: 14px;
    box-shadow: 0 8px 24px rgba(255, 122, 144, 0.35);
  }
  .nav-item {
    width: 60px;
    padding: 8px 0 6px;
    border: 0;
    border-radius: 14px;
    background: transparent;
    color: var(--muted);
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 4px;
    font-size: 11px;
    font-weight: 600;
  }
  .nav-item:hover {
    color: var(--text);
  }
  .nav-item.active {
    background: var(--glass-strong);
    color: var(--text);
  }

  main {
    flex: 1;
    min-width: 0;
    display: flex;
    flex-direction: column;
    gap: 14px;
  }
  header {
    display: flex;
    align-items: center;
    gap: 10px;
    flex-wrap: wrap;
  }
  h1 {
    margin: 0 auto 0 0;
    font: 800 20px var(--font-display);
  }
  .pill {
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 8px 14px;
    border-radius: 14px;
    background: var(--glass);
    border: 1px solid var(--line);
    font-size: 13px;
    max-width: 320px;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }
  .stage {
    flex: 1;
    min-height: 0;
    display: flex;
    align-items: center;
    justify-content: center;
  }
  .stage :global(.frame) {
    max-width: calc((100vh - 220px) * 16 / 9);
  }

  .toolbar {
    align-self: center;
    display: flex;
    align-items: center;
    flex-wrap: wrap;
    justify-content: center;
    gap: 6px;
    padding: 8px;
    border-radius: 20px;
  }
  .camera-select {
    position: relative;
    display: flex;
    align-items: center;
    gap: 8px;
    height: 44px;
    padding: 0 12px;
    border-radius: 14px;
    background: var(--glass-strong);
  }
  .camera-select select {
    appearance: none;
    border: 0;
    background: transparent;
    font-weight: 600;
    font-size: 13px;
    max-width: 220px;
    padding-right: 4px;
    cursor: pointer;
  }
  .camera-select option {
    background: #1b1726;
  }
  .sep {
    width: 1px;
    height: 26px;
    background: var(--line);
  }
  .tool {
    display: flex;
    align-items: center;
    gap: 8px;
    height: 44px;
    padding: 0 14px;
    border: 0;
    border-radius: 14px;
    background: transparent;
    color: var(--text-2);
    font-weight: 600;
    font-size: 13px;
  }
  .tool:hover {
    background: var(--glass);
  }
  .tool.icon {
    width: 44px;
    padding: 0;
    justify-content: center;
  }
  .tool.on {
    background: var(--accent);
    color: #fff;
  }

  aside {
    width: 310px;
    flex: none;
    padding: 20px;
    display: flex;
    flex-direction: column;
    gap: 20px;
    overflow-y: auto;
  }
  .grow {
    flex: 1;
  }
  .error {
    color: var(--bad);
  }
</style>
