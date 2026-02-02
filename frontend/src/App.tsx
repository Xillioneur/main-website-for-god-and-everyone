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

  // Theme switching state
  const [isDarkMode, setIsDarkMode] = useState(() => {
    // Initialize from localStorage or default to dark mode
    const savedTheme = localStorage.getItem('theme');
    return savedTheme === 'light' ? false : true;
  });

  // Track fullscreen state
  const [isFullscreen, setIsFullscreen] = useState(false);

  // Effect to apply theme class to body
  useEffect(() => {
    if (isDarkMode) {
      document.body.classList.remove('light-mode');
      localStorage.setItem('theme', 'dark');
    } else {
      document.body.classList.add('light-mode');
      localStorage.setItem('theme', 'light');
    }
  }, [isDarkMode]);

  // Effect to handle fullscreen changes
  useEffect(() => {
    const handleFullscreenChange = () => {
      setIsFullscreen(!!document.fullscreenElement);
    };

    document.addEventListener('fullscreenchange', handleFullscreenChange);
    document.addEventListener('webkitfullscreenchange', handleFullscreenChange);
    document.addEventListener('mozfullscreenchange', handleFullscreenChange);
    document.addEventListener('MSFullscreenChange', handleFullscreenChange);

    return () => {
      document.removeEventListener('fullscreenchange', handleFullscreenChange);
      document.removeEventListener('webkitfullscreenchange', handleFullscreenChange);
      document.removeEventListener('mozfullscreenchange', handleFullscreenChange);
      document.removeEventListener('MSFullscreenChange', handleFullscreenChange);
    };
  }, []);

  const toggleFullscreen = () => {
    if (iframeRef.current) {
      if (!document.fullscreenElement) {
        iframeRef.current.requestFullscreen().catch(err => {
          console.error(`Error attempting to enable full-screen mode: ${err.message} (${err.name})`);
          setError(`Fullscreen not supported or allowed by your browser. Error: ${err.message}`);
        });
      } else {
        document.exitFullscreen();
      }
    }
  };

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
      <h2>AVAILABLE MANIFESTATIONS</h2>
      {games.length === 0 && !error ? (
        <p>Awaiting divine revelations of code...</p>
      ) : (
        <div className="game-cards-container">
          {games.map((game) => (
            <div key={game.id} className="game-card" onClick={() => viewGameDetails(game)}>
              <img
                src={game.previewImageUrl}
                alt={`Preview of ${game.name}`}
                className="game-preview-image"
              />
              <h3>{game.name}</h3>
              <p>{game.description}</p>
              <button onClick={(e) => { e.stopPropagation(); loadAndRunWasm(game); }}>INITIATE DIVINE PLAY</button>
            </div>
          ))}
        </div>
      )}
    </div>
  );

  const renderGameDetails = () => (
    <div className="game-details-view">
      <h2>{selectedGameDetails?.name} - DIVINE EDICT</h2>
      <img
        src={selectedGameDetails?.previewImageUrl}
        alt={`Preview of ${selectedGameDetails?.name}`}
        className="game-details-image"
      />
      <p>{selectedGameDetails?.fullDescription}</p>
      <div className="game-details-actions">
        <button onClick={() => selectedGameDetails && loadAndRunWasm(selectedGameDetails)}>INITIATE DIVINE PLAY</button>
        <button onClick={backToList}>RETURN TO THE SANCTUARY</button>
      </div>
    </div>
  );

  const renderPlayingGame = () => (
    <div className="game-playing-container">
      {/* Game controls will now overlay or be minimal */}
      <div className="game-controls-overlay">
        <button onClick={backToList} className="control-button">BACK TO THE CREATION</button>
        <button onClick={toggleFullscreen} className="control-button">
          {isFullscreen ? 'EXIT FULLSCREEN' : 'ENTER THE VOID'}
        </button>
      </div>
      {loadingWasm && <p className="loading-message">Summoning {selectedGame?.name} from the aether...</p>}
      <div className="iframe-wrapper">
        <iframe
          ref={iframeRef}
          key={selectedGame?.id}
          src={`/wasm_loader.html?gameUrl=${selectedGame?.wasmPath}`}
          style={{ width: '100%', height: '100%', border: 'none' }}
          allowFullScreen={true}
        ></iframe>
      </div>
      {selectedGame?.id !== 'sdl2-example' && (
        <div className="game-output-console">
          <h3>ORACULAR TRACERIES:</h3>
          <pre>{gameOutput || 'Awaiting divine whispers from the depths...'}</pre>
        </div>
      )}
    </div>
  );


  return (
    <>
      {selectedGame ? (
        <div className="App playing-game">
          {renderPlayingGame()}
        </div>
      ) : (
        <div className="App" >
          <header className="App-header">
            <h1>THE DIVINE CODE: WASM MANIFESTATIONS</h1>
            <button className="theme-toggle-button" onClick={() => setIsDarkMode(!isDarkMode)}>
              {isDarkMode ? 'LIGHT MODE' : 'DARK MODE'}
            </button>
          </header>
          {error && <p className="error-message">MANIFESTATION ERROR: {error}</p>}
          {!selectedGameDetails && renderGameList()}
          {selectedGameDetails && renderGameDetails()}
        </div>
      )}
    </>
  )
}

export default App