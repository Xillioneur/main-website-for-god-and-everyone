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
app.use((req, res, next) => {
    res.setHeader('Cross-Origin-Opener-Policy', 'same-origin');
    res.setHeader('Cross-Origin-Embedder-Policy', 'require-corp');
    next();
});
// POINT OF TRUTH: Paths derived from current execution directory
const getTemplatesRoot = () => {
    const paths = [path_1.default.join(process.cwd(), 'backend/templates'), path_1.default.join(__dirname, '../templates')];
    for (const p of paths)
        if (fs_1.default.existsSync(p))
            return p;
    return paths[0];
};
const getFrontendDist = () => {
    const paths = [path_1.default.join(process.cwd(), 'frontend/dist'), path_1.default.join(__dirname, '../../frontend/dist')];
    for (const p of paths)
        if (fs_1.default.existsSync(p))
            return p;
    return paths[0];
};
const templatesRoot = getTemplatesRoot();
const frontendDist = getFrontendDist();
// Note: Metadata like descriptions still come from the source games folder
const wasmGamesSource = path_1.default.join(process.cwd(), 'games');
const googleAnalyticsTag = `<script async src="https://www.googletagmanager.com/gtag/js?id=G-PDHE3BDWQM" crossorigin="anonymous"></script><script>window.dataLayer = window.dataLayer || [];function gtag(){dataLayer.push(arguments);}gtag('js', new Date());gtag('config', 'G-PDHE3BDWQM');</script>`;
const gameVirtues = {
    'divine': 'REDEMPTION', 'ascension': 'CLARITY', 'ashes': 'FORTITUDE', 'parry': 'TEMPERANCE', 'hello': 'INITIATION', 'raylib_example': 'ORDER', 'sdl2_example': 'REACTION'
};
const technicalFoundations = ['hello', 'raylib_example', 'sdl2_example'];
async function getGamesMetadata(req) {
    try {
        if (!fs_1.default.existsSync(wasmGamesSource))
            return [];
        const gameSubdirs = await readdir(wasmGamesSource, { withFileTypes: true });
        const host = req.get('x-forwarded-host') || req.get('host') || 'thedivinecode.vercel.app';
        const protocol = host.includes('localhost') ? 'http' : 'https';
        const baseUrl = `${protocol}://${host}`;
        const games = [];
        for (const dirent of gameSubdirs) {
            if (dirent.isDirectory()) {
                const gameName = dirent.name;
                const gameFolderPath = path_1.default.join(wasmGamesSource, gameName);
                let fullDescription = "Manifestation under study.";
                try {
                    fullDescription = await readFile(path_1.default.join(gameFolderPath, 'description.md'), 'utf8');
                }
                catch (e) { }
                let logicSnippet = "";
                try {
                    logicSnippet = await readFile(path_1.default.join(gameFolderPath, 'logic_snippet.txt'), 'utf8');
                }
                catch (e) { }
                games.push({
                    id: gameName,
                    name: gameName.replace(/_/, ' ').replace(/\b\w/g, l => l.toUpperCase()),
                    type: technicalFoundations.includes(gameName) ? 'FOUNDATION' : 'MANIFESTATION',
                    virtue: gameVirtues[gameName] || 'LOGOS',
                    shortDescription: fullDescription.substring(0, 160).replace(/[#*`]/g, '').replace(/\n/g, ' ') + '...',
                    fullDescription: fullDescription,
                    logicSnippet: logicSnippet,
                    wasmPath: `/wasm/${gameName}/`,
                    previewImageUrl: `${baseUrl}/wasm/${gameName}/preview.png`,
                    mtime: fs_1.default.statSync(gameFolderPath).mtimeMs
                });
            }
        }
        return games.sort((a, b) => b.mtime - a.mtime);
    }
    catch (error) {
        return [];
    }
}
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
    let cleanedHtml = html.replace(/<title>.*?<\/title>/gi, "")
        .replace(/<meta name="(?:description|keywords|author)" content=".*?">/gi, "")
        .replace(/<meta property="og:.*?" content=".*?">/gi, "")
        .replace(/<meta name="twitter:.*?" content=".*?">/gi, "");
    if (cleanedHtml.includes(marker))
        return cleanedHtml.replace(marker, masterSignal);
    return cleanedHtml.replace(/(<head[^>]*>)/i, `$1${masterSignal}`);
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
    const templatePath = path_1.default.join(templatesRoot, 'index.template.html');
    if (!fs_1.default.existsSync(templatePath))
        return res.status(404).send('Template sanctuary empty.');
    try {
        const html = await readFile(templatePath, 'utf8');
        const host = req.get('x-forwarded-host') || req.get('host') || 'thedivinecode.vercel.app';
        const baseUrl = `https://${host}`;
        const homeMeta = `<title>The Divine Code | High-Performance C++ & WebAssembly Sanctuary</title><meta name="description" content="Explore a professional digital sanctuary of high-performance manifestations. Witness the beauty of C++ logic and WebAssembly."><meta property="og:type" content="website"><meta property="og:url" content="${baseUrl}/"><meta property="og:site_name" content="The Divine Code"><meta property="og:title" content="The Divine Code | Sacred WASM Codebase"><meta property="og:description" content="A professional digital sanctuary featuring high-performance manifestations."><meta property="og:image" content="${baseUrl}/homepage-preview.png"><meta property="og:image:width" content="1200"><meta property="og:image:height" content="630"><meta property="twitter:card" content="summary_large_image"><meta property="twitter:site" content="@liwawil"><meta property="twitter:image" content="${baseUrl}/homepage-preview.png">`;
        res.send(injectSacredTags(html, homeMeta));
    }
    catch (error) {
        res.status(500).send('Divine error.');
    }
});
app.get('/wasm/:gameId', async (req, res) => {
    const { gameId } = req.params;
    const templatePath = path_1.default.join(templatesRoot, 'wasm', `${gameId}.template.html`);
    if (!fs_1.default.existsSync(templatePath))
        return res.status(404).send('Manifestation template not found.');
    try {
        const games = await getGamesMetadata(req);
        const game = games.find(g => g.id === gameId);
        if (!game)
            return res.status(404).send('Manifestation data missing.');
        const html = await readFile(templatePath, 'utf8');
        const host = req.get('x-forwarded-host') || req.get('host') || 'thedivinecode.vercel.app';
        const baseUrl = `https://${host}`;
        const divineMeta = `<title>${game.name} | The Divine Code</title><meta name="description" content="${game.shortDescription}"><meta property="og:type" content="article"><meta property="og:url" content="${baseUrl}/wasm/${game.id}/"><meta property="og:site_name" content="The Divine Code"><meta property="og:title" content="${game.name} - The Divine Code"><meta property="og:description" content="${game.shortDescription}"><meta property="og:image" content="${game.previewImageUrl}"><meta property="og:image:width" content="1200"><meta property="og:image:height" content="1200"><meta property="twitter:card" content="summary_large_image"><meta property="twitter:image" content="${game.previewImageUrl}">`;
        res.send(injectSacredTags(html, divineMeta));
    }
    catch (error) {
        res.status(500).send('Divine error.');
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
