import fs from 'fs-extra';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const gamesRoot = path.resolve(__dirname, '../games');
const publicWasmDir = path.resolve(__dirname, './public/wasm');

async function copyGames() {
  try {
    await fs.emptyDir(publicWasmDir); // Clear existing files in public/wasm
    console.log('Cleared existing files in public/wasm.');

    const gameFolders = await fs.readdir(gamesRoot, { withFileTypes: true });

    for (const folder of gameFolders) {
      if (folder.isDirectory()) {
        const gameName = folder.name;
        const sourceGameDir = path.join(gamesRoot, gameName);
        const destGameDir = path.join(publicWasmDir, gameName);

        await fs.ensureDir(destGameDir); // Ensure destination subfolder exists

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