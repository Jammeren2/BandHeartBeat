<script lang="ts">
  import { onMount } from "svelte";
  import { Move, ScanFace, Video } from "@lucide/svelte";
  import { api } from "./api";
  import { store } from "./store.svelte";

  let canvas: HTMLCanvasElement;
  let dragging = $state(false);
  let gotFrame = $state(false);

  // Exactly what Discord receives: the composed 1280x720 frame, downscaled.
  onMount(() => {
    const ctx = canvas.getContext("2d")!;
    let busy = false;
    api.watchPreview(async (jpeg) => {
      if (busy) return; // drop frames rather than queue them
      busy = true;
      try {
        const bitmap = await createImageBitmap(new Blob([jpeg], { type: "image/jpeg" }));
        ctx.drawImage(bitmap, 0, 0, canvas.width, canvas.height);
        bitmap.close();
        gotFrame = true;
      } finally {
        busy = false;
      }
    });
  });

  const s = $derived(store.settings);
  const cam = $derived(store.snap?.camera);
  const fixed = $derived(s?.placement === "fixed");

  function moveAnchor(e: PointerEvent) {
    const r = canvas.getBoundingClientRect();
    const x = Math.min(1, Math.max(0, (e.clientX - r.left) / r.width));
    const y = Math.min(1, Math.max(0, (e.clientY - r.top) / r.height));
    store.set("anchor", [x, y]);
  }

  function down(e: PointerEvent) {
    if (!fixed) return;
    dragging = true;
    canvas.setPointerCapture(e.pointerId);
    moveAnchor(e);
  }
</script>

<div class="frame" class:fixed>
  <canvas
    bind:this={canvas}
    width="640"
    height="360"
    onpointerdown={down}
    onpointermove={(e) => dragging && moveAnchor(e)}
    onpointerup={() => (dragging = false)}
    aria-label="Превью виртуальной камеры"
  ></canvas>

  {#if !gotFrame}
    <div class="empty">Запуск…</div>
  {:else if !s?.camera}
    <div class="empty">
      <Video size={28} />
      <span>Выберите камеру внизу — превью покажет ровно то, что увидит Discord</span>
    </div>
  {/if}

  <div class="chips">
    {#if cam?.mode}
      <span class="chip">{cam.mode.width}×{cam.mode.height} → 1280×720 · {cam.fps.toFixed(0)} fps</span>
    {/if}
    {#if s?.camera && s.placement === "track"}
      <span class="chip" class:on={cam?.face}><ScanFace size={14} />{cam?.face ? "Лицо найдено" : "Ищу лицо"}</span>
    {/if}
    {#if fixed}
      <span class="chip"><Move size={14} />Перетащите значок по кадру</span>
    {/if}
  </div>
  {#if cam?.error}
    <div class="error">{cam.error}</div>
  {/if}
</div>

<style>
  .frame {
    position: relative;
    width: 100%;
    max-height: 100%;
    aspect-ratio: 16 / 9;
    border-radius: var(--radius);
    overflow: hidden;
    border: 1px solid var(--line);
    background: #120f1a;
    box-shadow: 0 20px 60px rgba(0, 0, 0, 0.35);
  }
  canvas {
    display: block;
    width: 100%;
    height: 100%;
  }
  .fixed canvas {
    cursor: crosshair;
  }
  .empty {
    position: absolute;
    inset: 0;
    display: flex;
    flex-direction: column;
    gap: 10px;
    align-items: center;
    justify-content: center;
    color: var(--text-2);
    text-align: center;
    padding: 24px;
    pointer-events: none;
  }
  .chips {
    position: absolute;
    left: 14px;
    bottom: 14px;
    display: flex;
    flex-wrap: wrap;
    gap: 8px;
    pointer-events: none;
  }
  .chip {
    display: inline-flex;
    align-items: center;
    gap: 6px;
    padding: 6px 12px;
    border-radius: 999px;
    background: rgba(0, 0, 0, 0.55);
    font-size: 12px;
    color: var(--text-2);
  }
  .chip.on {
    color: var(--ok);
  }
  .error {
    position: absolute;
    top: 14px;
    left: 14px;
    right: 14px;
    padding: 10px 14px;
    border-radius: 12px;
    background: rgba(127, 29, 29, 0.85);
    font-size: 13px;
  }
</style>
