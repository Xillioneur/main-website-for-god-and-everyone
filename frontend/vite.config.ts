import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

// https://vite.dev/config/
export default defineConfig({
  plugins: [react()],
  publicDir: 'public',
  server: {
    proxy: {
      '/api': 'http://localhost:3000',
      '/wasm': 'http://localhost:3000'
    },
    fs: {
      allow: ['./public'], // Allow serving files from the public directory
    },
  },
  build: {
    rollupOptions: {
      output: {
        assetFileNames: (assetInfo) => {
          if (assetInfo.name === 'index.css') return 'assets/main-theme.css';
          return 'assets/[name]-[hash][extname]';
        },
      },
    },
  },
})