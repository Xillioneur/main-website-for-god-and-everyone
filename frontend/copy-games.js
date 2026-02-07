import fs from 'fs-extra';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const gamesRoot = path.resolve(__dirname, '../games');
const backendRoot = path.resolve(__dirname, '../backend');
const distDir = path.resolve(__dirname, './dist');
const distWasmDir = path.resolve(__dirname, './dist/wasm');

async function copyGames() {
  try {
    console.log('--- SACRED SYNCHRONIZATION INITIATED ---');

    // 1. Ensure dist/wasm exists
    await fs.ensureDir(distWasmDir);

    // 2. Synchronize ALL games into dist/wasm
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

            // CLOAKING: Move game html to backend/templates so they are PRIVATE
            const gameHtml = path.join(dest, `${item.name}.html`);
            const templateDest = path.join(backendRoot, `templates/wasm/${item.name}`);
            await fs.ensureDir(templateDest);
            
            if (await fs.pathExists(gameHtml)) {
                await fs.move(gameHtml, path.join(templateDest, `${item.name}.template.html`), { overwrite: true });
            }
            console.log(`Manifested: ${item.name} synchronized and template isolated.`);
        }
    }

    // 3. Isolate the main index.html
    const indexHtml = path.join(distDir, 'index.html');
    if (await fs.pathExists(indexHtml)) {
        await fs.ensureDir(path.join(backendRoot, 'templates'));
        await fs.move(indexHtml, path.join(backendRoot, 'templates/index.template.html'), { overwrite: true });
        console.log('Manifested: Landing template isolated in backend.');
    }

    // 4. Synchronize CSS to static dist
    const gameShellCssSrc = path.join(gamesRoot, 'game_shell.css');
    if (await fs.pathExists(gameShellCssSrc)) {
        await fs.copy(gameShellCssSrc, path.join(distDir, 'game_shell.css'));
    }

    console.log('--- SACRED SYNCHRONIZATION COMPLETE ---');
  } catch (err) {
    console.error('Manifestation Error:', err);
    process.exit(1);
  }
}

copyGames();