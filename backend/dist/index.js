"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const express_1 = __importDefault(require("express"));
const path_1 = __importDefault(require("path"));
const app = (0, express_1.default)();
const port = process.env.PORT || 3000;
// API routes
app.get('/api', (req, res) => {
    res.json({ message: 'API is working!' });
});
app.get('/api/games', (req, res) => {
    const games = [
        {
            id: 'hello-wasm',
            name: 'Hello WebAssembly',
            description: 'A simple C++ program compiled to WebAssembly that prints a greeting.',
            wasmPath: '/wasm/hello.js', // Path to Emscripten-generated JS glue code
        },
        {
            id: 'sdl2-example',
            name: 'SDL2 WebAssembly Example',
            description: 'A simple C++ SDL2 application showing a red rectangle on a canvas.',
            wasmPath: '/wasm/sdl2_example.js', // Path to Emscripten-generated JS glue code
        },
        {
            id: 'raylib-example',
            name: 'raylib WebAssembly Example',
            description: 'A simple C++ raylib application showing a moving circle and color changes.',
            wasmPath: '/wasm/raylib_example.js', // Path to Emscripten-generated JS glue code
        },
        // Add more games here in the future
    ];
    res.json(games);
});
// Explicitly serve WASM glue code and WASM modules with correct MIME types
app.get('/wasm/hello.js', (req, res) => {
    res.type('application/javascript').sendFile(path_1.default.join(__dirname, '../../frontend/dist/wasm/hello.js'));
});
app.get('/wasm/hello.wasm', (req, res) => {
    res.type('application/wasm').sendFile(path_1.default.join(__dirname, '../../frontend/dist/wasm/hello.wasm'));
});
app.get('/wasm/sdl2_example.js', (req, res) => {
    res.type('application/javascript').sendFile(path_1.default.join(__dirname, '../../frontend/dist/wasm/sdl2_example.js'));
});
app.get('/wasm/sdl2_example.wasm', (req, res) => {
    res.type('application/wasm').sendFile(path_1.default.join(__dirname, '../../frontend/dist/wasm/sdl2_example.wasm'));
});
app.get('/wasm/raylib_example.js', (req, res) => {
    res.type('application/javascript').sendFile(path_1.default.join(__dirname, '../../frontend/dist/wasm/raylib_example.js'));
});
app.get('/wasm/raylib_example.wasm', (req, res) => {
    res.type('application/wasm').sendFile(path_1.default.join(__dirname, '../../frontend/dist/wasm/raylib_example.wasm'));
});
// Serve other static files from the React app
app.use(express_1.default.static(path_1.default.join(__dirname, '../../frontend/dist')));
// All other unhandled requests will return the React app's index.html
app.use((req, res) => {
    res.sendFile(path_1.default.join(__dirname, '../../frontend/dist/index.html'));
});
app.listen(port, () => {
    console.log(`Backend server listening on http://localhost:${port}`);
});
