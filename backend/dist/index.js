"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const express_1 = __importDefault(require("express"));
const path_1 = __importDefault(require("path"));
const fs_1 = __importDefault(require("fs"));
const util_1 = require("util");
const readFile = (0, util_1.promisify)(fs_1.default.readFile);
const app = (0, express_1.default)();
const port = process.env.PORT || 3000;
app.use((req, res, next) => {
    res.setHeader('Cross-Origin-Opener-Policy', 'same-origin');
    res.setHeader('Cross-Origin-Embedder-Policy', 'require-corp');
    next();
});
// PRIVATE TEMPLATE LOCATION (Not visible to Vercel Static)
const templateRoot = path_1.default.join(process.cwd(), 'backend/templates');
const wasmGamesRoot = path_1.default.join(process.cwd(), 'games');
const frontendDist = path_1.default.join(process.cwd(), 'frontend/dist');
const googleAnalyticsTag = `<script async src="https://www.googletagmanager.com/gtag/js?id=G-PDHE3BDWQM" crossorigin="anonymous"></script><script>window.dataLayer = window.dataLayer || [];function gtag(){dataLayer.push(arguments);}gtag('js', new Date());gtag('config', 'G-PDHE3BDWQM');</script>`;
async function getDivineCensus() {
    let census = { atomicWeight: 8485, manifestations: 4, foundations: 3, status: 'SANCTIFIED' };
    const p = path_1.default.join(process.cwd(), 'backend/census.json');
    if (fs_1.default.existsSync(p)) {
        try {
            census = JSON.parse(fs_1.default.readFileSync(p, 'utf8'));
        }
        catch (e) { }
    }
    const sacredStates = ["GATHERING GRACE", "HARMONIZING THREADS", "PARRYING THE VOID", "MANIFESTING LOGOS", "ASCENDING...", "STILLNESS ACHIEVED", "DIVINE RECKONING ACTIVE", "LATENCY: IMMACULATE", "UPTIME: ETERNAL", "ATOMS ALIGNED"];
    return { ...census, communion: '@liwawil', status: sacredStates[Math.floor(Math.random() * sacredStates.length)] };
}
function injectSacredTags(html, extraMeta = "") {
    const marker = "<!-- DIVINE_META_MANIFESTATION -->";
    const masterSignal = `${googleAnalyticsTag}${extraMeta}`;
    // Purge generic meta signals
    let cleanedHtml = html.replace(/<title>.*?<\/title>/gi, "");
    cleanedHtml = cleanedHtml.replace(/<meta name="(?:description|keywords|author)" content=".*?">/gi, "");
    cleanedHtml = cleanedHtml.replace(/<meta property="og:.*?" content=".*?">/gi, "");
    cleanedHtml = cleanedHtml.replace(/<meta name="twitter:.*?" content=".*?">/gi, "");
    if (html.includes(marker)) {
        return cleanedHtml.replace(marker, masterSignal);
    }
    return cleanedHtml.replace(/(<head[^>]*>)/i, `$1${masterSignal}`);
}
app.get('/api/games', async (req, res) => {
    // Use pre-baked census data for counts
    const census = await getDivineCensus();
    const host = req.get('x-forwarded-host') || req.get('host') || 'thedivinecode.vercel.app';
    const protocol = host.includes('localhost') ? 'http' : 'https';
    const baseUrl = `${protocol}://${host}`;
    // We serve the metadata required by the frontend
    const gameNames = ['divine', 'ascension', 'ashes', 'parry', 'hello', 'raylib_example', 'sdl2_example'];
    const gameVirtues = {
        'divine': 'REDEMPTION', 'ascension': 'CLARITY', 'ashes': 'FORTITUDE', 'parry': 'TEMPERANCE', 'hello': 'INITIATION', 'raylib_example': 'ORDER', 'sdl2_example': 'REACTION'
    };
    const games = gameNames.map(id => ({
        id,
        name: id.replace(/_/, ' ').replace(/\b\w/g, l => l.toUpperCase()),
        type: ['hello', 'raylib_example', 'sdl2_example'].includes(id) ? 'FOUNDATION' : 'MANIFESTATION',
        virtue: gameVirtues[id] || 'LOGOS',
        shortDescription: "A high-performance manifestation of the Divine Code.",
        wasmPath: `/wasm/${id}/`,
        previewImageUrl: `${baseUrl}/wasm/${id}/preview.png`,
        logicSnippet: "" // Snippets would be read here if needed
    }));
    res.json(games);
});
app.get('/api/stats', async (req, res) => {
    const stats = await getDivineCensus();
    res.json(stats);
});
app.get('/', async (req, res) => {
    const indexPath = path_1.default.join(templateRoot, 'index.template.html');
    try {
        let html = await readFile(indexPath, 'utf8');
        const host = req.get('x-forwarded-host') || req.get('host') || 'thedivinecode.vercel.app';
        const baseUrl = `https://${host}`;
        const homeMeta = `
    <title>The Divine Code | High-Performance C++ & WebAssembly Sanctuary</title>
    <meta name="description" content="A professional digital sanctuary of high-performance logic manifestations. Witness the beauty of the Word in code.">
    <meta property="og:type" content="website">
    <meta property="og:url" content="${baseUrl}/">
    <meta property="og:site_name" content="The Divine Code">
    <meta property="og:title" content="The Divine Code | High-Performance C++ & WebAssembly Sanctuary">
    <meta property="og:description" content="A professional digital sanctuary featuring high-performance WebAssembly code manifestations.">
    <meta property="og:image" content="${baseUrl}/homepage-preview.png">
    <meta property="og:image:width" content="1200">
    <meta property="og:image:height" content="630">
    <meta property="twitter:card" content="summary_large_image">
    <meta property="twitter:site" content="@liwawil">
    <meta property="twitter:image" content="${baseUrl}/homepage-preview.png">
    `;
        res.send(injectSacredTags(html, homeMeta));
    }
    catch (error) {
        res.status(500).send('Sanctuary Initialization Error.');
    }
});
app.get('/wasm/:gameId/', async (req, res) => {
    const { gameId } = req.params;
    const gameHtmlPath = path_1.default.join(templateRoot, 'wasm', gameId, `${gameId}.template.html`);
    try {
        let html = await readFile(gameHtmlPath, 'utf8');
        const host = req.get('x-forwarded-host') || req.get('host') || 'thedivinecode.vercel.app';
        const baseUrl = `https://${host}`;
        const divineMeta = `
    <title>${gameId.toUpperCase()} | The Divine Code</title>
    <meta property="og:title" content="${gameId.toUpperCase()} - The Divine Code">
    <meta property="og:image" content="${baseUrl}/wasm/${gameId}/preview.png">
    <meta property="twitter:card" content="summary_large_image">
    `;
        res.send(injectSacredTags(html, divineMeta));
    }
    catch (error) {
        res.status(500).send('Manifestation not found.');
    }
});
app.use(express_1.default.static(frontendDist, { index: false }));
app.use((req, res) => {
    res.redirect('/');
});
exports.default = app;
if (process.env.NODE_ENV !== 'production') {
    app.listen(port, () => console.log(`Backend server listening on http://localhost:${port}`));
}
