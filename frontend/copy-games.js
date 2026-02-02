import fs from 'fs-extra';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const gamesRoot = path.resolve(__dirname, '../games');
const publicDir = path.resolve(__dirname, './public');
const publicWasmDir = path.resolve(__dirname, './public/wasm');
const distAssetsDir = path.resolve(__dirname, './dist/assets'); // Fixed path: . instead of ..

async function copyGames() {
  try {
    // 1. Clear existing files in public/wasm
    await fs.emptyDir(publicWasmDir);
    console.log('Cleared existing files in public/wasm.');

    // 2. Copy main-theme.css from dist/assets to public/main-theme.css
    const mainThemeCssSrc = path.join(distAssetsDir, 'main-theme.css');
    const mainThemeCssDest = path.join(publicDir, 'main-theme.css');
    if (await fs.pathExists(mainThemeCssSrc)) {
        await fs.copy(mainThemeCssSrc, mainThemeCssDest);
        console.log(`Copied: main-theme.css to public/`);
    }

    // 3. Copy game_shell.css to public/game_shell.css
    const gameShellCssSrc = path.join(gamesRoot, 'game_shell.css');
    const gameShellCssDest = path.join(publicDir, 'game_shell.css');
    if (await fs.pathExists(gameShellCssSrc)) {
        await fs.copy(gameShellCssSrc, gameShellCssDest);
        console.log(`Copied: game_shell.css to public/`);
    }

    const gameFolders = await fs.readdir(gamesRoot, { withFileTypes: true });

    for (const folder of gameFolders) {
      if (folder.isDirectory()) {
        const gameName = folder.name;
        const sourceGameDir = path.join(gamesRoot, gameName);
        const destGameDir = path.join(publicWasmDir, gameName);

        await fs.ensureDir(destGameDir);

        const gameFiles = await fs.readdir(sourceGameDir);

        for (const file of gameFiles) {
          const srcPath = path.join(sourceGameDir, file);
          const destPath = path.join(destGameDir, file);

          if (file.endsWith('.html') || file.endsWith('.js') || file.endsWith('.wasm') || file.endsWith('.md') || file.endsWith('.svg') || file.endsWith('.png')) {
            await fs.copy(srcPath, destPath);
            console.log(`Copied: ${gameName}/${file}`);
          }
        }
      }
    }
    console.log('Finished copying game files to public/wasm.');
  } catch (err) {
    console.error('Error copying game files:', err);
    process.exit(1);
  }
}

copyGames();