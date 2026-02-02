import { useState, useEffect } from 'react' // Removed useRef as iframe is gone
import './App.css'

interface Game {
  id: string;
  name: string;
  description: string; // Short description for list view
  fullDescription: string; // Detailed description for expanded view
  wasmPath: string; // Now points directly to the game's HTML path
  previewImageUrl: string; // URL for the preview image
}

function App() {
  const [games, setGames] = useState<Game[]>([]);
  const [selectedGameDetails, setSelectedGameDetails] = useState<Game | null>(null); // The game whose details are being viewed
  const [error, setError] = useState<string | null>(null);

  // Theme switching state
  const [isDarkMode, setIsDarkMode] = useState(() => {
    const savedTheme = localStorage.getItem('theme');
    return savedTheme === 'light' ? false : true;
  });

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

  const viewGameDetails = (game: Game) => {
    setSelectedGameDetails(game);
  };

  // Modified to redirect to the game's specific URL
  const loadAndRunWasm = (game: Game) => {
    window.location.href = game.wasmPath; // Redirect to the game's HTML file
  };

  const backToList = () => {
    setSelectedGameDetails(null);
    setError(null);
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

  // The App component now only renders the list or details view, not the game itself
  return (
    <div className="App">
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
  )
}

export default App