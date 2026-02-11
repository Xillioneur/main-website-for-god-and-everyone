import express from 'express';
import path from 'path';
import fs from 'fs';
import { promisify } from 'util';
import { exec } from 'child_process';
import os from 'os';

const readdir = promisify(fs.readdir);
const readFile = promisify(fs.readFile);
const writeFile = promisify(fs.writeFile);
const execAsync = promisify(exec);

const app = express();
const port = process.env.PORT || 3000;

app.use(express.json({ limit: '10mb' }));

app.use((req, res, next) => {
  res.setHeader('Cross-Origin-Opener-Policy', 'same-origin');
  res.setHeader('Cross-Origin-Embedder-Policy', 'require-corp');
  next();
});

// POINT OF TRUTH: Paths derived from current execution directory
const getTemplatesRoot = () => {
    const paths = [path.join(process.cwd(), 'backend/templates'), path.join(__dirname, '../templates')];
    for (const p of paths) if (fs.existsSync(p)) return p;
    return paths[0];
};

const getFrontendDist = () => {
    const paths = [path.join(process.cwd(), 'frontend/dist'), path.join(__dirname, '../../frontend/dist')];
    for (const p of paths) if (fs.existsSync(p)) return p;
    return paths[0];
};

const getWasmGamesSource = () => {
    const paths = [
        path.join(process.cwd(), 'games'), 
        path.join(__dirname, '../games'), 
        path.join(__dirname, '../../games'),
        path.join(process.cwd(), 'frontend/dist/wasm'),
        path.join(__dirname, '../../frontend/dist/wasm')
    ];
    for (const p of paths) if (fs.existsSync(p)) return p;
    return paths[0];
};

const templatesRoot = getTemplatesRoot();
const frontendDist = getFrontendDist();
const wasmGamesSource = getWasmGamesSource();

const googleAnalyticsTag = `<script async src="https://www.googletagmanager.com/gtag/js?id=G-PDHE3BDWQM" crossorigin="anonymous"></script><script>window.dataLayer = window.dataLayer || [];function gtag(){dataLayer.push(arguments);}gtag('js', new Date());gtag('config', 'G-PDHE3BDWQM');</script>`;

const gameVirtues: Record<string, string> = {
  'divine': 'REDEMPTION', 
  'ascension': 'CLARITY', 
  'ashes': 'FORTITUDE', 
  'parry': 'TEMPERANCE', 
  'hello': 'C++ CORE', 
  'raylib_example': 'RAYLIB FRAMEWORK', 
  'sdl2_example': 'SDL2 ENGINE'
};

const technicalFoundations = ['hello', 'raylib_example', 'sdl2_example'];
const foundationNames: Record<string, string> = {
    'hello': 'C++ Modern Standard',
    'raylib_example': 'Raylib Manifestation',
    'sdl2_example': 'SDL2 Input Matrix'
};

async function getGamesMetadata(req: express.Request) {
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
    if (!fs.existsSync(wasmGamesSource)) {
        console.warn(`WASM Games Source not found at: ${wasmGamesSource}. Using fallback.`);
        return fallbackGames;
    }
    
    const gameSubdirs = await readdir(wasmGamesSource, { withFileTypes: true });
    console.log(`Scanning for manifestations in: ${wasmGamesSource} (Found ${gameSubdirs.length} items)`);
    
    const games: any[] = [];
    for (const dirent of gameSubdirs) {
      if (dirent.isDirectory() && dirent.name !== 'playground') {
        const gameName = dirent.name;
        const gameFolderPath = path.join(wasmGamesSource, gameName);
        let fullDescription = "Manifestation under study.";
        try { fullDescription = await readFile(path.join(gameFolderPath, 'description.md'), 'utf8'); } catch (e) {}
        let logicSnippet = "";
        try { logicSnippet = await readFile(path.join(gameFolderPath, 'logic_snippet.txt'), 'utf8'); } catch (e) {}

        let mtime = Date.now();
        try {
            const stats = await fs.promises.stat(gameFolderPath);
            mtime = stats.mtimeMs;
        } catch (e) {}

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
  } catch (error) { 
    console.error("Error fetching games metadata:", error);
    return fallbackGames; 
  }
}

async function getDivineCensus() {
    let census = { atomicWeight: 8485, manifestations: 4, foundations: 3, status: 'SANCTIFIED' };
    const p = path.join(process.cwd(), 'backend/census.json');
    if (fs.existsSync(p)) { try { census = JSON.parse(fs.readFileSync(p, 'utf8')); } catch (e) {} }
    const sacredStates = ["GATHERING GRACE", "HARMONIZING THREADS", "PARRYING THE VOID", "MANIFESTING LOGOS", "ASCENDING...", "STILLNESS ACHIEVED", "DIVINE RECKONING ACTIVE", "LATENCY: IMMACULATE", "UPTIME: ETERNAL", "ATOMS ALIGNED"];
    return { ...census, communion: '@liwawil', status: sacredStates[Math.floor(Math.random() * sacredStates.length)] };
}

function injectSacredTags(html: string, extraMeta: string = "") {
    const marker = "<!-- DIVINE_META_MANIFESTATION -->";
    const masterSignal = `${googleAnalyticsTag}${extraMeta}`;
    let cleanedHtml = html.replace(/<title>.*?<\/title>/gi, "")
                          .replace(/<meta name="(?:description|keywords|author)" content=".*?">/gi, "")
                          .replace(/<meta property="og:.*?" content=".*?">/gi, "")
                          .replace(/<meta name="twitter:.*?" content=".*?">/gi, "");
    if (cleanedHtml.includes(marker)) return cleanedHtml.replace(marker, masterSignal);
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
    if (!code) return res.status(400).json({ error: 'No code fragment provided.' });

    const playgroundDir = path.join(process.cwd(), 'games/playground');
    const publicPlaygroundDir = path.join(frontendDist, 'wasm/playground');
    const resourcesDir = path.join(playgroundDir, 'resources');
    
    try {
        if (!fs.existsSync(playgroundDir)) fs.mkdirSync(playgroundDir, { recursive: true });
        if (!fs.existsSync(publicPlaygroundDir)) fs.mkdirSync(publicPlaygroundDir, { recursive: true });

        // Inject settings into code if they exist
        let processedCode = code;
        if (settings) {
            processedCode = processedCode.replace(/InitWindow\(\s*\d+\s*,\s*\d+\s*,/g, `InitWindow(${settings.width}, ${settings.height},`);
            processedCode = processedCode.replace(/SetTargetFPS\(\s*\d+\s*\)/g, `SetTargetFPS(${settings.fps})`);
        }

        const activeFile = fileName || 'manifestation.cpp';
        const sourceFile = path.join(playgroundDir, activeFile);
        await writeFile(sourceFile, processedCode);

        // Find all .cpp files in playground
        const allFiles = await readdir(playgroundDir);
        const cppFiles = allFiles.filter(f => f.endsWith('.cpp')).map(f => path.join(playgroundDir, f)).join(' ');

        const raylibPath = path.join(os.homedir(), 'raylib');
        const raylibSrcPath = path.join(raylibPath, 'src');
        const libRaylibPath = path.join(raylibSrcPath, 'libraylib.web.a');
        const shellFile = path.join(process.cwd(), 'games/game_shell.html');

        const outputFile = path.join(playgroundDir, 'playground.html');
        
        let preloadFlag = "";
        if (fs.existsSync(resourcesDir) && fs.readdirSync(resourcesDir).length > 0) {
            preloadFlag = `--preload-file resources`;
        }

        const emccCmd = `emcc ${cppFiles} -o ${outputFile} -O2 -std=c++23 -pthread -I${raylibSrcPath} -L${raylibSrcPath} ${libRaylibPath} -s USE_GLFW=3 -s ASYNCIFY -s FORCE_FILESYSTEM=1 -s USE_SDL=2 -s USE_PTHREADS=1 -s PTHREAD_POOL_SIZE=2 -s MAX_WEBGL_VERSION=2 -s MIN_WEBGL_VERSION=2 --shell-file ${shellFile} -DGRAPHICS_API_OPENGL_ES3 ${preloadFlag}`;

        console.log('Manifesting Fragment:', emccCmd);
        
        const { stdout, stderr } = await execAsync(emccCmd);
        
        // After compilation, copy files to public dist for serving
        const filesToCopy = ['playground.js', 'playground.wasm', 'playground.data'];
        for (const file of filesToCopy) {
            const src = path.join(playgroundDir, file);
            const dest = path.join(publicPlaygroundDir, file);
            if (fs.existsSync(src)) fs.copyFileSync(src, dest);
        }

        // Copy HTML to templates sanctuary
        const htmlSrc = path.join(playgroundDir, 'playground.html');
        const htmlDest = path.join(templatesRoot, 'wasm/playground.template.html');
        if (fs.existsSync(htmlSrc)) fs.copyFileSync(htmlSrc, htmlDest);

        res.json({ 
            success: true, 
            message: 'Manifestation complete.', 
            logs: stdout + stderr,
            wasmPath: '/wasm/playground/'
        });
    } catch (error: any) {
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
    if (!name || !data) return res.status(400).json({ error: 'Incomplete fragment.' });

    const playgroundDir = path.join(process.cwd(), 'games/playground');
    const resourcesDir = path.join(playgroundDir, 'resources');

    try {
        if (!fs.existsSync(resourcesDir)) fs.mkdirSync(resourcesDir, { recursive: true });
        
        // Remove data:image/png;base64, etc.
        const base64Data = data.replace(/^data:.*?;base64,/, "");
        await writeFile(path.join(resourcesDir, name), base64Data, 'base64');
        
        res.json({ success: true, message: `Asset ${name} manifested in resources.` });
    } catch (error: any) {
        res.status(500).json({ error: 'Asset manifestation failed.' });
    }
});

app.get('/api/playground-files', async (req, res) => {
    const playgroundDir = path.join(process.cwd(), 'games/playground');
    const resourcesDir = path.join(playgroundDir, 'resources');
    
    try {
        const files: string[] = [];
        if (fs.existsSync(playgroundDir)) {
            const rootFiles = await readdir(playgroundDir);
            rootFiles.forEach(f => {
                if (f.endsWith('.cpp') || f.endsWith('.h') || f.endsWith('.hpp')) {
                    files.push(f);
                }
            });
        }
        if (fs.existsSync(resourcesDir)) {
            const assetFiles = await readdir(resourcesDir);
            assetFiles.forEach(f => files.push(`resources/${f}`));
        }
        res.json({ success: true, files });
    } catch (error) {
        res.status(500).json({ error: 'Failed to retrieve files.' });
    }
});

app.get('/api/playground-file-content', async (req, res) => {
    const { name } = req.query;
    if (!name || typeof name !== 'string') return res.status(400).json({ error: 'No fragment name.' });

    const playgroundDir = path.join(process.cwd(), 'games/playground');
    const safeName = path.basename(name);
    const filePath = path.join(playgroundDir, safeName);

    try {
        if (!fs.existsSync(filePath)) return res.status(404).json({ error: 'Fragment not found.' });
        const content = await readFile(filePath, 'utf8');
        res.json({ success: true, content });
    } catch (error) {
        res.status(500).json({ error: 'Failed to read fragment.' });
    }
});

app.delete('/api/delete-playground-file', async (req, res) => {
    const { name } = req.query;
    if (!name || typeof name !== 'string') return res.status(400).json({ error: 'No fragment name.' });

    const playgroundDir = path.join(process.cwd(), 'games/playground');
    const safeName = path.basename(name);
    const filePath = path.join(playgroundDir, safeName);

    try {
        if (fs.existsSync(filePath)) {
            fs.unlinkSync(filePath);
            res.json({ success: true, message: 'Fragment discarded.' });
        } else {
            res.status(404).json({ error: 'Fragment not found.' });
        }
    } catch (error) {
        res.status(500).json({ error: 'Failed to discard fragment.' });
    }
});

// Serve static files from frontend/dist FIRST
// This ensures /wasm/game/game.js is served from disk, not by the template route
app.use(express.static(frontendDist, { index: false }));

app.get('/', async (req, res) => {
  const templatePath = path.join(templatesRoot, 'index.template.html');
  if (!fs.existsSync(templatePath)) return res.status(404).send('Template sanctuary empty.');
  try {
    const html = await readFile(templatePath, 'utf8');
    const host = req.get('x-forwarded-host') || req.get('host') || 'thedivinecode.vercel.app';
    const baseUrl = `https://${host}`;
    const homeMeta = `<title>The Divine Code | High-Performance C++ & WebAssembly Sanctuary</title><meta name="description" content="Explore a professional digital sanctuary of high-performance manifestations. Witness the beauty of C++ logic and WebAssembly."><meta property="og:type" content="website"><meta property="og:url" content="${baseUrl}/"><meta property="og:site_name" content="The Divine Code"><meta property="og:title" content="The Divine Code | Sacred WASM Codebase"><meta property="og:description" content="A professional digital sanctuary featuring high-performance manifestations."><meta property="og:image" content="${baseUrl}/homepage-preview.png"><meta property="og:image:width" content="1200"><meta property="og:image:height" content="630"><meta property="twitter:card" content="summary_large_image"><meta property="twitter:site" content="@liwawil"><meta property="twitter:image" content="${baseUrl}/homepage-preview.png">`;
    res.send(injectSacredTags(html, homeMeta));
  } catch (error) { res.status(500).send('Divine error.'); }
});

app.get('/wasm/:gameId', async (req, res) => {
  const { gameId } = req.params;
  
  // If gameId looks like a file (has an extension), don't serve it as a template
  if (gameId.includes('.')) {
      return res.status(404).send('Manifestation not found.');
  }

  const templatePath = path.join(templatesRoot, 'wasm', `${gameId}.template.html`);
  if (!fs.existsSync(templatePath)) return res.status(404).send('Manifestation template not found.');
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

    if (!game) return res.status(404).send('Manifestation data missing.');
    const html = await readFile(templatePath, 'utf8');
    const host = req.get('x-forwarded-host') || req.get('host') || 'thedivinecode.vercel.app';
    const baseUrl = `https://${host}`;
    const divineMeta = `<title>${game.name} | The Divine Code</title><meta name="description" content="${game.shortDescription}"><meta property="og:type" content="article"><meta property="og:url" content="${baseUrl}/wasm/${game.id}/"><meta property="og:site_name" content="The Divine Code"><meta property="og:title" content="${game.name} - The Divine Code"><meta property="og:description" content="${game.shortDescription}"><meta property="og:image" content="${game.previewImageUrl}"><meta property="og:image:width" content="1200"><meta property="og:image:height" content="1200"><meta property="twitter:card" content="summary_large_image"><meta property="twitter:image" content="${game.previewImageUrl}">`;
    res.send(injectSacredTags(html, divineMeta));
  } catch (error) { res.status(500).send('Divine error.'); }
});

app.use((req, res) => {
  res.redirect('/');
});

export default app;

if (process.env.NODE_ENV !== 'production') {
  app.listen(port, () => console.log(`Backend server listening on http://localhost:${port}`));
}