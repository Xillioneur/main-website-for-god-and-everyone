"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const express_1 = __importDefault(require("express"));
const path_1 = __importDefault(require("path"));
const fs_1 = __importDefault(require("fs"));
const util_1 = require("util");
const readdir = (0, util_1.promisify)(fs_1.default.readdir);
const readFile = (0, util_1.promisify)(fs_1.default.readFile);
const app = (0, express_1.default)();
const port = process.env.PORT || 3000;
// Divine Security & Multithreading Headers (COOP/COEP)
app.use((req, res, next) => {
    res.setHeader('Cross-Origin-Opener-Policy', 'same-origin');
    res.setHeader('Cross-Origin-Embedder-Policy', 'require-corp');
    next();
});
// POINT OF TRUTH: Source directory for games and their metadata
const wasmGamesRoot = path_1.default.join(__dirname, '../../games');
// Google Analytics Tag
const googleAnalyticsTag = `
<script async src="https://www.googletagmanager.com/gtag/js?id=G-LFWV3YSBMT" crossorigin="anonymous"></script>
<script>
  window.dataLayer = window.dataLayer || [];
  function gtag(){dataLayer.push(arguments);}
  gtag('js', new Date());
  gtag('config', 'G-LFWV3YSBMT');
</script>
`;
// Map of games to their primary Virtues
const gameVirtues = {
    'divine': 'REDEMPTION',
    'ascension': 'CLARITY',
    'ashes': 'FORTITUDE',
    'parry': 'TEMPERANCE',
    'hello': 'INITIATION',
    'raylib_example': 'ORDER',
    'sdl2_example': 'REACTION'
};
// Helper to get all game metadata
async function getGamesMetadata() {
    try {
        const games = [];
        if (!fs_1.default.existsSync(wasmGamesRoot))
            return [];
        const gameSubdirs = await readdir(wasmGamesRoot, { withFileTypes: true });
        for (const dirent of gameSubdirs) {
            if (dirent.isDirectory()) {
                const gameName = dirent.name;
                const gameFolderPath = path_1.default.join(wasmGamesRoot, gameName);
                const gameFiles = await readdir(gameFolderPath);
                const gameHtml = `${gameName}.html`;
                const jsFile = `${gameName}.js`;
                const wasmFile = `${gameName}.wasm`;
                const descriptionMd = 'description.md';
                const logicSnippet = 'logic_snippet.cpp';
                // Supported preview formats
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
                        const mdContent = await readFile(path_1.default.join(gameFolderPath, descriptionMd), 'utf8');
                        fullDescription = mdContent;
                    }
                    catch (error) {
                        console.warn(`No description.md found for ${gameName}`);
                    }
                    let logicCode = "";
                    try {
                        const codeContent = await readFile(path_1.default.join(gameFolderPath, logicSnippet), 'utf8');
                        logicCode = codeContent;
                    }
                    catch (error) {
                        console.warn(`No logic_snippet.cpp found for ${gameName}`);
                    }
                    const stats = fs_1.default.statSync(gameFolderPath);
                    games.push({
                        id: gameName,
                        name: gameName.replace(/_/, ' ').replace(/\b\w/g, l => l.toUpperCase()),
                        virtue: gameVirtues[gameName] || 'LOGOS',
                        shortDescription: fullDescription.substring(0, 160).replace(/[#*`]/g, '').replace(/\n/g, ' ') + '...',
                        fullDescription: `By the grace of the Almighty Creator, this code manifests. 

 ${fullDescription} 

 A divine journey awaits those who dare to seek the truth within the logic. Let His light guide your path, and may your pixels be blessed.`,
                        logicSnippet: logicCode,
                        wasmPath: `/wasm/${gameName}/`,
                        previewImageUrl: `/wasm/${gameName}/${previewImage}`,
                        mtime: stats.mtimeMs
                    });
                }
            }
        }
        return games.sort((a, b) => b.mtime - a.mtime);
    }
    catch (error) {
        console.error('Failed to read games directory:', error);
        return [];
    }
}
// --- HOME PAGE SEO INJECTION ---
app.get('/', async (req, res) => {
    const indexPath = path_1.default.join(__dirname, '../../frontend/dist/index.html');
    if (!fs_1.default.existsSync(indexPath)) {
        return res.status(404).send('Sacred Index not found. Please build the frontend.');
    }
    try {
        let html = await readFile(indexPath, 'utf8');
        const host = req.get('host');
        const protocol = (host === null || host === void 0 ? void 0 : host.includes('localhost')) ? 'http' : 'https';
        const baseUrl = `${protocol}://${host}`;
        const hasPngPreview = fs_1.default.existsSync(path_1.default.join(__dirname, '../../frontend/dist/homepage-preview.png'));
        const previewImage = `${baseUrl}/${hasPngPreview ? 'homepage-preview.png' : 'homepage-preview.svg'}`;
        const homeMeta = `${googleAnalyticsTag}
    <title>The Divine Code | High-Performance WebAssembly Codebase</title>
    <meta name="description" content="Explore The Divine Codebase. A collection of high-performance C++ games and logic manifested through WebAssembly. Witness the beauty of sacred code.">
    <meta property="og:type" content="website">
    <meta property="og:url" content="${baseUrl}/">
    <meta property="og:title" content="The Divine Code | Sacred WASM Codebase">
    <meta property="og:description" content="A professional digital sanctuary featuring high-performance WebAssembly code manifestations and divine logic.">
    <meta property="og:image" content="${previewImage}">
    <meta property="twitter:card" content="summary_large_image">
    <meta property="twitter:url" content="${baseUrl}/">
    <meta property="twitter:title" content="The Divine Code | Sacred WebAssembly Codebase">
    <meta property="twitter:description" content="High-performance C++ codebases manifested through the power of WebAssembly. All glory to the Divine Architect.">
    <meta property="twitter:image" content="${previewImage}">

    <script type="application/ld+json">
    {
      "@context": "https://schema.org",
      "@type": "WebSite",
      "name": "The Divine Code",
      "alternateName": "The Divine Codebase",
      "url": "${baseUrl}/",
      "description": "High-performance digital manifestations where logic serves beauty, and every line of code is a pilgrimage toward the Infinite.",
      "publisher": {
        "@type": "Organization",
        "name": "The Divine Architects",
        "logo": {
          "@type": "ImageObject",
          "url": "${baseUrl}/favicon.svg"
        }
      }
    }
    </script>
    `;
        html = html.replace(/<title>.*?<\/title>/i, '');
        html = html.replace(/<head>/i, `<head>${homeMeta}`);
        res.send(html);
    }
    catch (error) {
        console.error('Home SEO Injection failed:', error);
        res.sendFile(indexPath);
    }
});
app.get('/api/games', async (req, res) => {
    const games = await getGamesMetadata();
    res.json(games);
});
// SEO: Dynamic Sitemap Route
app.get('/sitemap.xml', async (req, res) => {
    const games = await getGamesMetadata();
    const host = req.get('host');
    const protocol = (host === null || host === void 0 ? void 0 : host.includes('localhost')) ? 'http' : 'https';
    const baseUrl = `${protocol}://${host}`;
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
    res.header('Content-Type', 'application/xml; charset=utf-8');
    res.send(sitemap);
});
// --- DIVINE SEO INJECTION ROUTE FOR GAMES ---
app.get('/wasm/:gameId/', async (req, res) => {
    const { gameId } = req.params;
    const gameHtmlPath = path_1.default.join(wasmGamesRoot, gameId, `${gameId}.html`);
    if (!fs_1.default.existsSync(gameHtmlPath)) {
        return res.status(404).send('Divine Manifestation not found.');
    }
    try {
        const games = await getGamesMetadata();
        const game = games.find(g => g.id === gameId);
        let html = await readFile(gameHtmlPath, 'utf8');
        if (!game) {
            return res.send(html);
        }
        const host = req.get('host');
        const protocol = (host === null || host === void 0 ? void 0 : host.includes('localhost')) ? 'http' : 'https';
        const baseUrl = `${protocol}://${host}`;
        const absoluteImageUrl = `${baseUrl}${game.previewImageUrl}`;
        const divineMeta = `${googleAnalyticsTag}
    <title>${game.name} | The Divine Code</title>
    <meta name="description" content="${game.shortDescription}">
    <meta property="og:type" content="website">
    <meta property="og:url" content="${baseUrl}/wasm/${game.id}/">
    <meta property="og:title" content="${game.name} - A Manifestation of The Divine Code">
    <meta property="og:description" content="${game.shortDescription}">
    <meta property="og:image" content="${absoluteImageUrl}">
    <meta property="twitter:card" content="summary_large_image">
    <meta property="twitter:url" content="${baseUrl}/wasm/${game.id}/">
    <meta property="twitter:title" content="${game.name} | The Divine Code">
    <meta property="twitter:description" content="${game.shortDescription}">
    <meta property="twitter:image" content="${absoluteImageUrl}">

    <script type="application/ld+json">
    {
      "@context": "https://schema.org",
      "@type": "SoftwareApplication",
      "name": "${game.name}",
      "operatingSystem": "Web Browser",
      "applicationCategory": "GameApplication",
      "description": "${game.shortDescription}",
      "image": "${absoluteImageUrl}",
      "author": {
        "@type": "Organization",
        "name": "The Divine Architects"
      },
      "offers": {
        "@type": "Offer",
        "price": "0",
        "priceCurrency": "USD"
      }
    }
    </script>
    `;
        html = html.replace(/<head>/i, `<head>${divineMeta}`);
        res.send(html);
    }
    catch (error) {
        console.error('SEO Injection failed:', error);
        res.sendFile(gameHtmlPath);
    }
});
app.use('/wasm/:gameId', (req, res, next) => {
    const { gameId } = req.params;
    const gameFolderPath = path_1.default.join(wasmGamesRoot, gameId);
    express_1.default.static(gameFolderPath)(req, res, next);
});
app.use(express_1.default.static(path_1.default.join(__dirname, '../../frontend/dist'), { index: false }));
app.use((req, res) => {
    res.sendFile(path_1.default.join(__dirname, '../../frontend/dist/index.html'));
});
app.listen(port, () => {
    console.log(`Backend server listening on http://localhost:${port}`);
});
