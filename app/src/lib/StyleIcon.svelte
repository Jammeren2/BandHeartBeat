<script lang="ts">
  import type { Style } from "./api";

  let { style }: { style: Style } = $props();
  const heart = "M12 21s-7.5-4.6-9.6-9.2C.9 8.5 3 4.5 6.8 4.5c2.1 0 3.5 1.1 5.2 3 1.7-1.9 3.1-3 5.2-3 3.8 0 5.9 4 4.4 7.3C19.5 16.4 12 21 12 21z";
</script>

<!-- Miniatures of the overlay styles rendered by crates/core/src/overlay.rs. -->
<svg viewBox="0 0 64 32" width="64" height="32" aria-hidden="true">
  {#if style === "capsule"}
    <rect x="2" y="5" width="60" height="22" rx="11" fill="rgba(10,12,16,.7)" stroke="rgba(255,255,255,.25)" />
    <path d={heart} transform="translate(7 8) scale(.68)" fill="#ff6b6b" />
    <text x="28" y="21.5" font-size="12" font-weight="700" fill="#fff">87</text>
  {:else if style === "heart"}
    <path d={heart} transform="translate(21 0) scale(.95)" fill="#ff4d6d" stroke="#fff" stroke-width="1" />
    <text x="32" y="31" font-size="9" font-weight="700" fill="#fff" text-anchor="middle">87</text>
  {:else if style === "ecg"}
    <rect x="2" y="3" width="60" height="26" rx="5" fill="rgba(6,20,18,.8)" stroke="rgba(45,212,191,.5)" />
    <path d="M6 13h12l2-3 2 5 3-9 3 11 2-4h12l2-3 2 5 3-9 3 11 2-4h4" fill="none" stroke="#2dd4bf" stroke-width="1.4" />
    <text x="7" y="26" font-size="7" font-family="Consolas, monospace" font-weight="700" fill="#2dd4bf">87 BPM</text>
  {:else if style === "ring"}
    <circle cx="32" cy="16" r="13" fill="rgba(10,12,16,.7)" stroke="rgba(255,255,255,.18)" stroke-width="3" />
    <circle cx="32" cy="16" r="13" fill="none" stroke="#4ade80" stroke-width="3" stroke-dasharray="26 100" transform="rotate(-90 32 16)" stroke-linecap="round" />
    <text x="32" y="20" font-size="10" font-weight="700" fill="#fff" text-anchor="middle">87</text>
  {:else if style === "minimal"}
    <text x="14" y="24" font-size="18" font-weight="800" fill="#fff">87</text>
    <path d={heart} transform="translate(39 12) scale(.45)" fill="#ff6b6b" />
  {:else}
    <rect x="4" y="2" width="56" height="28" rx="5" fill="rgba(255,255,255,.93)" />
    <path d={heart} transform="translate(8 5) scale(.42)" fill="#d9363e" />
    <text x="20" y="14" font-size="9" font-weight="800" fill="#1b1d22">87</text>
    <path d="M8 25l6-2 6 1 6-4 6 1 6-3 6 2 6-3 4 1" fill="none" stroke="#d9363e" stroke-width="1.2" />
  {/if}
</svg>
