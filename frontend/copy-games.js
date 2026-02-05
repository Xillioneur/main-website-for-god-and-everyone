import fs from 'fs-extra';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const gamesRoot = path.resolve(__dirname, '../games');
const publicDir = path.resolve(__dirname, './public');
const distDir = path.resolve(__dirname, './dist');
const distAssetsDir = path.resolve(__dirname, './dist/assets');
const distWasmDir = path.resolve(__dirname, './dist/wasm');

async function copyGames() {
  try {
    console.log('--- SACRED SYNCHRONIZATION INITIATED ---');

    // 1. Ensure dist/wasm exists
    await fs.ensureDir(distWasmDir);

    // 2. Synchronize ALL games into dist/wasm for Vercel Static Serving
    // We filter for directories to avoid copying root makefiles/shells
    const items = await fs.readdir(gamesRoot, { withFileTypes: true });
    for (const item of items) {
        if (item.isDirectory()) {
            const src = path.join(gamesRoot, item.name);
            const dest = path.join(distWasmDir, item.name);
            await fs.copy(src, dest);
            console.log(`Manifested: ${item.name} synced to dist/wasm/`);
        }
    }

    // 3. Synchronize main-theme.css
    const mainThemeCssSrc = path.join(distAssetsDir, 'main-theme.css');
    if (await fs.pathExists(mainThemeCssSrc)) {
        await fs.copy(mainThemeCssSrc, path.join(publicDir, 'main-theme.css'));
        await fs.copy(mainThemeCssSrc, path.join(distDir, 'main-theme.css'));
        console.log('Manifested: main-theme.css synced to public/ and dist/');
    }

    // 4. Synchronize game_shell.css
    const gameShellCssSrc = path.join(gamesRoot, 'game_shell.css');
    if (await fs.pathExists(gameShellCssSrc)) {
        await fs.copy(gameShellCssSrc, path.join(publicDir, 'game_shell.css'));
        await fs.copy(gameShellCssSrc, path.join(distDir, 'game_shell.css'));
        console.log('Manifested: game_shell.css synced to public/ and dist/');
    }

    console.log('--- SACRED SYNCHRONIZATION COMPLETE ---');
  } catch (err) {
    console.error('Manifestation Error during synchronization:', err);
    process.exit(1);
  }
}

copyGames();
