import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'

export default defineConfig({
  plugins: [vue()],
  server: {
    port: 5173,
    host: '0.0.0.0',
    hmr: false,
    headers: {
      'Cache-Control': 'no-store',
    },
    proxy: {
      '/api/v1/stats': { target: 'http://100.107.105.99:8080', changeOrigin: true },
      '/api/v1/smogon': { target: 'http://100.107.105.99:8080', changeOrigin: true },
      '/api': { target: 'http://localhost:9000', changeOrigin: true },
      '/ws': { target: 'http://localhost:9000', ws: true, changeOrigin: true },
    },
  },
  build: {
    outDir: 'dist',
    assetsDir: 'assets',
  }
})
