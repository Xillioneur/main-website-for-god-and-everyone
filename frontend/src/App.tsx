import { useState, useEffect } from 'react'
import ReactMarkdown from 'react-markdown'
import './App.css'

interface Game {
  id: string;
  name: string;
  description: string;
  fullDescription: string;
  wasmPath: string;
  previewImageUrl: string;
}

function App() {
  const [games, setGames] = useState<Game[]>([]);
  const [selectedGameDetails, setSelectedGameDetails] = useState<Game | null>(null);
  const [error, setError] = useState<string | null>(null);

  const [isDarkMode, setIsDarkMode] = useState(() => {
    const savedTheme = localStorage.getItem('theme');
    return savedTheme === 'light' ? false : true;
  });

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
    window.scrollTo({ top: 0, behavior: 'smooth' });
  };

  const loadAndRunWasm = (game: Game) => {
    window.location.href = game.wasmPath;
  };

  const backToList = () => {
    setSelectedGameDetails(null);
    setError(null);
  };

  const Hero = () => (
    <section className="hero-section">
      <div className="hero-content">
        <h1>THE DIVINE ARCHITECT'S PLAYGROUND</h1>
        <p className="hero-subtitle">BEHOLD THE PIXELS OF CREATION. BORN FROM THE VOID, MANIFESTED BY THE GRACE OF THE ALMIGHTY CODE.</p>
        <div className="hero-cta">
          {games.length > 0 && (
            <button className="hero-button" onClick={() => viewGameDetails(games[0])}>
              WITNESS THE LATEST MANIFESTATION
            </button>
          )}
        </div>
      </div>
    </section>
  );

  const renderGameList = () => (
    <div className="game-list">
      <div className="section-header">
        <h2>SACRED ARTIFACTS</h2>
        <div className="divider"></div>
      </div>
      {games.length === 0 && !error ? (
        <p className="loading-text">Awaiting divine revelations of code...</p>
      ) : (
        <div className="game-cards-container">
          {games.map((game) => (
            <div key={game.id} className="game-card" onClick={() => viewGameDetails(game)}>
              <div className="card-image-wrapper">
                <img
                  src={game.previewImageUrl}
                  alt={`Preview of ${game.name}`}
                  className="game-preview-image"
                />
                <div className="card-overlay">
                   <button className="card-play-button" onClick={(e) => { e.stopPropagation(); loadAndRunWasm(game); }}>PLAY</button>
                </div>
              </div>
              <div className="card-content">
                <h3>{game.name}</h3>
                <p>{game.description}</p>
              </div>
            </div>
          ))}
        </div>
      )}
    </div>
  );

  const renderGameDetails = () => (
    <div className="game-details-view animate-in">
      <div className="details-header">
        <button className="back-button-top" onClick={backToList}>← RETURN TO THE SANCTUARY</button>
        <h2>{selectedGameDetails?.name}</h2>
      </div>
      <div className="details-layout">
        <div className="details-image-container">
          <img
            src={selectedGameDetails?.previewImageUrl}
            alt={`Preview of ${selectedGameDetails?.name}`}
            className="game-details-image"
          />
        </div>
        <div className="details-info">
          <div className="details-edict">
            <h3>DIVINE EDICT</h3>
            <div className="markdown-content">
              <ReactMarkdown>{selectedGameDetails?.fullDescription || ''}</ReactMarkdown>
            </div>
          </div>
          <div className="game-details-actions">
            <button className="action-play-button" onClick={() => selectedGameDetails && loadAndRunWasm(selectedGameDetails)}>INITIATE DIVINE PLAY</button>
          </div>
        </div>
      </div>
    </div>
  );

  return (
    <div className="main-canvas">
      <header className="site-header">
        <div className="logo" onClick={backToList}>DIVINE CODE</div>
        <button className="theme-toggle-minimal" onClick={() => setIsDarkMode(!isDarkMode)}>
          {isDarkMode ? 'LIGHT' : 'DARK'}
        </button>
      </header>
      
      {error && <div className="error-banner">MANIFESTATION ERROR: {error}</div>}

      <main className="content-area">
        {!selectedGameDetails ? (
          <>
            <Hero />
            <div className="games-grid-wrapper">
              {renderGameList()}
            </div>
          </>
        ) : (
          renderGameDetails()
        )}
      </main>

      <footer className="site-footer">
        <p>IN CODE WE TRUST. ALL GLORY TO THE CREATOR.</p>
      </footer>
    </div>
  )
}

export default App