import express from 'express';
import path from 'path';
import fs from 'fs';
import { promisify } from 'util';

const readdir = promisify(fs.readdir);
const readFile = promisify(fs.readFile);

const app = express();
const port = process.env.PORT || 3000;

// Directory where WASM files are located in the built frontend
const wasmDir = path.join(__dirname, '../../frontend/dist/wasm');

// API routes
app.get('/api', (req, res) => {
  res.json({ message: 'API is working!' });
});

// Function to dynamically get the list of games
async function getGames() {
  try {
    const games: any[] = [];
    const gameSubdirs = await readdir(wasmDir, { withFileTypes: true });

    for (const dirent of gameSubdirs) {
      if (dirent.isDirectory()) {
        const gameName = dirent.name; // e.g., 'hello', 'main'
        const gameFolderPath = path.join(wasmDir, gameName);
        const gameFiles = await readdir(gameFolderPath);

        // Assuming the main game file is named after the folder
        const jsFile = `${gameName}.js`;
        const wasmFile = `${gameName}.wasm`;
        const descriptionMd = 'description.md';
        const previewImage = 'preview.svg'; // or .png

        const jsFileExists = gameFiles.includes(jsFile);
        const wasmFileExists = gameFiles.includes(wasmFile);

        if (jsFileExists && wasmFileExists) {
          let fullDescription = "No description provided.";
          try {
            const mdContent = await readFile(path.join(gameFolderPath, descriptionMd), 'utf8');
            fullDescription = mdContent;
          } catch (error) {
            console.warn(`No description.md found for ${gameName}`);
          }

          games.push({
            id: gameName,
            name: gameName.replace(/_/, ' ').replace(/\b\w/g, l => l.toUpperCase()), // e.g., Hello, Main
            description: fullDescription.substring(0, 100) + '...', // Short description
            fullDescription: `By the grace of the Almighty Creator, this game manifests. ${fullDescription} A divine journey awaits those who dare to seek the truth within the code. Let His light guide your path, and may your pixels be blessed.`,
            wasmPath: `/wasm/${gameName}/${jsFile}`,
            previewImageUrl: `/wasm/${gameName}/${previewImage}`,
          });
        }
      }
    }

    return games;
  } catch (error) {
    console.error('Failed to read WASM directory:', error);
    return [];
  }
}

app.get('/api/games', async (req, res) => {
  const games = await getGames();
  res.json(games);
});

// Serve WASM glue code and WASM modules dynamically from the wasmDir
app.use('/wasm', express.static(wasmDir));

// Serve other static files from the React app
app.use(express.static(path.join(__dirname, '../../frontend/dist')));

// All other unhandled requests will return the React app's index.html
app.use((req, res) => {
  res.sendFile(path.join(__dirname, '../../frontend/dist/index.html'));
});

app.listen(port, () => {
  console.log(`Backend server listening on http://localhost:${port}`);
});