<script lang="ts">
  import { FACE_POINTS, PLATED, STYLES } from "../lib/api";
  import StyleIcon from "../lib/StyleIcon.svelte";
  import WatchCard from "../lib/WatchCard.svelte";
  import { store } from "../lib/store.svelte";

  const s = $derived(store.settings!);
</script>

<!-- Default panel: everything needed during a call. -->
<section>
  <h2 class="section-title">Часы</h2>
  <WatchCard />
  <label class="check">
    <input type="checkbox" checked={s.auto_scan} onchange={(e) => store.set("auto_scan", e.currentTarget.checked)} />
    Автопоиск часов в фоне
  </label>
</section>

<section>
  <h2 class="section-title">Стиль значка</h2>
  <div class="styles">
    {#each STYLES as style (style.id)}
      <button class="style" class:active={s.style === style.id} onclick={() => store.set("style", style.id)} aria-pressed={s.style === style.id}>
        <StyleIcon style={style.id} />
        <span>{style.label}</span>
      </button>
    {/each}
  </div>
</section>

{#if s.placement === "track"}
  <section>
    <h2 class="section-title">Крепление к лицу</h2>
    <div class="points">
      <button class="chip" class:active={s.face_point === "auto"} onclick={() => store.set("face_point", "auto")}>Авто</button>
      {#each FACE_POINTS as p (p.id)}
        <button class="chip" class:active={s.face_point === p.id} onclick={() => { store.set("face_point", p.id); store.set("point_offset", [0, 0]); }}>
          {p.label}
        </button>
      {/each}
    </div>
    {#if s.face_point !== "auto"}
      <div class="offset">
        <span class="hint">Тяните по превью, чтобы сдвинуть значок от точки</span>
        <button class="btn" disabled={s.point_offset[0] === 0 && s.point_offset[1] === 0} onclick={() => store.set("point_offset", [0, 0])}>Сбросить</button>
      </div>
    {/if}
  </section>
{/if}

<section>
  <label class="slider">
    <span class="row"><span>Размер</span><span>{Math.round(s.size * 100)}%</span></span>
    <input type="range" min="0.5" max="2" step="0.05" value={s.size} oninput={(e) => store.set("size", +e.currentTarget.value)} />
  </label>
  {#if s.placement === "track"}
    <label class="slider">
      <span class="row"><span>Плавность трекинга</span><span>{Math.round(s.smoothing * 100)}%</span></span>
      <input type="range" min="0" max="1" step="0.05" value={s.smoothing} oninput={(e) => store.set("smoothing", +e.currentTarget.value)} />
    </label>
  {/if}
  {#if PLATED.includes(s.style)}
    <label class="check">
      <input type="checkbox" checked={s.badge_background} onchange={(e) => store.set("badge_background", e.currentTarget.checked)} />
      Фон значка
    </label>
  {/if}
  <label class="check">
    <input type="checkbox" checked={s.pulse_animation} onchange={(e) => store.set("pulse_animation", e.currentTarget.checked)} />
    Пульсировать в такт
  </label>
</section>

<style>
  section {
    display: flex;
    flex-direction: column;
    gap: 12px;
  }
  .styles {
    display: grid;
    grid-template-columns: repeat(3, minmax(0, 1fr));
    gap: 8px;
  }
  .style {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 4px;
    padding: 8px 4px 6px;
    border-radius: 12px;
    border: 1px solid var(--line);
    background: transparent;
    font-size: 11px;
    font-weight: 600;
    color: var(--text-2);
  }
  .style:hover {
    background: var(--glass);
  }
  .points {
    display: flex;
    flex-wrap: wrap;
    gap: 6px;
  }
  .chip {
    min-height: 32px;
    padding: 0 12px;
    border-radius: 999px;
    border: 1px solid var(--line);
    background: transparent;
    color: var(--text-2);
    font-size: 12px;
    font-weight: 600;
  }
  .chip.active {
    background: var(--accent);
    border-color: transparent;
    color: #fff;
  }
  .offset {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 8px;
  }
  .style.active {
    border: 2px solid var(--accent);
    background: var(--accent-soft);
    color: var(--text);
  }
</style>
