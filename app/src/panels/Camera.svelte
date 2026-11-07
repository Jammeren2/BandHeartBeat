<script lang="ts">
  import { CircleCheck, CircleAlert } from "@lucide/svelte";
  import { api, type VcamStatus } from "../lib/api";
  import { store } from "../lib/store.svelte";

  const s = $derived(store.settings!);
  const vcam = $derived(store.snap?.vcam);
  let busy = $state(false);
  let message = $state<string | null>(null);

  async function run(action: () => Promise<VcamStatus>) {
    busy = true;
    message = null;
    try {
      await action();
    } catch (e) {
      message = String(e);
    } finally {
      busy = false;
    }
  }
</script>

<section>
  <h2 class="section-title">Кадр</h2>
  <div class="segmented" role="group" aria-label="Заполнение кадра">
    <button class:active={s.fit === "contain"} onclick={() => store.set("fit", "contain")}>Целиком</button>
    <button class:active={s.fit === "cover"} onclick={() => store.set("fit", "cover")}>Заполнить</button>
  </div>
  <span class="hint">
    {s.fit === "contain"
      ? "Камера видна полностью. Если она не 16:9 — по бокам будут полосы."
      : "Кадр заполнен без полос, края камеры не 16:9 обрезаются."}
  </span>
  <label class="check">
    <input type="checkbox" checked={s.mirror} onchange={(e) => store.set("mirror", e.currentTarget.checked)} />
    Отразить по горизонтали
  </label>
</section>

<section>
  <h2 class="section-title">Виртуальная камера</h2>
  <div class="status">
    {#if vcam?.kind === "installed"}
      <CircleCheck size={18} color="var(--ok)" /><span>Установлена · «BandHeartBeat Camera»</span>
    {:else if vcam?.kind === "outdated"}
      <CircleAlert size={18} color="var(--warn)" /><span>Указывает на старую копию программы</span>
    {:else}
      <CircleAlert size={18} color="var(--bad)" /><span>Не установлена</span>
    {/if}
  </div>
  <span class="hint">В Discord: Настройки → Голос и видео → Камера → «BandHeartBeat Camera». Права администратора не нужны.</span>
  <div class="row">
    <button class="btn primary" disabled={busy} onclick={() => run(api.installCamera)}>
      {vcam?.kind === "installed" ? "Переустановить" : "Установить"}
    </button>
    <button class="btn danger" disabled={busy || vcam?.kind === "missing"} onclick={() => run(api.removeCamera)}>Удалить</button>
  </div>
  {#if message}<span class="hint error">{message}</span>{/if}
  <span class="hint">После переустановки перезапустите Discord полностью (через значок в трее).</span>
</section>

<style>
  section {
    display: flex;
    flex-direction: column;
    gap: 12px;
  }
  .segmented {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 4px;
    padding: 4px;
    border-radius: 14px;
    background: var(--glass);
  }
  .segmented button {
    min-height: 38px;
    border: 0;
    border-radius: 10px;
    background: transparent;
    color: var(--muted);
    font-weight: 600;
  }
  .segmented button.active {
    background: var(--glass-strong);
    color: var(--text);
  }
  .status {
    display: flex;
    align-items: center;
    gap: 8px;
    font-weight: 600;
  }
  .row {
    display: flex;
    gap: 8px;
  }
  .error {
    color: var(--bad);
  }
</style>
