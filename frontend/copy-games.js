import fs from 'fs-extra';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const gamesRoot = path.resolve(__dirname, '../games');
const publicDir = path.resolve(__dirname, './public');
const distDir = path.resolve(__dirname, './dist');
const distAssetsDir = path.resolve(__dirname, './dist/assets');

async function copyGames() {
  try {
    console.log('--- SACRED SYNCHRONIZATION INITIATED ---');

    // 1. Synchronize main-theme.css
    const mainThemeCssSrc = path.join(distAssetsDir, 'main-theme.css');
    const mainThemeCssPublic = path.join(publicDir, 'main-theme.css');
    const mainThemeCssDist = path.join(distDir, 'main-theme.css');

    if (await fs.pathExists(mainThemeCssSrc)) {
        await fs.copy(mainThemeCssSrc, mainThemeCssPublic);
        await fs.copy(mainThemeCssSrc, mainThemeCssDist);
        console.log('Manifested: main-theme.css synced to public/ and dist/');
    }

    // 2. Synchronize game_shell.css
    const gameShellCssSrc = path.join(gamesRoot, 'game_shell.css');
    const gameShellCssPublic = path.join(publicDir, 'game_shell.css');
    const gameShellCssDist = path.join(distDir, 'game_shell.css');

    if (await fs.pathExists(gameShellCssSrc)) {
        await fs.copy(gameShellCssSrc, gameShellCssPublic);
        await fs.copy(gameShellCssSrc, gameShellCssDist);
        console.log('Manifested: game_shell.css synced to public/ and dist/');
    }

    console.log('--- SACRED SYNCHRONIZATION COMPLETE ---');
  } catch (err) {
    console.error('Manifestation Error during synchronization:', err);
    process.exit(1);
  }
}

copyGames();