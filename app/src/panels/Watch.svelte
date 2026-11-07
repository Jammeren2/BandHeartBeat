<script lang="ts">
  import { RefreshCw } from "@lucide/svelte";
  import { api } from "../lib/api";
  import WatchCard from "../lib/WatchCard.svelte";
  import { store } from "../lib/store.svelte";

  const s = $derived(store.settings!);
</script>

<section>
  <h2 class="section-title">Часы</h2>
  <WatchCard />
  <label class="check">
    <input type="checkbox" checked={s.auto_scan} onchange={(e) => store.set("auto_scan", e.currentTarget.checked)} />
    Автопоиск часов в фоне
  </label>
  <button class="btn" onclick={() => api.retryWatch()}><RefreshCw size={16} />Искать заново</button>
  {#if s.last_device}
    <div class="remembered">
      <span class="hint">Запомненные часы: {s.last_device}</span>
      <button class="btn danger" onclick={() => store.set("last_device", null)}>Забыть</button>
    </div>
  {/if}
</section>

<section>
  <h2 class="section-title">Как включить трансляцию пульса</h2>
  <div class="tip">
    <b>HUAWEI / Honor Band</b>
    <span class="hint">Найдите на часах или в приложении Huawei Health пункт «Трансляция пульса» (HR Data Broadcast) и включите его.</span>
  </div>
  <div class="tip">
    <b>Xiaomi Mi Band / Smart Band</b>
    <span class="hint">Включите трансляцию пульса в приложении Mi Fitness (Zepp Life) и начните тренировку на браслете.</span>
  </div>
  <div class="tip">
    <b>Нагрудные датчики</b>
    <span class="hint">Любой датчик со стандартным Bluetooth-сервисом пульса подключится сам.</span>
  </div>
  <span class="hint">Часы не должны быть заняты другим устройством. Поиск идёт постоянно, пока включён автопоиск.</span>
</section>

<style>
  section {
    display: flex;
    flex-direction: column;
    gap: 12px;
  }
  .tip {
    display: flex;
    flex-direction: column;
    gap: 4px;
    padding: 12px;
    border-radius: 14px;
    background: var(--glass);
  }
  .remembered {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 8px;
  }
</style>
