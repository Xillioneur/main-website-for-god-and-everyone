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
    const paths = [path.join(process.cwd(), 'games'), path.join(__dirname, '../../games')];
    for (const p of paths) if (fs.existsSync(p)) return p;
    return paths[0];
};

const getFrontendDist = () => {
    const paths = [path.join(process.cwd(), 'frontend/dist'), path.join(__dirname, '../../frontend/dist')];
    for (const p of paths) if (fs.existsSync(p)) return p;
    return paths[0];
};

const wasmGamesRoot = getGamesRoot();
const frontendDist = getFrontendDist();

// POINT OF TRUTH: Clean Google Tag
const googleAnalyticsTag = `<script async src="https://www.googletagmanager.com/gtag/js?id=G-PDHE3BDWQM" crossorigin="anonymous"></script><script>window.dataLayer = window.dataLayer || [];function gtag(){dataLayer.push(arguments);}gtag('js', new Date());gtag('config', 'G-PDHE3BDWQM');</script>`;

const gameVirtues: Record<string, string> = {
  'divine': 'REDEMPTION', 'ascension': 'CLARITY', 'ashes': 'FORTITUDE', 'parry': 'TEMPERANCE', 'hello': 'INITIATION', 'raylib_example': 'ORDER', 'sdl2_example': 'REACTION'
};

const technicalFoundations = ['hello', 'raylib_example', 'sdl2_example'];

async function getGamesMetadata(req: express.Request) {
  try {
    const games: any[] = [];
    if (!fs.existsSync(wasmGamesRoot)) return [];
    const gameSubdirs = await readdir(wasmGamesRoot, { withFileTypes: true });
    const host = req.get('x-forwarded-host') || req.get('host');
    const protocol = host?.includes('localhost') ? 'http' : 'https';
    const baseUrl = `${protocol}://${host}`;

    for (const dirent of gameSubdirs) {
      if (dirent.isDirectory()) {
        const gameName = dirent.name;
        const gameFolderPath = path.join(wasmGamesRoot, gameName);
        const gameFiles = await readdir(gameFolderPath);
        const stats = fs.statSync(gameFolderPath);

        let fullDescription = "Manifestation under study.";
        try { fullDescription = await readFile(path.join(gameFolderPath, 'description.md'), 'utf8'); } catch (e) {}

        let logicSnippet = "";
        try { logicSnippet = await readFile(path.join(gameFolderPath, 'logic_snippet.txt'), 'utf8'); } catch (e) {}

        const previewImage = gameFiles.find(f => f.startsWith('preview.')) || 'preview.png';

        games.push({
          id: gameName,
          name: gameName.replace(/_/, ' ').replace(/\b\w/g, l => l.toUpperCase()),
          type: technicalFoundations.includes(gameName) ? 'FOUNDATION' : 'MANIFESTATION',
          virtue: gameVirtues[gameName] || 'LOGOS',
          shortDescription: fullDescription.substring(0, 160).replace(/[#*`]/g, '').replace(/\n/g, ' ') + '...',
          fullDescription: fullDescription,
          logicSnippet: logicSnippet,
          wasmPath: `/wasm/${gameName}/`,
          previewImageUrl: `${baseUrl}/wasm/${gameName}/${previewImage}`,
          mtime: stats.mtimeMs
        });
      }
    }
    return games.sort((a, b) => b.mtime - a.mtime);
  } catch (error) { return []; }
}

async function getDivineCensus() {
    let census = { atomicWeight: 8485, manifestations: 4, foundations: 3, status: 'SANCTIFIED' };
    const p = path.join(process.cwd(), 'backend/census.json');
    if (fs.existsSync(p)) { try { census = JSON.parse(fs.readFileSync(p, 'utf8')); } catch (e) {} }
    const sacredStates = ["GATHERING GRACE", "HARMONIZING THREADS", "PARRYING THE VOID", "MANIFESTING LOGOS", "ASCENDING...", "STILLNESS ACHIEVED", "DIVINE RECKONING ACTIVE", "LATENCY: IMMACULATE", "UPTIME: ETERNAL", "ATOMS ALIGNED"];
    return { ...census, communion: '@liwawil', status: sacredStates[Math.floor(Math.random() * sacredStates.length)] };
}

// THE FINAL FOOLPROOF INJECTION: Targets the sacred marker exclusively
function injectSacredTags(html: string, extraMeta: string = "") {
    const marker = "<!-- DIVINE_META_MANIFESTATION -->";
    const masterSignal = `${googleAnalyticsTag}${extraMeta}`;
    
    if (html.includes(marker)) {
        return html.replace(marker, masterSignal);
    }
    
    // Fallback if marker is missing
    return html.replace(/(<head[^>]*>)/i, `$1${masterSignal}`);
}

app.get('/api/games', async (req, res) => {
  const games = await getGamesMetadata(req);
  res.json(games);
});

app.get('/api/stats', async (req, res) => {
    const stats = await getDivineCensus();
    res.json(stats);
});

app.get('/', async (req, res) => {
  const indexPath = path.join(frontendDist, 'index.template.html');
  if (!fs.existsSync(indexPath)) return res.status(404).send('Divine error: Landing template not found.');
  try {
    let html = await readFile(indexPath, 'utf8');
    const host = req.get('x-forwarded-host') || req.get('host');
    const protocol = host?.includes('localhost') ? 'http' : 'https';
    const baseUrl = `${protocol}://${host}`;
    
    const homeMeta = `
    <title>The Divine Code | High-Performance C++ & WebAssembly Sanctuary</title>
    <meta name="description" content="Explore a professional digital sanctuary featuring high-performance C++ manifestations and sacred logic. Experience Divine Reckoning, Ascension, and more via WebAssembly.">
    <meta property="og:type" content="website">
    <meta property="og:url" content="${baseUrl}/">
    <meta property="og:site_name" content="The Divine Code">
    <meta property="og:title" content="The Divine Code | High-Performance C++ & WebAssembly Sanctuary">
    <meta property="og:description" content="A professional digital sanctuary of high-performance manifestations. Witness the beauty of the Word in code.">
    <meta property="og:image" content="${baseUrl}/homepage-preview.png">
    <meta property="og:image:width" content="1200">
    <meta property="og:image:height" content="630">
    <meta property="twitter:card" content="summary_large_image">
    <meta property="twitter:site" content="@liwawil">
    <meta property="twitter:title" content="The Divine Code | High-Performance C++ & WebAssembly Sanctuary">
    <meta property="twitter:description" content="A professional digital sanctuary of high-performance manifestations. Experience the power of sacred logic.">
    <meta property="twitter:image" content="${baseUrl}/homepage-preview.png">
    `;
    
    res.send(injectSacredTags(html, homeMeta));
  } catch (error) { res.status(500).send('Divine error.'); }
});

app.get('/wasm/:gameId/', async (req, res) => {
  const { gameId } = req.params;
  const gameHtmlPath = path.join(frontendDist, 'wasm', gameId, `${gameId}.template.html`);
  if (!fs.existsSync(gameHtmlPath)) return res.status(404).send('Manifestation not found.');
  try {
    const games = await getGamesMetadata(req);
    const game = games.find(g => g.id === gameId);
    let html = await readFile(gameHtmlPath, 'utf8');
    if (!game) return res.send(html);
    
    const host = req.get('x-forwarded-host') || req.get('host');
    const protocol = host?.includes('localhost') ? 'http' : 'https';
    const baseUrl = `${protocol}://${host}`;
    
    const divineMeta = `
    <title>${game.name} | The Divine Code</title>
    <meta name="description" content="${game.shortDescription}">
    <meta property="og:type" content="article">
    <meta property="og:url" content="${baseUrl}/wasm/${game.id}/">
    <meta property="og:site_name" content="The Divine Code">
    <meta property="og:title" content="${game.name} - The Divine Code">
    <meta property="og:description" content="${game.shortDescription}">
    <meta property="og:image" content="${game.previewImageUrl}">
    <meta property="og:image:width" content="1200">
    <meta property="og:image:height" content="1200">
    <meta property="twitter:card" content="summary_large_image">
    <meta property="twitter:title" content="${game.name} | The Divine Code">
    <meta property="twitter:description" content="${game.shortDescription}">
    <meta property="twitter:image" content="${game.previewImageUrl}">
    `;
    
    res.send(injectSacredTags(html, divineMeta));
  } catch (error) { res.status(500).send('Divine error.'); }
});

app.use(express.static(frontendDist, { index: false }));

app.use((req, res) => {
  const indexPath = path.join(frontendDist, 'index.template.html');
  if (fs.existsSync(indexPath)) {
      res.redirect('/');
  } else {
      res.status(404).send('Sanctuary not found.');
  }
});

export default app;

if (process.env.NODE_ENV !== 'production') {
  app.listen(port, () => console.log(`Backend server listening on http://localhost:${port}`));
}