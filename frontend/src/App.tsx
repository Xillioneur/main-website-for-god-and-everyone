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
        <h1>THE DIVINE LOGOS</h1>
        <p className="hero-subtitle">
          "All things were made through Him, and without Him was not anything made that was made." 
          <br /><br />
          Explore digital manifestations where logic serves beauty, and every line of code is a pilgrimage toward the Infinite.
        </p>
        <div className="hero-cta">
          {games.length > 0 && (
            <button className="hero-button" onClick={() => viewGameDetails(games[0])}>
              BEGIN THE PILGRIMAGE
            </button>
          )}
        </div>
      </div>
    </section>
  );

  const renderGameList = () => (
    <div className="game-list">
      <div className="section-header">
        <h2>SACRED MANIFESTATIONS</h2>
        <div className="divider"></div>
      </div>
      {games.length === 0 && !error ? (
        <p className="loading-text">Awaiting the Word to manifest in code...</p>
      ) : (
        <div className="game-cards-container">
          {games.map((game) => (
            <div key={game.id} className="game-card" onClick={() => viewGameDetails(game)}>
              <div className="card-image-wrapper">
                <img
                  src={game.previewImageUrl}
                  alt={`Visions of ${game.name}`}
                  className="game-preview-image"
                />
                <div className="card-overlay">
                   <button className="card-play-button" onClick={(e) => { e.stopPropagation(); loadAndRunWasm(game); }}>ENTER MYSTERY</button>
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
        <button className="back-button-top" onClick={backToList}>← RETURN TO THE CATHEDRAL</button>
        <h2>{selectedGameDetails?.name}</h2>
      </div>
      <div className="details-layout">
        <div className="details-image-container">
          <img
            src={selectedGameDetails?.previewImageUrl}
            alt={`Icon of ${selectedGameDetails?.name}`}
            className="game-details-image"
          />
        </div>
        <div className="details-info">
          <div className="details-edict">
            <h3>THE DOCTRINE OF THE CODE</h3>
            <div className="markdown-content">
              <ReactMarkdown>{selectedGameDetails?.fullDescription || ''}</ReactMarkdown>
            </div>
          </div>
          <div className="game-details-actions">
            <button className="action-play-button" onClick={() => selectedGameDetails && loadAndRunWasm(selectedGameDetails)}>INITIATE SACRED PLAY</button>
          </div>
        </div>
      </div>
    </div>
  );

  return (
    <div className="main-canvas">
      <header className="site-header">
        <div className="logo" onClick={backToList}>AD MAJOREM DEI GLORIAM</div>
        <button className="theme-toggle-minimal" onClick={() => setIsDarkMode(!isDarkMode)}>
          {isDarkMode ? 'CLARITY' : 'OBSCURITY'}
        </button>
      </header>
      
      {error && <div className="error-banner">TRIAL ENCOUNTERED: {error}</div>}

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
        <p>
          "But thou hast arranged all things by measure and number and weight." — Wisdom 11:20
          <br /><br />
          For the Greater Glory of God.
        </p>
      </footer>
    </div>
  )
}

export default App