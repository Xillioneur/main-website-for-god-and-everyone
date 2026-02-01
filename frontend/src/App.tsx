import { useState, useEffect, useRef } from 'react' // Re-added useRef
import './App.css'

interface Game {
  id: string;
  name: string;
  description: string;
  wasmPath: string;
}

function App() {
  const [games, setGames] = useState<Game[]>([]);
  const [selectedGame, setSelectedGame] = useState<Game | null>(null);
  const [gameOutput, setGameOutput] = useState<string>(''); // Re-introduced gameOutput
  const [loadingWasm, setLoadingWasm] = useState<boolean>(false); // Re-introduced loadingWasm
  const [error, setError] = useState<string | null>(null);
  const iframeRef = useRef<HTMLIFrameElement>(null); // Ref for the iframe

  // Fetch games on component mount
  useEffect(() => {
    const fetchGames = async () => {
      try {
        const response = await fetch('/api/games');
        if (!response.ok) {
          throw new Error(`HTTP error! status: ${response.status}`);
        }
        const data: Game[] = await response.json();
        setGames(data);
      } catch (e: any) {
        setError('Failed to fetch games: ' + e.message);
      }
    };
    fetchGames();
  }, []);

  // Effect to handle messages from the iframe
  useEffect(() => {
    const handleMessage = (event: MessageEvent) => {
      // Ensure messages are from the expected origin (your domain)
      // For development, '*' is fine, but in production, specify your domain
      if (event.origin !== window.location.origin && event.origin !== 'null') { // 'null' for local file/iframe
        // console.warn('Received message from unknown origin:', event.origin);
        // return;
      }

      const { type, payload } = event.data;

      switch (type) {
        case 'wasm_output':
          setGameOutput(prev => prev + payload + '\n');
          break;
        case 'wasm_error':
          setError(prev => (prev ? prev + '\n' : '') + 'WASM Error: ' + payload);
          break;
        case 'wasm_status':
          // We can use this to track loading progress if needed
          if (payload.includes('All downloads complete.')) {
            setLoadingWasm(false);
          } else if (payload.includes('Preparing...')) {
            setLoadingWasm(true);
          }
          break;
        case 'wasm_loaded': // Custom event from wasm_loader.html when script is loaded
          setLoadingWasm(false);
          // Auto-focus the iframe's contentWindow (which should focus the canvas)
          if (iframeRef.current && iframeRef.current.contentWindow) {
            iframeRef.current.contentWindow.focus();
          }
          break;
        default:
          console.log('Received unknown message from iframe:', event.data);
      }
    };

    window.addEventListener('message', handleMessage);

    return () => {
      window.removeEventListener('message', handleMessage);
    };
  }, []);


  const loadAndRunWasm = (game: Game) => {
    setSelectedGame(game);
    setLoadingWasm(true); // Set loading true when a game is selected
    setGameOutput('');
    setError(null);
  };

  return (
    <div className="App">
      <header className="App-header">
        <h1>WebAssembly Game Platform</h1>
      </header>

      {error && <p className="error-message">Error: {error}</p>}

      {!selectedGame ? (
        <div className="game-list">
          <h2>Available Games</h2>
          {games.length === 0 && !error ? (
            <p>Loading games...</p>
          ) : (
            <ul>
              {games.map((game) => (
                <li key={game.id}>
                  <h3>{game.name}</h3>
                  <p>{game.description}</p>
                  <button onClick={() => loadAndRunWasm(game)}>Play</button>
                </li>
              ))}
            </ul>
          )}
        </div>
      ) : (
        <div className="game-container">
          <h2>Now Playing: {selectedGame.name}</h2>
          <button onClick={() => setSelectedGame(null)}>Back to Game List</button>
          {loadingWasm && <p>Loading WebAssembly module for {selectedGame.name}...</p>}
          <div style={{ position: 'relative', width: '100%', height: 'calc(100vh - 150px)', border: 'none' }}>
            <iframe
              ref={iframeRef} // Attach ref to iframe
              key={selectedGame.id} // Key ensures iframe remounts when game changes
              src={`/wasm_loader.html?gameUrl=${selectedGame.wasmPath}`}
              style={{ width: '100%', height: '100%', border: 'none' }}
              allowFullScreen={true}
              // Add sandbox attributes for security if needed
            ></iframe>
          </div>
          {selectedGame.id !== 'sdl2-example' && ( // Display text output for non-SDL games
            <div>
              <h3>Game Output:</h3>
              <pre>{gameOutput || 'No output from game.'}</pre>
            </div>
          )}
        </div>
      )}
    </div>
  )
}

export default App