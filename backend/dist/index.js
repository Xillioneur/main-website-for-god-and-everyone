"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const express_1 = __importDefault(require("express"));
const path_1 = __importDefault(require("path"));
const fs_1 = __importDefault(require("fs"));
const util_1 = require("util");
const child_process_1 = require("child_process");
const os_1 = __importDefault(require("os"));
const readdir = (0, util_1.promisify)(fs_1.default.readdir);
const readFile = (0, util_1.promisify)(fs_1.default.readFile);
const writeFile = (0, util_1.promisify)(fs_1.default.writeFile);
const execAsync = (0, util_1.promisify)(child_process_1.exec);
const app = (0, express_1.default)();
const port = process.env.PORT || 3000;
app.use(express_1.default.json({ limit: '10mb' }));
app.use((req, res, next) => {
    res.setHeader('Cross-Origin-Opener-Policy', 'same-origin');
    res.setHeader('Cross-Origin-Embedder-Policy', 'require-corp');
    next();
});
// POINT OF TRUTH: Paths derived from current execution directory
const getProjectRoot = () => {
    // If we are in backend/src or backend/dist, we need to go up
    let current = process.cwd();
    if (current.endsWith('backend'))
        return path_1.default.join(current, '..');
    return current;
};
const projectRoot = getProjectRoot();
const getTemplatesRoot = () => {
    const paths = [
        path_1.default.join(projectRoot, 'backend/templates'),
        path_1.default.join(__dirname, '../templates')
    ];
    for (const p of paths)
        if (fs_1.default.existsSync(p))
            return p;
    return paths[0];
};
const getFrontendDist = () => {
    const paths = [
        path_1.default.join(projectRoot, 'frontend/dist'),
        path_1.default.join(__dirname, '../../frontend/dist')
    ];
    for (const p of paths)
        if (fs_1.default.existsSync(p))
            return p;
    return paths[0];
};
const getWasmGamesSource = () => {
    const paths = [
        path_1.default.join(projectRoot, 'games'),
        path_1.default.join(projectRoot, 'frontend/dist/wasm')
    ];
    for (const p of paths)
        if (fs_1.default.existsSync(p))
            return p;
    return paths[0];
};
const templatesRoot = getTemplatesRoot();
const frontendDist = getFrontendDist();
const wasmGamesSource = getWasmGamesSource();
const googleAnalyticsTag = `<script async src="https://www.googletagmanager.com/gtag/js?id=G-PDHE3BDWQM" crossorigin="anonymous"></script><script>window.dataLayer = window.dataLayer || [];function gtag(){dataLayer.push(arguments);}gtag('js', new Date());gtag('config', 'G-PDHE3BDWQM');</script>`;
const gameVirtues = {
    'divine': 'REDEMPTION',
    'ascension': 'CLARITY',
    'ashes': 'FORTITUDE',
    'parry': 'TEMPERANCE',
    'hello': 'C++ CORE',
    'raylib_example': 'RAYLIB FRAMEWORK',
    'sdl2_example': 'SDL2 ENGINE'
};
const technicalFoundations = ['hello', 'raylib_example', 'sdl2_example'];
const foundationNames = {
    'hello': 'C++ Modern Standard',
    'raylib_example': 'Raylib Manifestation',
    'sdl2_example': 'SDL2 Input Matrix'
};
async function getGamesMetadata(req) {
    const host = req.get('x-forwarded-host') || req.get('host') || 'thedivinecode.vercel.app';
    const protocol = host.includes('localhost') ? 'http' : 'https';
    const baseUrl = `${protocol}://${host}`;
    const fallbackGames = [
        {
            id: 'divine',
            name: 'Divine Reckoning',
            type: 'MANIFESTATION',
            virtue: 'REDEMPTION',
            shortDescription: 'Our flagship third-person Soulslike manifestation...',
            fullDescription: 'Our flagship third-person Soulslike manifestation was released today...',
            wasmPath: '/wasm/divine/',
            previewImageUrl: `${baseUrl}/wasm/divine/preview.png`,
            mtime: Date.now()
        },
        {
            id: 'ascension',
            name: 'Ascension: Light of Logic',
            type: 'MANIFESTATION',
            virtue: 'CLARITY',
            shortDescription: 'Pushing boundaries of parallel computation...',
            fullDescription: 'This manifestation pushed the boundaries of parallel computation within the browser...',
            wasmPath: '/wasm/ascension/',
            previewImageUrl: `${baseUrl}/wasm/ascension/preview.png`,
            mtime: Date.now() - 100
        },
        {
            id: 'ashes',
            name: 'Ashes of the Scroll',
            type: 'MANIFESTATION',
            virtue: 'FORTITUDE',
            shortDescription: 'The first 3D landscape of the Divine Codebase...',
            fullDescription: 'The first 3D landscape of the Divine Codebase has been manifested...',
            wasmPath: '/wasm/ashes/',
            previewImageUrl: `${baseUrl}/wasm/ashes/preview.png`,
            mtime: Date.now() - 200
        },
        {
            id: 'parry',
            name: 'Ashes of the Bullet',
            type: 'MANIFESTATION',
            virtue: 'TEMPERANCE',
            shortDescription: 'Inaugural challenge of focus and stillness...',
            fullDescription: 'Our inaugural challenge of focus and stillness was born...',
            wasmPath: '/wasm/parry/',
            previewImageUrl: `${baseUrl}/wasm/parry/preview.png`,
            mtime: Date.now() - 300
        },
        {
            id: 'hello',
            name: 'C++ Modern Standard',
            type: 'FOUNDATION',
            virtue: 'C++ CORE',
            shortDescription: 'The bedrock of our digital sanctuary built on C++23...',
            fullDescription: 'A fundamental study in modern C++23 capabilities, demonstrating the efficiency and type-safety of our core codebase.',
            wasmPath: '/wasm/hello/',
            previewImageUrl: `${baseUrl}/wasm/hello/preview.png`,
            mtime: Date.now() - 1000
        },
        {
            id: 'raylib_example',
            name: 'Raylib Manifestation',
            type: 'FOUNDATION',
            virtue: 'RAYLIB FRAMEWORK',
            shortDescription: 'Hardware-accelerated geometry and rendering logic...',
            fullDescription: 'We established the visual laws of our universe through Raylib, utilizing its immediate-mode simplicity for high-performance browser rendering.',
            wasmPath: '/wasm/raylib_example/',
            previewImageUrl: `${baseUrl}/wasm/raylib_example/preview.png`,
            mtime: Date.now() - 1100
        },
        {
            id: 'sdl2_example',
            name: 'SDL2 Input Matrix',
            type: 'FOUNDATION',
            virtue: 'SDL2 ENGINE',
            shortDescription: 'Low-level hardware abstraction and event handling...',
            fullDescription: 'A study in the Simple DirectMedia Layer (SDL2), providing the foundation for low-level cross-platform input and windowing.',
            wasmPath: '/wasm/sdl2_example/',
            previewImageUrl: `${baseUrl}/wasm/sdl2_example/preview.png`,
            mtime: Date.now() - 1200
        }
    ];
    try {
        if (!fs_1.default.existsSync(wasmGamesSource)) {
            console.warn(`WASM Games Source not found at: ${wasmGamesSource}. Using fallback.`);
            return fallbackGames;
        }
        const gameSubdirs = await readdir(wasmGamesSource, { withFileTypes: true });
        console.log(`Scanning for manifestations in: ${wasmGamesSource} (Found ${gameSubdirs.length} items)`);
        const games = [];
        for (const dirent of gameSubdirs) {
            if (dirent.isDirectory() && dirent.name !== 'playground') {
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
                let mtime = Date.now();
                try {
                    const stats = await fs_1.default.promises.stat(gameFolderPath);
                    mtime = stats.mtimeMs;
                }
                catch (e) { }
                games.push({
                    id: gameName,
                    name: foundationNames[gameName] || gameName.replace(/_/, ' ').replace(/\b\w/g, l => l.toUpperCase()),
                    type: technicalFoundations.includes(gameName) ? 'FOUNDATION' : 'MANIFESTATION',
                    virtue: gameVirtues[gameName] || 'LOGOS',
                    shortDescription: fullDescription.substring(0, 160).replace(/[#*`]/g, '').replace(/\n/g, ' ') + '...',
                    fullDescription: fullDescription,
                    logicSnippet: logicSnippet,
                    wasmPath: `/wasm/${gameName}/`,
                    previewImageUrl: `${baseUrl}/wasm/${gameName}/preview.png`,
                    mtime: mtime
                });
            }
        }
        if (games.length === 0) {
            console.warn("No valid game directories found. Using fallback.");
            return fallbackGames;
        }
        return games.sort((a, b) => b.mtime - a.mtime);
    }
    catch (error) {
        console.error("Error fetching games metadata:", error);
        return fallbackGames;
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
app.post('/api/compile', async (req, res) => {
    const { code, settings, fileName } = req.body;
    if (!code)
        return res.status(400).json({ error: 'No code fragment provided.' });
    const playgroundDir = path_1.default.join(projectRoot, 'games/playground');
    const publicPlaygroundDir = path_1.default.join(frontendDist, 'wasm/playground');
    const resourcesDir = path_1.default.join(playgroundDir, 'resources');
    try {
        if (!fs_1.default.existsSync(playgroundDir))
            fs_1.default.mkdirSync(playgroundDir, { recursive: true });
        if (!fs_1.default.existsSync(publicPlaygroundDir))
            fs_1.default.mkdirSync(publicPlaygroundDir, { recursive: true });
        // Inject settings into code if they exist
        let processedCode = code;
        if (settings) {
            processedCode = processedCode.replace(/InitWindow\(\s*\d+\s*,\s*\d+\s*,/g, `InitWindow(${settings.width}, ${settings.height},`);
            processedCode = processedCode.replace(/SetTargetFPS\(\s*\d+\s*\)/g, `SetTargetFPS(${settings.fps})`);
        }
        const activeFile = fileName || 'manifestation.cpp';
        const sourceFile = path_1.default.join(playgroundDir, activeFile);
        await writeFile(sourceFile, processedCode);
        // Find all .cpp files in playground
        const allFiles = await readdir(playgroundDir);
        const cppFiles = allFiles.filter(f => f.endsWith('.cpp')).map(f => path_1.default.join(playgroundDir, f)).join(' ');
        const raylibPath = path_1.default.join(os_1.default.homedir(), 'raylib');
        const raylibSrcPath = path_1.default.join(raylibPath, 'src');
        const libRaylibPath = path_1.default.join(raylibSrcPath, 'libraylib.web.a');
        const shellFile = path_1.default.join(projectRoot, 'games/game_shell.html');
        const outputFile = path_1.default.join(playgroundDir, 'playground.html');
        let preloadFlag = "";
        if (fs_1.default.existsSync(resourcesDir) && fs_1.default.readdirSync(resourcesDir).length > 0) {
            preloadFlag = `--preload-file resources`;
        }
        const optLevel = (settings === null || settings === void 0 ? void 0 : settings.optLevel) || '2';
        const emccCmd = `emcc ${cppFiles} -o ${outputFile} -O${optLevel} -std=c++23 -pthread -I${raylibSrcPath} -L${raylibSrcPath} ${libRaylibPath} -s USE_GLFW=3 -s ASYNCIFY -s FORCE_FILESYSTEM=1 -s USE_SDL=2 -s USE_PTHREADS=1 -s PTHREAD_POOL_SIZE=2 -s MAX_WEBGL_VERSION=2 -s MIN_WEBGL_VERSION=2 --shell-file ${shellFile} -DGRAPHICS_API_OPENGL_ES3 ${preloadFlag}`;
        console.log('Manifesting Fragment:', emccCmd);
        const { stdout, stderr } = await execAsync(emccCmd);
        // After compilation, copy files to public dist for serving
        const filesToCopy = ['playground.js', 'playground.wasm', 'playground.data'];
        for (const file of filesToCopy) {
            const src = path_1.default.join(playgroundDir, file);
            const dest = path_1.default.join(publicPlaygroundDir, file);
            if (fs_1.default.existsSync(src)) {
                fs_1.default.copyFileSync(src, dest);
                console.log(`Synced: ${file} to public sanctuary.`);
            }
        }
        // Also copy resources if they were preloaded
        if (fs_1.default.existsSync(resourcesDir)) {
            const publicResourcesDir = path_1.default.join(publicPlaygroundDir, 'resources');
            if (!fs_1.default.existsSync(publicResourcesDir))
                fs_1.default.mkdirSync(publicResourcesDir, { recursive: true });
            const assets = fs_1.default.readdirSync(resourcesDir);
            for (const asset of assets) {
                fs_1.default.copyFileSync(path_1.default.join(resourcesDir, asset), path_1.default.join(publicResourcesDir, asset));
            }
        }
        // Copy HTML to templates sanctuary
        const htmlSrc = path_1.default.join(playgroundDir, 'playground.html');
        const htmlDest = path_1.default.join(templatesRoot, 'wasm/playground.template.html');
        if (fs_1.default.existsSync(htmlSrc))
            fs_1.default.copyFileSync(htmlSrc, htmlDest);
        res.json({
            success: true,
            message: 'Manifestation complete.',
            logs: stdout + stderr,
            wasmPath: '/wasm/playground/'
        });
    }
    catch (error) {
        console.error('Manifestation Error:', error);
        res.status(500).json({
            error: 'The logic is unsound.',
            logs: (error.stdout || '') + (error.stderr || '') || error.message
        });
    }
});
app.post('/api/upload-asset', async (req, res) => {
    // Basic base64 based upload for MVP simplicity
    const { name, data } = req.body;
    if (!name || !data)
        return res.status(400).json({ error: 'Incomplete fragment.' });
    const playgroundDir = path_1.default.join(projectRoot, 'games/playground');
    const resourcesDir = path_1.default.join(playgroundDir, 'resources');
    try {
        if (!fs_1.default.existsSync(resourcesDir))
            fs_1.default.mkdirSync(resourcesDir, { recursive: true });
        // Remove data:image/png;base64, etc.
        const base64Data = data.replace(/^data:.*?;base64,/, "");
        await writeFile(path_1.default.join(resourcesDir, name), base64Data, 'base64');
        res.json({ success: true, message: `Asset ${name} manifested in resources.` });
    }
    catch (error) {
        res.status(500).json({ error: 'Asset manifestation failed.' });
    }
});
app.post('/api/upload-code', async (req, res) => {
    const { name, data } = req.body;
    if (!name || !data)
        return res.status(400).json({ error: 'Incomplete fragment.' });
    const playgroundDir = path_1.default.join(projectRoot, 'games/playground');
    try {
        if (!fs_1.default.existsSync(playgroundDir))
            fs_1.default.mkdirSync(playgroundDir, { recursive: true });
        const safeName = path_1.default.basename(name);
        await writeFile(path_1.default.join(playgroundDir, safeName), data);
        res.json({ success: true, message: `Code ${name} manifested.` });
    }
    catch (error) {
        res.status(500).json({ error: 'Code manifestation failed.' });
    }
});
app.get('/api/playground-files', async (req, res) => {
    const playgroundDir = path_1.default.join(projectRoot, 'games/playground');
    const resourcesDir = path_1.default.join(playgroundDir, 'resources');
    try {
        const files = [];
        if (fs_1.default.existsSync(playgroundDir)) {
            const rootFiles = await readdir(playgroundDir);
            rootFiles.forEach(f => {
                if (f.endsWith('.cpp') || f.endsWith('.h') || f.endsWith('.hpp')) {
                    files.push(f);
                }
            });
        }
        if (fs_1.default.existsSync(resourcesDir)) {
            const assetFiles = await readdir(resourcesDir);
            assetFiles.forEach(f => files.push(`resources/${f}`));
        }
        res.json({ success: true, files });
    }
    catch (error) {
        res.status(500).json({ error: 'Failed to retrieve files.' });
    }
});
app.get('/api/playground-file-content', async (req, res) => {
    const { name } = req.query;
    if (!name || typeof name !== 'string')
        return res.status(400).json({ error: 'No fragment name.' });
    const playgroundDir = path_1.default.join(projectRoot, 'games/playground');
    const safeName = path_1.default.basename(name);
    const filePath = path_1.default.join(playgroundDir, safeName);
    try {
        if (!fs_1.default.existsSync(filePath))
            return res.status(404).json({ error: 'Fragment not found.' });
        const content = await readFile(filePath, 'utf8');
        res.json({ success: true, content });
    }
    catch (error) {
        res.status(500).json({ error: 'Failed to read fragment.' });
    }
});
app.get('/api/playground-snapshots', async (req, res) => {
    const snapshotsDir = path_1.default.join(projectRoot, 'games/playground/snapshots');
    try {
        if (!fs_1.default.existsSync(snapshotsDir))
            fs_1.default.mkdirSync(snapshotsDir, { recursive: true });
        const files = await readdir(snapshotsDir);
        const snapshots = files.filter(f => f.endsWith('.json')).map(f => {
            const stats = fs_1.default.statSync(path_1.default.join(snapshotsDir, f));
            return { name: f.replace('.json', ''), timestamp: stats.mtimeMs };
        });
        res.json({ success: true, snapshots });
    }
    catch (error) {
        res.status(500).json({ error: 'Failed to retrieve snapshots.' });
    }
});
app.post('/api/save-snapshot', async (req, res) => {
    const { name, code, fileName } = req.body;
    if (!name || !code)
        return res.status(400).json({ error: 'Incomplete snapshot data.' });
    const snapshotsDir = path_1.default.join(projectRoot, 'games/playground/snapshots');
    try {
        if (!fs_1.default.existsSync(snapshotsDir))
            fs_1.default.mkdirSync(snapshotsDir, { recursive: true });
        const snapshotPath = path_1.default.join(snapshotsDir, `${name}.json`);
        await writeFile(snapshotPath, JSON.stringify({ name, code, fileName, timestamp: Date.now() }));
        res.json({ success: true, message: 'Snapshot preserved.' });
    }
    catch (error) {
        res.status(500).json({ error: 'Failed to preserve snapshot.' });
    }
});
app.get('/api/load-snapshot', async (req, res) => {
    const { name } = req.query;
    if (!name || typeof name !== 'string')
        return res.status(400).json({ error: 'No snapshot name.' });
    const snapshotsDir = path_1.default.join(projectRoot, 'games/playground/snapshots');
    try {
        const snapshotPath = path_1.default.join(snapshotsDir, `${name}.json`);
        if (!fs_1.default.existsSync(snapshotPath))
            return res.status(404).json({ error: 'Snapshot not found.' });
        const data = JSON.parse(await readFile(snapshotPath, 'utf8'));
        res.json({ success: true, ...data });
    }
    catch (error) {
        res.status(500).json({ error: 'Failed to load snapshot.' });
    }
});
app.delete('/api/delete-playground-file', async (req, res) => {
    const { name } = req.query;
    if (!name || typeof name !== 'string')
        return res.status(400).json({ error: 'No fragment name.' });
    const playgroundDir = path_1.default.join(projectRoot, 'games/playground');
    const safeName = path_1.default.basename(name);
    const filePath = path_1.default.join(playgroundDir, safeName);
    try {
        if (fs_1.default.existsSync(filePath)) {
            fs_1.default.unlinkSync(filePath);
            res.json({ success: true, message: 'Fragment discarded.' });
        }
        else {
            res.status(404).json({ error: 'Fragment not found.' });
        }
    }
    catch (error) {
        res.status(500).json({ error: 'Failed to discard fragment.' });
    }
});
// Serve static files from frontend/dist FIRST
// This ensures /wasm/game/game.js is served from disk, not by the template route
app.use(express_1.default.static(frontendDist, { index: false }));
// IMPORTANT: Serve WASM/JS/DATA files from the wasm directory
app.use('/wasm', express_1.default.static(path_1.default.join(frontendDist, 'wasm'), {
    setHeaders: (res, filePath) => {
        if (filePath.endsWith('.wasm')) {
            res.setHeader('Content-Type', 'application/wasm');
        }
    }
}));
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
    // If gameId looks like a file (has an extension), it should have been caught by static middleware.
    // If it still reaches here, it's either a directory request or a missing file.
    if (gameId.includes('.')) {
        return res.status(404).send('Manifestation not found.');
    }
    const templatePath = path_1.default.join(templatesRoot, 'wasm', `${gameId}.template.html`);
    if (!fs_1.default.existsSync(templatePath))
        return res.status(404).send('Manifestation template not found.');
    try {
        const games = await getGamesMetadata(req);
        let game = games.find(g => g.id === gameId);
        if (!game && gameId === 'playground') {
            game = {
                id: 'playground',
                name: 'Playground Manifestation',
                shortDescription: 'A custom fragment of logic being manifested.',
                previewImageUrl: '/placeholder-game-preview.png'
            };
        }
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
app.use((req, res) => {
    res.redirect('/');
});
exports.default = app;
if (process.env.NODE_ENV !== 'production') {
    app.listen(port, () => console.log(`Backend server listening on http://localhost:${port}`));
}
