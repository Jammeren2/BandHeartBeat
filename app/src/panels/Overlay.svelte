<script lang="ts">
  import { STYLES } from "../lib/api";
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
  .style.active {
    border: 2px solid var(--accent);
    background: var(--accent-soft);
    color: var(--text);
  }
</style>
