<script lang="ts">
  import { onMount } from "svelte";
  import { Move, ScanFace, Video } from "@lucide/svelte";
  import { api, FACE_POINTS, type FacePoint } from "./api";
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
  const tracking = $derived(s?.placement === "track" && !!s?.camera);
  const face = $derived(tracking ? cam?.face_box ?? null : null);
  // Dragging moves the anchor in the fixed mode, or the offset from a chosen face point.
  const draggable = $derived(fixed || (!!face && s?.face_point !== "auto"));

  /** Face point position as 0..1 of the frame. */
  function pointAt(at: [number, number], box: [number, number, number, number]): [number, number] {
    return [box[0] + box[2] * at[0], box[1] + box[3] * at[1]];
  }

  function cursor(e: PointerEvent): [number, number] {
    const r = canvas.getBoundingClientRect();
    return [Math.min(1, Math.max(0, (e.clientX - r.left) / r.width)), Math.min(1, Math.max(0, (e.clientY - r.top) / r.height))];
  }

  function drag(e: PointerEvent) {
    const [x, y] = cursor(e);
    if (fixed) {
      store.set("anchor", [x, y]);
      return;
    }
    const point = FACE_POINTS.find((p) => p.id === s?.face_point);
    if (!point || !face) return;
    const [px, py] = pointAt(point.at, face);
    // Offsets are in face widths, measured in output pixels (1280x720).
    const unit = face[2] * 1280;
    store.set("point_offset", [((x - px) * 1280) / unit, ((y - py) * 720) / unit]);
  }

  function down(e: PointerEvent) {
    if (!draggable) return;
    dragging = true;
    canvas.setPointerCapture(e.pointerId);
    drag(e);
  }

  function choose(id: FacePoint) {
    store.set("face_point", id);
    store.set("point_offset", [0, 0]);
  }
</script>

<div class="frame" class:draggable>
  <canvas
    bind:this={canvas}
    width="640"
    height="360"
    onpointerdown={down}
    onpointermove={(e) => dragging && drag(e)}
    onpointerup={() => (dragging = false)}
    aria-label="Превью виртуальной камеры"
  ></canvas>

  {#if face}
    <!-- Only in this preview: Discord never sees the face box or the points. -->
    <div class="face" style="left:{face[0] * 100}%;top:{face[1] * 100}%;width:{face[2] * 100}%;height:{face[3] * 100}%"></div>
    {#each FACE_POINTS as p (p.id)}
      {@const [x, y] = pointAt(p.at, face)}
      <button
        class="point"
        class:active={s?.face_point === p.id}
        style="left:{x * 100}%;top:{y * 100}%"
        onclick={() => choose(s?.face_point === p.id ? "auto" : p.id)}
        title={s?.face_point === p.id ? `${p.label} — нажмите, чтобы вернуть «Авто»` : p.label}
        aria-label={p.label}
      ></button>
    {/each}
  {/if}

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
    {:else if face && s?.face_point === "auto"}
      <span class="chip">Нажмите точку на лице, чтобы прикрепить значок</span>
    {:else if face}
      <span class="chip"><Move size={14} />Тяните по кадру — смещение от точки</span>
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
  .draggable canvas {
    cursor: crosshair;
  }
  .face {
    position: absolute;
    border: 1.5px dashed rgba(255, 255, 255, 0.35);
    border-radius: 18%;
    pointer-events: none;
    transition: all 0.45s linear;
  }
  .point {
    position: absolute;
    width: 18px;
    height: 18px;
    margin: -9px 0 0 -9px;
    padding: 0;
    border-radius: 50%;
    border: 2px solid #fff;
    background: rgba(255, 255, 255, 0.25);
    box-shadow: 0 0 0 4px rgba(0, 0, 0, 0.25);
    transition: left 0.45s linear, top 0.45s linear, transform 0.15s;
  }
  .point:hover {
    transform: scale(1.25);
  }
  .point.active {
    background: var(--accent);
    border-color: #fff;
    box-shadow: 0 0 0 5px var(--accent-soft), 0 0 14px var(--accent);
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
