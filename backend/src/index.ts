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

// ROBUST PATH RESOLUTION
const getGamesRoot = () => {
    const paths = [
        path.join(process.cwd(), 'games'),
        path.join(__dirname, '../../games'),
        path.join(__dirname, '../games')
    ];
    for (const p of paths) {
        if (fs.existsSync(p)) return p;
    }
    return paths[0];
};

const getFrontendDist = () => {
    const paths = [
        path.join(process.cwd(), 'frontend/dist'),
        path.join(__dirname, '../../frontend/dist'),
        path.join(__dirname, '../frontend/dist')
    ];
    for (const p of paths) {
        if (fs.existsSync(p)) return p;
    }
    return paths[0];
};

const wasmGamesRoot = getGamesRoot();
const frontendDist = getFrontendDist();

// POINT OF TRUTH: Clean Google Tag
const googleAnalyticsTag = `<script async src="https://www.googletagmanager.com/gtag/js?id=G-PDHE3BDWQM" crossorigin="anonymous"></script><script>window.dataLayer = window.dataLayer || [];function gtag(){dataLayer.push(arguments);}gtag('js', new Date());gtag('config', 'G-PDHE3BDWQM');</script>`;

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

// REFINED: Read Baked Census from Build-Time
async function getDivineCensus() {
    let census = {
        atomicWeight: 8469, // Fallback
        manifestations: 4,
        foundations: 3,
        status: 'SANCTIFIED'
    };

    const bakedCensusPath = path.join(process.cwd(), 'backend/census.json');
    const localCensusPath = path.join(__dirname, '../census.json');
    const rootCensusPath = path.join(__dirname, '../../backend/census.json');

    const tryPaths = [bakedCensusPath, localCensusPath, rootCensusPath];
    for (const p of tryPaths) {
        if (fs.existsSync(p)) {
            try {
                census = JSON.parse(fs.readFileSync(p, 'utf8'));
                break;
            } catch (e) {
                console.error('Failed to parse census at', p);
            }
        }
    }

    const sacredStates = ["GATHERING GRACE", "HARMONIZING THREADS", "PARRYING THE VOID", "MANIFESTING LOGOS", "ASCENDING...", "STILLNESS ACHIEVED", "DIVINE RECKONING ACTIVE", "LATENCY: IMMACULATE", "UPTIME: ETERNAL", "ATOMS ALIGNED"];
    const randomStatus = sacredStates[Math.floor(Math.random() * sacredStates.length)];

    return { 
        ...census, 
        communion: '@liwawil', 
        status: randomStatus 
    };
}

// Helper to inject tags at the absolute start of head
function injectSacredTags(html: string, extraMeta: string = "") {
    const cleanedHtml = html.replace(/<script async src="https:\/\/www\.googletagmanager\.com\/gtag\/js\?id=G-PDHE3BDWQM".*?<\/script><script>.*?<\/script>/is, "");
    return cleanedHtml.replace(/(<head[^>]*>)/i, `$1${googleAnalyticsTag}${extraMeta}`);
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
  let sitemap = `<?xml version="1.0" encoding="UTF-8"?>\n<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">\n`;
  sitemap += `  <url><loc>${baseUrl}/</loc><changefreq>daily</changefreq><priority>1.0</priority></url>\n`;
  games.forEach(game => {
    sitemap += `  <url><loc>${baseUrl}${game.wasmPath}</loc><changefreq>weekly</changefreq><priority>0.8</priority></url>\n`;
  });
  sitemap += `</urlset>`;
  res.header('Content-Type', 'application/xml; charset=utf-8');
  res.send(sitemap);
});

app.get('/', async (req, res) => {
  const indexPath = path.join(frontendDist, 'index.html');
  if (!fs.existsSync(indexPath)) return res.status(404).send('Build frontend first.');
  try {
    let html = await readFile(indexPath, 'utf8');
    const baseUrl = `${req.get('host')?.includes('localhost') ? 'http' : 'https'}://${req.get('host')}`;
    const homeMeta = `<title>The Divine Code | High-Performance WebAssembly Codebase</title><meta name="description" content="Explore The Divine Codebase. A collection of high-performance C++ games and logic manifested through WebAssembly."><meta property="og:image" content="${baseUrl}/homepage-preview.png"><meta property="twitter:image" content="${baseUrl}/homepage-preview.png">`;
    res.send(injectSacredTags(html, homeMeta));
  } catch (error) {
    res.status(500).send('Divine error in home SEO injection.');
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
    const baseUrl = `${req.get('host')?.includes('localhost') ? 'http' : 'https'}://${req.get('host')}`;
    const divineMeta = `<title>${game.name} | The Divine Code</title><meta property="og:image" content="${baseUrl}${game.previewImageUrl}">`;
    res.send(injectSacredTags(html, divineMeta));
  } catch (error) {
    res.status(500).send('Divine error in game SEO injection.');
  }
});

app.use('/wasm/:gameId', (req, res, next) => {
  express.static(path.join(wasmGamesRoot, req.params.gameId))(req, res, next);
});

app.use(express.static(frontendDist, { index: false }));

app.use((req, res) => {
  res.sendFile(path.join(frontendDist, 'index.html'));
});

export default app;

if (process.env.NODE_ENV !== 'production') {
  app.listen(port, () => console.log(`Backend server listening on http://localhost:${port}`));
}