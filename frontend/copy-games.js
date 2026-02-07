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

    // 2. Synchronize ALL games into dist/wasm, FILTERING OUT source, and RENAMING html
    const items = await fs.readdir(gamesRoot, { withFileTypes: true });
    for (const item of items) {
        if (item.isDirectory()) {
            const src = path.join(gamesRoot, item.name);
            const dest = path.join(distWasmDir, item.name);
            
            await fs.copy(src, dest, {
                filter: (src) => {
                    const ext = path.extname(src).toLowerCase();
                    return !['.cpp', '.h', '.hpp', '.o', '.a', '.git'].includes(ext);
                }
            });

            // CLOAKING: Rename game html so Vercel doesn't serve it statically
            const gameHtml = path.join(dest, `${item.name}.html`);
            if (await fs.pathExists(gameHtml)) {
                await fs.move(gameHtml, path.join(dest, `${item.name}.template.html`), { overwrite: true });
            }
            console.log(`Manifested: ${item.name} assets synced and cloaked.`);
        }
    }

    // 3. Cloak the main index.html
    const indexHtml = path.join(distDir, 'index.html');
    if (await fs.pathExists(indexHtml)) {
        await fs.move(indexHtml, path.join(distDir, 'index.template.html'), { overwrite: true });
        console.log('Manifested: index.html cloaked as template.');
    }

    // 4. Synchronize CSS
    const mainThemeCssSrc = path.join(distAssetsDir, 'main-theme.css');
    if (await fs.pathExists(mainThemeCssSrc)) {
        await fs.copy(mainThemeCssSrc, path.join(publicDir, 'main-theme.css'));
        await fs.copy(mainThemeCssSrc, path.join(distDir, 'main-theme.css'));
    }

    const gameShellCssSrc = path.join(gamesRoot, 'game_shell.css');
    if (await fs.pathExists(gameShellCssSrc)) {
        await fs.copy(gameShellCssSrc, path.join(publicDir, 'game_shell.css'));
        await fs.copy(gameShellCssSrc, path.join(distDir, 'game_shell.css'));
    }

    console.log('--- SACRED SYNCHRONIZATION COMPLETE ---');
  } catch (err) {
    console.error('Manifestation Error:', err);
    process.exit(1);
  }
}

copyGames();
