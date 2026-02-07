import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const gamesRoot = path.resolve(__dirname, '../games');
const censusPath = path.resolve(__dirname, '../backend/census.json');

async function generateCensus() {
    console.log('--- INITIATING DIVINE CENSUS (BUILD-TIME) ---');
    
    let totalLoc = 0;
    let manifestations = 0;
    let foundations = 0;
    const extensions = ['.cpp', '.h', '.hpp'];
    const techFoundations = ['hello', 'raylib_example', 'sdl2_example'];

    const walkSync = (dir) => {
        if (!fs.existsSync(dir)) return;
        const files = fs.readdirSync(dir);
        files.forEach((file) => {
            const filePath = path.join(dir, file);
            if (fs.statSync(filePath).isDirectory()) {
                walkSync(filePath);
            } else if (extensions.some(ext => filePath.endsWith(ext))) {
                const content = fs.readFileSync(filePath, 'utf8');
                const lines = content.split('\n').length;
                totalLoc += lines;
            }
        });
    };

    if (fs.existsSync(gamesRoot)) {
        walkSync(gamesRoot);
        
        const items = fs.readdirSync(gamesRoot, { withFileTypes: true });
        for (const item of items) {
            if (item.isDirectory()) {
                const hasHtml = fs.existsSync(path.join(gamesRoot, item.name, `${item.name}.html`));
                if (hasHtml) {
                    if (techFoundations.includes(item.name)) foundations++;
                    else manifestations++;
                }
            }
        }
    }

    const censusData = {
        atomicWeight: totalLoc,
        manifestations: manifestations,
        foundations: foundations,
        status: "SANCTIFIED",
        timestamp: new Date().toISOString()
    };

    fs.writeFileSync(censusPath, JSON.stringify(censusData, null, 2));
    console.log(`Census Manifested: ${totalLoc} LOC across ${manifestations + foundations} titles.`);
    console.log('--- DIVINE CENSUS COMPLETE ---');
}

generateCensus().catch(err => {
    console.error('Census Error:', err);
    process.exit(1);
});