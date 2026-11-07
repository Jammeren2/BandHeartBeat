import { defineConfig } from "vite";
import { svelte } from "@sveltejs/vite-plugin-svelte";

// Tauri expects a fixed port and no screen clearing in dev.
export default defineConfig({
  plugins: [svelte()],
  clearScreen: false,
  // Mascot art lives in crates/core/assets and is shared with the renderer.
  server: { port: 5173, strictPort: true, fs: { allow: [".."] } },
  build: { target: "es2022" },
});
