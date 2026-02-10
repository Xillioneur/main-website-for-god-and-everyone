try {
    const backend = require('../backend/dist/index.js');
    module.exports = backend.default || backend;
} catch (error) {
    console.error('CRITICAL: Failed to load backend logic.', error);
    module.exports = (req, res) => {
        res.status(500).json({ 
            error: 'The Divine Code is temporarily unreachable.', 
            details: error.message,
            stack: error.stack,
            path: __dirname,
            cwd: process.cwd()
        });
    };
}