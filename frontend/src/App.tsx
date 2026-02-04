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

  // Keyboard Shortcuts (Phase 3)
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.key === 'Escape' && selectedGameDetails) {
        backToList();
      }
    };
    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [selectedGameDetails]);

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

  const shareManifestation = (game: Game) => {
    if (navigator.share) {
      navigator.share({
        title: `${game.name} | THE DIVINE CODE`,
        text: `Behold this manifestation of sacred logic: ${game.name}`,
        url: window.location.origin + game.wasmPath,
      });
    } else {
      navigator.clipboard.writeText(window.location.origin + game.wasmPath);
      alert('Manifestation link copied to clipboard.');
    }
  };

  const backToList = () => {
    setSelectedGameDetails(null);
    setError(null);
  };

  const Hero = () => (
    <section className="hero-section">
      <div className="hero-content">
        <h1>THE ETERNAL CODE</h1>
        <p className="hero-subtitle">
          "All things were made through Him, and without Him was not anything made that was made." 
          <br /><br />
          Explore high-performance digital manifestations where logic serves beauty, and every line of code is a pilgrimage toward the Infinite.
        </p>
        <div className="hero-cta">
          {games.length > 0 && (
            <button className="hero-button" onClick={() => viewGameDetails(games[0])}>
              EXPLORE THE CODEBASE
            </button>
          )}
        </div>
      </div>
    </section>
  );

  const ChronicleOfLight = () => (
    <section className="chronicle-section animate-in">
      <div className="section-header">
        <h2>CHRONICLE OF LIGHT</h2>
        <div className="divider"></div>
      </div>
      <div className="chronicle-container">
        {/* VOLUME IV */}
        <div className="chronicle-volume">
          <div className="volume-header">
            <span className="folder-icon">📂</span>
            <span className="volume-title">VOL_IV_GRACE_OF_UX</span>
            <span className="volume-date">FEB 2026</span>
          </div>
          <div className="volume-contents">
            <div className="log-entry">
              <span className="entry-icon">📄</span>
              <div className="entry-text">
                <strong>PHASE_3_SYNCHRONIZATION</strong>
                <p>The Sacred Shell Manifested. Global keyboard rituals (ESC) initialized. Fluid loading transitions added to all manifestations.</p>
              </div>
            </div>
            <div className="log-entry">
              <span className="entry-icon">📄</span>
              <div className="entry-text">
                <strong>DIVINE_VISION_&_SHARING</strong>
                <p>Mission statement established. Web Share API integrated for digital evangelization. High-shadow visual polish applied.</p>
              </div>
            </div>
          </div>
        </div>

        {/* VOLUME III */}
        <div className="chronicle-volume">
          <div className="volume-header">
            <span className="folder-icon">📂</span>
            <span className="volume-title">VOL_III_SACRED_IDENTITY</span>
            <span className="volume-date">FEB 2026</span>
          </div>
          <div className="volume-contents">
            <div className="log-entry">
              <span className="entry-icon">📄</span>
              <div className="entry-text">
                <strong>THE_CODERS_CROSS</strong>
                <p>High-quality Coder's Cross Favicon manifested. Identity shifted to THE DIVINE CODE. Brand synergy applied across the Sanctuary.</p>
              </div>
            </div>
            <div className="log-entry">
              <span className="entry-icon">📄</span>
              <div className="entry-text">
                <strong>GENESIS_OF_ASCENSION</strong>
                <p>Flagship manifestation renamed from 'Cursor' to 'ASCENSION'. All shadows of strife and death purified from the code.</p>
              </div>
            </div>
            <div className="log-entry">
              <span className="entry-icon">📄</span>
              <div className="entry-text">
                <strong>CATHOLIC_COPY_OVERHAUL</strong>
                <p>Entire platform re-framed around Catholic teachings and the Logos. Ad Majorem Dei Gloriam.</p>
              </div>
            </div>
          </div>
        </div>

        {/* VOLUME II */}
        <div className="chronicle-volume">
          <div className="volume-header">
            <span className="folder-icon">📂</span>
            <span className="volume-title">VOL_II_THE_WORD_SPREADS</span>
            <span className="volume-date">FEB 2026</span>
          </div>
          <div className="volume-contents">
            <div className="log-entry">
              <span className="entry-icon">📄</span>
              <div className="entry-text">
                <strong>SEO_&_METADATA_INJECTION</strong>
                <p>Dynamic Sitemap and Robots.txt manifested. Dynamic SEO tag injection logic built for individual game manifestations.</p>
              </div>
            </div>
            <div className="log-entry">
              <span className="entry-icon">📄</span>
              <div className="entry-text">
                <strong>ANALYTICS_REFINEMENT</strong>
                <p>Google Analytics tag successfully integrated through secure COEP/COOP headers. Traffic tracking initialized.</p>
              </div>
            </div>
          </div>
        </div>

        {/* VOLUME I */}
        <div className="chronicle-volume">
          <div className="volume-header">
            <span className="folder-icon">📂</span>
            <span className="volume-title">VOL_I_GENESIS</span>
            <span className="volume-date">FEB 2026</span>
          </div>
          <div className="volume-contents">
            <div className="log-entry">
              <span className="entry-icon">📄</span>
              <div className="entry-text">
                <strong>MVP_MANIFESTATION</strong>
                <p>Core WebAssembly loading engine established. Dark/Light mode theme engine built. Initial game artifacts uploaded.</p>
              </div>
            </div>
            <div className="log-entry">
              <span className="entry-icon">📄</span>
              <div className="entry-text">
                <strong>STYLE_EVOLUTION</strong>
                <p>Transitioned to modern iOS-inspired bold minimalism. Full-screen manifestation support added.</p>
              </div>
            </div>
          </div>
        </div>
      </div>
    </section>
  );

  const MissionStatement = () => (
    <section className="mission-section animate-in">
      <div className="mission-content">
        <div className="section-header">
          <h2>THE DIVINE VISION</h2>
          <div className="divider"></div>
        </div>
        <div className="mission-grid">
          <div className="mission-item">
            <h3>LOGOS & LOGIC</h3>
            <p>We believe that code is not merely a tool for utility, but a reflection of the Divine Order. In the mathematical precision of a C++ algorithm, we catch a glimpse of the Creator's hand.</p>
          </div>
          <div className="mission-item">
            <h3>SACRED PLAY</h3>
            <p>Our manifestations are designed to be life-affirming. We strive for "Non-Violent Sophistication"—challenges that test the soul's fortitude and patience without resorting to the base instincts of strife.</p>
          </div>
          <div className="mission-item">
            <h3>ASCENSION</h3>
            <p>Every journey through our digital sanctuary is intended to be a step toward clarity. From the parry of a storm to the harmony of multithreaded logic, we seek the Infinite through the Finite.</p>
          </div>
        </div>
      </div>
    </section>
  );

  const renderGameList = () => (
    <div className="game-list">
      <div className="section-header">
        <h2>SACRED LOGIC</h2>
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
                   <button className="card-play-button" onClick={(e) => { e.stopPropagation(); loadAndRunWasm(game); }}>ASCEND</button>
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
        <button className="back-button-top" onClick={backToList}>← RETURN TO THE SANCTUARY (ESC)</button>
        <div className="title-share-row">
          <h2>{selectedGameDetails?.name}</h2>
          <button className="share-button" onClick={() => selectedGameDetails && shareManifestation(selectedGameDetails)}>
            <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"><path d="M4 12v8a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2v-8"/><polyline points="16 6 12 2 8 6"/><line x1="12" y1="2" x2="12" y2="15"/></svg>
            SHARE
          </button>
        </div>
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
            <h3>THE LOGIC OF THE WORD</h3>
            <div className="markdown-content">
              <ReactMarkdown>{selectedGameDetails?.fullDescription || ''}</ReactMarkdown>
            </div>
          </div>
          <div className="game-details-actions">
            <button className="action-play-button" onClick={() => selectedGameDetails && loadAndRunWasm(selectedGameDetails)}>INITIATE SACRED JOURNEY</button>
          </div>
        </div>
      </div>
    </div>
  );

  const CodexIcon = () => (
    <svg width="24" height="24" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg" style={{ marginRight: '12px' }}>
      <path d="M7 8L3 12L7 16" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"/>
      <path d="M17 8L21 12L17 16" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"/>
      <path d="M12 5V19" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round"/>
      <path d="M9 12H15" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round"/>
    </svg>
  );

  return (
    <div className="main-canvas">
      <header className="site-header">
        <div className="logo" onClick={backToList} style={{ display: 'flex', alignItems: 'center' }}>
          <CodexIcon />
          <span>THE DIVINE CODE</span>
        </div>
        <button className="theme-toggle-minimal" onClick={() => setIsDarkMode(!isDarkMode)}>
          {isDarkMode ? 'CLARITY' : 'OBSCURITY'}
        </button>
      </header>
      
      {error && <div className="error-banner">AWAITING CLARITY: {error}</div>}

      <main className="content-area">
        {!selectedGameDetails ? (
          <>
            <Hero />
            <div className="games-grid-wrapper">
              {renderGameList()}
            </div>
            <div className="home-secondary-content">
              <ChronicleOfLight />
              <MissionStatement />
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