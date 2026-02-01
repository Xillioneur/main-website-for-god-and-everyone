import { useState, useEffect, useRef } from 'react'
import './App.css'

interface Game {
  id: string;
  name: string;
  description: string; // Short description for list view
  fullDescription: string; // Detailed description for expanded view
  wasmPath: string;
  previewImageUrl: string; // URL for the preview image
}

function App() {
  const [games, setGames] = useState<Game[]>([]);
  const [selectedGame, setSelectedGame] = useState<Game | null>(null); // The game currently being played
  const [selectedGameDetails, setSelectedGameDetails] = useState<Game | null>(null); // The game whose details are being viewed
  const [gameOutput, setGameOutput] = useState<string>('');
  const [loadingWasm, setLoadingWasm] = useState<boolean>(false);
  const [error, setError] = useState<string | null>(null);
  const iframeRef = useRef<HTMLIFrameElement>(null);

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

  useEffect(() => {
    const handleMessage = (event: MessageEvent) => {
      if (event.origin !== window.location.origin && event.origin !== 'null') {
        return;
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
          if (payload.includes('All downloads complete.')) {
            setLoadingWasm(false);
          } else if (payload.includes('Preparing...')) {
            setLoadingWasm(true);
          }
          break;
        case 'wasm_loaded':
          setLoadingWasm(false);
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

  const viewGameDetails = (game: Game) => {
    setSelectedGameDetails(game);
  };

  const loadAndRunWasm = (game: Game) => {
    setSelectedGame(game);
    setSelectedGameDetails(null); // Clear details view
    setLoadingWasm(true);
    setGameOutput('');
    setError(null);
  };

  const backToList = () => {
    setSelectedGame(null);
    setSelectedGameDetails(null);
    setLoadingWasm(false);
    setError(null);
    setGameOutput('');
  };

  const renderGameList = () => (
    <div className="game-list">
      <h2>Available Games</h2>
      {games.length === 0 && !error ? (
        <p>Loading games...</p>
      ) : (
        <div className="game-cards-container">
          {games.map((game) => (
            <div key={game.id} className="game-card">
              <img
                src={game.previewImageUrl}
                alt={`Preview of ${game.name}`}
                className="game-preview-image"
                onClick={() => viewGameDetails(game)}
              />
              <h3>{game.name}</h3>
              <p>{game.description}</p>
              <button onClick={() => viewGameDetails(game)}>Details</button>
            </div>
          ))}
        </div>
      )}
    </div>
  );

  const renderGameDetails = () => (
    <div className="game-details-view">
      <h2>{selectedGameDetails?.name}</h2>
      <img
        src={selectedGameDetails?.previewImageUrl}
        alt={`Preview of ${selectedGameDetails?.name}`}
        className="game-details-image"
      />
      <p>{selectedGameDetails?.fullDescription}</p>
      <div className="game-details-actions">
        <button onClick={() => selectedGameDetails && loadAndRunWasm(selectedGameDetails)}>Play Now</button>
        <button onClick={backToList}>Back to List</button>
      </div>
    </div>
  );

  const renderPlayingGame = () => (
    <div className="game-container">
      <h2>Now Playing: {selectedGame?.name}</h2>
      <button onClick={backToList}>Back to Game List</button>
      {loadingWasm && <p>Loading WebAssembly module for {selectedGame?.name}...</p>}
      <div style={{ position: 'relative', width: '100%', height: 'calc(100vh - 150px)', border: 'none' }}>
        <iframe
          ref={iframeRef}
          key={selectedGame?.id}
          src={`/wasm_loader.html?gameUrl=${selectedGame?.wasmPath}`}
          style={{ width: '100%', height: '100%', border: 'none' }}
          allowFullScreen={true}
        ></iframe>
      </div>
      {selectedGame?.id !== 'sdl2-example' && (
        <div>
          <h3>Game Output:</h3>
          <pre>{gameOutput || 'No output from game.'}</pre>
        </div>
      )}
    </div>
  );


  return (
    <div className="App">
      <header className="App-header">
        <h1>WebAssembly Game Platform</h1>
      </header>

      {error && <p className="error-message">Error: {error}</p>}

      {!selectedGame && !selectedGameDetails && renderGameList()}
      {!selectedGame && selectedGameDetails && renderGameDetails()}
      {selectedGame && renderPlayingGame()}
    </div>
  )
}

export default App