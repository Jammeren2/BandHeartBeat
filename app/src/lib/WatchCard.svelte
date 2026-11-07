<script lang="ts">
  import { RefreshCw, Watch } from "@lucide/svelte";
  import { api } from "./api";
  import { store } from "./store.svelte";

  const link = $derived(store.snap?.link);
  const bpm = $derived(store.snap?.bpm ?? null);

  const view = $derived.by(() => {
    switch (link?.kind) {
      case "connected":
        return { name: link.name, text: bpm ? "Подключено" : "Подключено · ждём пульс", tone: "ok" };
      case "connecting":
        return { name: link.name, text: "Подключение…", tone: "warn" };
      case "scanning":
        return { name: "Поиск часов", text: "Включите трансляцию пульса на часах", tone: "warn" };
      case "error":
        return { name: "Bluetooth", text: link.message, tone: "bad" };
      default:
        return { name: "Автопоиск выключен", text: "Включите, чтобы найти часы", tone: "" };
    }
  });
</script>

<div class="card">
  <div class="icon"><Watch size={20} /></div>
  <div class="info">
    <span class="name">{view.name}</span>
    <span class="status"><span class="dot {view.tone}"></span>{view.text}</span>
  </div>
  {#if bpm}
    <span class="bpm">{bpm}</span>
  {:else}
    <button class="retry" onclick={() => api.retryWatch()} aria-label="Искать заново" title="Искать заново">
      <RefreshCw size={16} />
    </button>
  {/if}
</div>

<style>
  .card {
    display: flex;
    align-items: center;
    gap: 12px;
    padding: 12px;
    border-radius: 16px;
    background: var(--glass);
  }
  .icon {
    width: 40px;
    height: 40px;
    border-radius: 12px;
    background: var(--glass-strong);
    display: grid;
    place-items: center;
    color: var(--text-2);
    flex: none;
  }
  .info {
    display: flex;
    flex-direction: column;
    min-width: 0;
    flex: 1;
  }
  .name {
    font-weight: 600;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }
  .status {
    display: flex;
    align-items: center;
    gap: 6px;
    font-size: 12px;
    color: var(--muted);
  }
  .bpm {
    font: 800 28px var(--font-display);
    color: var(--accent);
  }
  .retry {
    width: 36px;
    height: 36px;
    border-radius: 10px;
    border: 1px solid var(--line);
    background: transparent;
    display: grid;
    place-items: center;
  }
</style>
