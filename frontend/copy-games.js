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
const backendTemplatesDir = path.resolve(__dirname, '../backend/templates');

async function copyGames() {
  try {
    console.log('--- SACRED SYNCHRONIZATION INITIATED ---');

    // 1. Ensure directories exist
    await fs.ensureDir(distWasmDir);
    await fs.ensureDir(backendTemplatesDir);
    await fs.ensureDir(path.join(backendTemplatesDir, 'wasm'));

    // 2. Synchronize ALL games into dist/wasm, FILTERING OUT source
    const items = await fs.readdir(gamesRoot, { withFileTypes: true });
    for (const item of items) {
        if (item.isDirectory()) {
            const src = path.join(gamesRoot, item.name);
            const dest = path.join(distWasmDir, item.name);
            
            await fs.copy(src, dest, {
                filter: (src) => {
                    const ext = path.extname(src).toLowerCase();
                    // Keep ONLY non-source files in the public dist
                    return !['.cpp', '.h', '.hpp', '.o', '.a', '.git', '.html'].includes(ext);
                }
            });

            // Move game HTML to Backend Templates for SEO Injection
            const gameHtml = path.join(src, `${item.name}.html`);
            if (await fs.pathExists(gameHtml)) {
                await fs.copy(gameHtml, path.join(backendTemplatesDir, 'wasm', `${item.name}.template.html`));
            }
            console.log(`Manifested: ${item.name} assets synced and template stored.`);
        }
    }

    // 3. Move main index.html to Backend Templates
    const indexHtml = path.join(distDir, 'index.html');
    if (await fs.pathExists(indexHtml)) {
        await fs.move(indexHtml, path.join(backendTemplatesDir, 'index.template.html'), { overwrite: true });
        console.log('Manifested: index.html moved to template sanctuary.');
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