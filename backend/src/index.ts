import express from 'express';
import path from 'path';
import fs from 'fs';
import { promisify } from 'util';

const readdir = promisify(fs.readdir);
const readFile = promisify(fs.readFile);

const app = express();
const port = process.env.PORT || 3000;

// Divine Security & Multithreading Headers (COOP/COEP)
app.use((req, res, next) => {
  res.setHeader('Cross-Origin-Opener-Policy', 'same-origin');
  res.setHeader('Cross-Origin-Embedder-Policy', 'require-corp');
  next();
});

const wasmGamesRoot = path.join(__dirname, '../../games');

const googleAnalyticsTag = `
<script async src="https://www.googletagmanager.com/gtag/js?id=G-LFWV3YSBMT" crossorigin="anonymous"></script>
<script>
  window.dataLayer = window.dataLayer || [];
  function gtag(){dataLayer.push(arguments);}
  gtag('js', new Date());
  gtag('config', 'G-LFWV3YSBMT');
</script>
`;

const gameVirtues: Record<string, string> = {
  'divine': 'REDEMPTION',
  'ascension': 'CLARITY',
  'ashes': 'FORTITUDE',
  'parry': 'TEMPERANCE',
  'hello': 'INITIATION',
  'raylib_example': 'ORDER',
  'sdl2_example': 'REACTION'
};

const technicalFoundations = ['hello', 'raylib_example', 'sdl2_example'];

async function getGamesMetadata() {
  try {
    const games: any[] = [];
    if (!fs.existsSync(wasmGamesRoot)) return [];
    
    const gameSubdirs = await readdir(wasmGamesRoot, { withFileTypes: true });

    for (const dirent of gameSubdirs) {
      if (dirent.isDirectory()) {
        const gameName = dirent.name;
        const gameFolderPath = path.join(wasmGamesRoot, gameName);
        const gameFiles = await readdir(gameFolderPath);

        const gameHtml = `${gameName}.html`;
        const jsFile = `${gameName}.js`;
        const wasmFile = `${gameName}.wasm`;
        const descriptionMd = 'description.md';
        const logicSnippet = 'logic_snippet.txt';
        
        const previewFormats = ['preview.png', 'preview.jpg', 'preview.jpeg', 'preview.gif', 'preview.svg'];
        let previewImage = 'preview.svg';
        
        for (const format of previewFormats) {
          if (gameFiles.includes(format)) {
            previewImage = format;
            break;
          }
        }

        const gameHtmlExists = gameFiles.includes(gameHtml);
        const jsFileExists = gameFiles.includes(jsFile);
        const wasmFileExists = gameFiles.includes(wasmFile);

        if (gameHtmlExists && jsFileExists && wasmFileExists) {
          let fullDescription = "No description provided.";
          try {
            const mdContent = await readFile(path.join(gameFolderPath, descriptionMd), 'utf8');
            fullDescription = mdContent;
          } catch (error) {
            console.warn(`No description.md found for ${gameName}`);
          }

          let logicCode = "";
          try {
            const codeContent = await readFile(path.join(gameFolderPath, logicSnippet), 'utf8');
            logicCode = codeContent;
          } catch (error) {
            console.warn(`No logic_snippet.txt found for ${gameName}`);
          }

          const stats = fs.statSync(gameFolderPath);

          games.push({
            id: gameName,
            name: gameName.replace(/_/, ' ').replace(/\b\w/g, l => l.toUpperCase()),
            type: technicalFoundations.includes(gameName) ? 'FOUNDATION' : 'MANIFESTATION',
            virtue: gameVirtues[gameName] || 'LOGOS',
            shortDescription: fullDescription.substring(0, 160).replace(/[#*`]/g, '').replace(/\n/g, ' ') + '...',
            fullDescription: `By the grace of the Almighty Creator, this code manifests. \n\n ${fullDescription} \n\n A divine journey awaits those who dare to seek the truth within the logic. Let His light guide your path, and may your pixels be blessed.`,
            logicSnippet: logicCode,
            wasmPath: `/wasm/${gameName}/`,
            previewImageUrl: `/wasm/${gameName}/${previewImage}`,
            mtime: stats.mtimeMs
          });
        }
      }
    }
    return games.sort((a, b) => b.mtime - a.mtime);
  } catch (error) {
    console.error('Failed to read games directory:', error);
    return [];
  }
}

// NEW: Dynamic Census Logic
async function getDivineCensus() {
    let totalLoc = 0;
    const extensions = ['.cpp', '.h', '.hpp'];
    
    const walkSync = (dir: string) => {
        const files = fs.readdirSync(dir);
        files.forEach((file) => {
            const filePath = path.join(dir, file);
            if (fs.statSync(filePath).isDirectory()) {
                walkSync(filePath);
            } else if (extensions.some(ext => filePath.endsWith(extensions[extensions.indexOf(ext)]))) {
                const content = fs.readFileSync(filePath, 'utf8');
                totalLoc += content.split('\n').length;
            }
        });
    };

    if (fs.existsSync(wasmGamesRoot)) walkSync(wasmGamesRoot);
    const games = await getGamesMetadata();

    return {
        atomicWeight: totalLoc,
        manifestations: games.filter(g => g.type === 'MANIFESTATION').length,
        foundations: games.filter(g => g.type === 'FOUNDATION').length,
        status: 'SANCTIFIED'
    };
}

app.get('/api/games', async (req, res) => {
  const games = await getGamesMetadata();
  res.json(games);
});

app.get('/api/stats', async (req, res) => {
    const stats = await getDivineCensus();
    res.json(stats);
});

app.get('/sitemap.xml', async (req, res) => {
  const games = await getGamesMetadata();
  const host = req.get('host');
  const protocol = host?.includes('localhost') ? 'http' : 'https';
  const baseUrl = `${protocol}://${host}`;
  
  let sitemap = `<?xml version="1.0" encoding="UTF-8"?>\n`;
  sitemap += `<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">\n`;
  sitemap += `  <url><loc>${baseUrl}/</loc><changefreq>daily</changefreq><priority>1.0</priority></url>\n`;
  
  games.forEach(game => {
    sitemap += `  <url><loc>${baseUrl}${game.wasmPath}</loc><changefreq>weekly</changefreq><priority>0.8</priority></url>\n`;
  });
  
  sitemap += `</urlset>`;
  res.header('Content-Type', 'application/xml; charset=utf-8');
  res.send(sitemap);
});

app.get('/', async (req, res) => {
  const indexPath = path.join(__dirname, '../../frontend/dist/index.html');
  if (!fs.existsSync(indexPath)) return res.status(404).send('Build frontend first.');

  try {
    let html = await readFile(indexPath, 'utf8');
    const host = req.get('host');
    const protocol = host?.includes('localhost') ? 'http' : 'https';
    const baseUrl = `${protocol}://${host}`;
    const previewImage = `${baseUrl}/homepage-preview.png`;

    const homeMeta = `${googleAnalyticsTag}
    <title>The Divine Code | High-Performance WebAssembly Codebase</title>
    <meta name="description" content="Explore The Divine Codebase. A collection of high-performance C++ games and logic manifested through WebAssembly.">
    <meta property="og:image" content="${previewImage}">
    <meta property="twitter:image" content="${previewImage}">
    `;

    html = html.replace(/<head>/i, `<head>${homeMeta}`);
    res.send(html);
  } catch (error) {
    res.sendFile(indexPath);
  }
});

app.get('/wasm/:gameId/', async (req, res) => {
  const { gameId } = req.params;
  const gameHtmlPath = path.join(wasmGamesRoot, gameId, `${gameId}.html`);
  if (!fs.existsSync(gameHtmlPath)) return res.status(404).send('Not found.');

  try {
    const games = await getGamesMetadata();
    const game = games.find(g => g.id === gameId);
    let html = await readFile(gameHtmlPath, 'utf8');
    if (!game) return res.send(html);

    const host = req.get('host');
    const baseUrl = `${host?.includes('localhost') ? 'http' : 'https'}://${host}`;
    const divineMeta = `${googleAnalyticsTag}<title>${game.name} | The Divine Code</title><meta property="og:image" content="${baseUrl}${game.previewImageUrl}">`;

    html = html.replace(/<head>/i, `<head>${divineMeta}`);
    res.send(html);
  } catch (error) {
    res.sendFile(gameHtmlPath);
  }
});

app.use('/wasm/:gameId', (req, res, next) => {
  express.static(path.join(wasmGamesRoot, req.params.gameId))(req, res, next);
});

app.use(express.static(path.join(__dirname, '../../frontend/dist'), { index: false }));

app.use((req, res) => {
  res.sendFile(path.join(__dirname, '../../frontend/dist/index.html'));
});

app.listen(port, () => console.log(`Backend server listening on http://localhost:${port}`));