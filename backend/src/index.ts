import express from 'express';
import path from 'path';
import fs from 'fs';
import { promisify } from 'util';

const readdir = promisify(fs.readdir);
const readFile = promisify(fs.readFile);

const app = express();
const port = process.env.PORT || 3000;

// Divine Security & Multithreading Headers (COOP/COEP)
// These are required to enable SharedArrayBuffer for multithreaded WASM.
app.use((req, res, next) => {
  res.setHeader('Cross-Origin-Opener-Policy', 'same-origin');
  res.setHeader('Cross-Origin-Embedder-Policy', 'require-corp');
  next();
});

// POINT OF TRUTH: Source directory for games and their metadata
const wasmGamesRoot = path.join(__dirname, '../../games');

// Helper to get all game metadata
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
        const previewImage = 'preview.svg';

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

          games.push({
            id: gameName,
            name: gameName.replace(/_/, ' ').replace(/\b\w/g, l => l.toUpperCase()),
            shortDescription: fullDescription.substring(0, 160).replace(/[#*`]/g, '').replace(/\n/g, ' ') + '...',
            fullDescription: `By the grace of the Almighty Creator, this game manifests. 

 ${fullDescription} 

 A divine journey awaits those who dare to seek the truth within the code. Let His light guide your path, and may your pixels be blessed.`,
            wasmPath: `/wasm/${gameName}/`,
            previewImageUrl: `/wasm/${gameName}/${previewImage}`,
          });
        }
      }
    }
    return games;
  } catch (error) {
    console.error('Failed to read games directory:', error);
    return [];
  }
}

app.get('/api/games', async (req, res) => {
  const games = await getGamesMetadata();
  res.json(games);
});

// SEO: Dynamic Sitemap Route
app.get('/sitemap.xml', async (req, res) => {
  const games = await getGamesMetadata();
  const baseUrl = `${req.protocol}://${req.get('host')}`;
  
  let sitemap = `<?xml version="1.0" encoding="UTF-8"?>\n`;
  sitemap += `<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">
`;
  sitemap += `  <url><loc>${baseUrl}/</loc><changefreq>daily</changefreq><priority>1.0</priority></url>
`;
  
  games.forEach(game => {
    sitemap += `  <url><loc>${baseUrl}${game.wasmPath}</loc><changefreq>weekly</changefreq><priority>0.8</priority></url>
`;
  });
  
  sitemap += `</urlset>`;
  res.header('Content-Type', 'application/xml');
  res.send(sitemap);
});

// --- DIVINE SEO INJECTION ROUTE ---
app.get('/wasm/:gameId/', async (req, res) => {
  const { gameId } = req.params;
  const gameHtmlPath = path.join(wasmGamesRoot, gameId, `${gameId}.html`);
  
  if (!fs.existsSync(gameHtmlPath)) {
    return res.status(404).send('Divine Manifestation not found.');
  }

  try {
    const games = await getGamesMetadata();
    const game = games.find(g => g.id === gameId);
    
    let html = await readFile(gameHtmlPath, 'utf8');
    
    if (!game) {
      return res.send(html);
    }

    const baseUrl = `${req.protocol}://${req.get('host')}`;
    const absoluteImageUrl = `${baseUrl}${game.previewImageUrl}`;

    // Prepare Divine Meta Tags
    const divineMeta = `
    <!-- PRIMARY META -->
    <title>${game.name} | The Divine Code</title>
    <meta name="description" content="${game.shortDescription}">

    <!-- OPEN GRAPH / FACEBOOK / X -->
    <meta property="og:type" content="website">
    <meta property="og:url" content="${baseUrl}/wasm/${game.id}/">
    <meta property="og:title" content="${game.name} - A Manifestation of The Divine Code">
    <meta property="og:description" content="${game.shortDescription}">
    <meta property="og:image" content="${absoluteImageUrl}">

    <!-- X (TWITTER) -->
    <meta property="twitter:card" content="summary_large_image">
    <meta property="twitter:url" content="${baseUrl}/wasm/${game.id}/">
    <meta property="twitter:title" content="${game.name} | The Divine Code">
    <meta property="twitter:description" content="${game.shortDescription}">
    <meta property="twitter:image" content="${absoluteImageUrl}">

    <!-- JSON-LD STRUCTURED DATA -->
    <script type="application/ld+json">
    {
      "@context": "https://schema.org",
      "@type": "SoftwareApplication",
      "name": "${game.name}",
      "operatingSystem": "Web Browser",
      "applicationCategory": "GameApplication",
      "description": "${game.shortDescription}",
      "image": "${absoluteImageUrl}",
      "offers": {
        "@type": "Offer",
        "price": "0",
        "priceCurrency": "USD"
      }
    }
    </script>
    `;

    // Inject into the <head> using a case-insensitive match
    html = html.replace(/<head>/i, `<head>${divineMeta}`);
    
    res.send(html);
  } catch (error) {
    console.error('SEO Injection failed:', error);
    res.sendFile(gameHtmlPath);
  }
});

// Serve other static game assets
app.use('/wasm/:gameId', (req, res, next) => {
  const { gameId } = req.params;
  const gameFolderPath = path.join(wasmGamesRoot, gameId);
  express.static(gameFolderPath)(req, res, next);
});

// Serve static files from the built frontend
app.use(express.static(path.join(__dirname, '../../frontend/dist')));

// SPA fallback using use() instead of get('*') for Express 5 compatibility
app.use((req, res) => {
  res.sendFile(path.join(__dirname, '../../frontend/dist/index.html'));
});

app.listen(port, () => {
  console.log(`Backend server listening on http://localhost:${port}`);
});
