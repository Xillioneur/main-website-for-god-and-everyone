import { useState, useEffect, useRef } from 'react'
import ReactMarkdown from 'react-markdown'
import { Prism as SyntaxHighlighter } from 'react-syntax-highlighter'
import { vscDarkPlus, prism } from 'react-syntax-highlighter/dist/esm/styles/prism'
import './App.css'

interface Game {
  id: string;
  name: string;
  virtue: string;
  type: 'MANIFESTATION' | 'FOUNDATION';
  description: string;
  fullDescription: string;
  logicSnippet: string;
  wasmPath: string;
  previewImageUrl: string;
}

interface DivineStats {
  atomicWeight: number;
  manifestations: number;
  foundations: number;
  communion: string;
  status: string;
}

// --- STATIC COMPONENTS (Defined outside to prevent remounting on scroll) ---

const CodexIcon = () => (
  <svg width="24" height="24" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg" style={{ marginRight: '12px' }}>
    <path d="M7 8L3 12L7 16" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"/>
    <path d="M17 8L21 12L17 16" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"/>
    <path d="M12 5V19" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round"/>
    <path d="M9 12H15" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round"/>
  </svg>
);

const ChronicleOfLight = () => (
  <section className="chronicle-section animate-in">
    <div className="section-header">
      <h2>CHRONICLE OF LIGHT</h2>
      <div className="divider"></div>
    </div>
    <div className="chronicle-list">
      <div className="update-entry">
        <div className="update-meta">
          <span className="update-date">FEB 05</span>
          <div className="update-dot"></div>
        </div>
        <div className="update-content">
          <h4>DIVINE RECKONING</h4>
          <p>Our flagship Soulslike manifestation—a brutal journey of grace and redemption.</p>
        </div>
      </div>

      <div className="update-entry">
        <div className="update-meta">
          <span className="update-date">FEB 04</span>
          <div className="update-dot"></div>
        </div>
        <div className="update-content">
          <h4>ASHES OF THE SCROLL</h4>
          <p>A 3D pilgrimage through digital landscapes of intention and peace.</p>
        </div>
      </div>

      <div className="update-entry">
        <div className="update-meta">
          <span className="update-date">FEB 04</span>
          <div className="update-dot"></div>
        </div>
        <div className="update-content">
          <h4>THE LIGHT OF LOGIC</h4>
          <p>A meditation on harmony, with a thousand threads working in digital unity.</p>
        </div>
      </div>

      <div className="update-entry">
        <div className="update-meta">
          <span className="update-date">FEB 03</span>
          <div className="update-dot"></div>
        </div>
        <div className="update-content">
          <h4>ASHES OF THE BULLET</h4>
          <p>Mastering the rite of stillness within a storm of movement.</p>
        </div>
      </div>

      <div className="update-entry">
        <div className="update-meta">
          <span className="update-date">FEB 02</span>
          <div className="update-dot"></div>
        </div>
        <div className="update-content">
          <h4>THE ECHO OF WILL</h4>
          <p>A technical study in interaction and event-driven responsive harmony.</p>
        </div>
      </div>

      <div className="update-entry">
        <div className="update-meta">
          <span className="update-date">FEB 02</span>
          <div className="update-dot"></div>
        </div>
        <div className="update-content">
          <h4>THE GEOMETRY OF TRUTH</h4>
          <p>Foundational laws of visual structure manifested through Raylib.</p>
        </div>
      </div>

      <div className="update-entry">
        <div className="update-meta">
          <span className="update-date">FEB 01</span>
          <div className="update-dot"></div>
        </div>
        <div className="update-content">
          <h4>INITIATION</h4>
          <p>The digital genesis—the first Word manifests within the void.</p>
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

function App() {
  const [games, setGames] = useState<Game[]>([]);
  const [filteredGames, setFilteredGames] = useState<Game[]>([]);
  const [foundations, setFoundations] = useState<Game[]>([]);
  const [stats, setStats] = useState<DivineStats | null>(null);
  const [selectedGameDetails, setSelectedGameDetails] = useState<Game | null>(null);
  const [activeVirtue, setActiveVirtue] = useState<string>('ALL');
  const [error, setError] = useState<string | null>(null);
  const [showHelp, setShowHelp] = useState(false);
  
  const progressBarRef = useRef<HTMLDivElement>(null);

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

  // Phase 11: Optimized Scroll Tracking
  useEffect(() => {
    let requestRunning = false;
    const handleScroll = () => {
      if (!requestRunning) {
        requestRunning = true;
        requestAnimationFrame(() => {
          const totalScroll = document.documentElement.scrollHeight - window.innerHeight;
          const currentProgress = totalScroll > 0 ? (window.pageYOffset / totalScroll) * 100 : 0;
          if (progressBarRef.current) {
            progressBarRef.current.style.width = `${currentProgress}%`;
          }
          requestRunning = false;
        });
      }
    };
    window.addEventListener('scroll', handleScroll);
    return () => window.removeEventListener('scroll', handleScroll);
  }, []);

  // Phase 4 & 11: Navigation & Shortcuts
  useEffect(() => {
    const handlePopState = () => {
      const params = new URLSearchParams(window.location.search);
      const gameId = params.get('manifest');
      if (gameId && games.length > 0) {
        const game = games.concat(foundations).find(g => g.id === gameId);
        if (game) setSelectedGameDetails(game);
        else setSelectedGameDetails(null);
      } else {
        setSelectedGameDetails(null);
      }
    };

    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.key === 'Escape') backToList();
      if (e.key === '?' || e.key === '/') {
        if (!selectedGameDetails) setShowHelp(prev => !prev);
      }
    };

    window.addEventListener('popstate', handlePopState);
    window.addEventListener('keydown', handleKeyDown);
    
    return () => {
      window.removeEventListener('popstate', handlePopState);
      window.removeEventListener('keydown', handleKeyDown);
    };
  }, [games, foundations, selectedGameDetails]);

  useEffect(() => {
    const fetchData = async () => {
      try {
        setError(null);
        const [gamesRes, statsRes] = await Promise.all([
          fetch('/api/games'),
          fetch('/api/stats')
        ]);
        
        if (!gamesRes.ok) throw new Error('Interrupt');
        const gamesData: Game[] = await gamesRes.json();
        setGames(gamesData.filter(g => g.type === 'MANIFESTATION'));
        setFoundations(gamesData.filter(g => g.type === 'FOUNDATION'));
        setFilteredGames(gamesData.filter(g => g.type === 'MANIFESTATION'));

        if (statsRes.ok) {
          const statsData = await statsRes.json();
          setStats(statsData);
        }
      } catch (e: any) {
        setError('A momentary cloud has passed over the connection.');
      }
    };
    fetchData();
  }, []);

  useEffect(() => {
    if (activeVirtue === 'ALL') {
      setFilteredGames(games);
    } else {
      setFilteredGames(games.filter(g => g.virtue === activeVirtue));
    }
  }, [activeVirtue, games]);

  const viewGameDetails = (game: Game) => {
    setSelectedGameDetails(game);
    const url = new URL(window.location.href);
    url.searchParams.set('manifest', game.id);
    window.history.pushState({ gameId: game.id }, '', url);
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
    const url = new URL(window.location.href);
    url.searchParams.delete('manifest');
    window.history.pushState({}, '', url);
    setError(null);
    setShowHelp(false);
  };

  // --- RENDER HELPERS ---

  const renderHero = () => (
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
            <div className="latest-highlight">
              <span className="highlight-tag">LATEST MANIFESTATION</span>
              <button className="hero-button" onClick={() => viewGameDetails(games[0])}>
                EXPLORE {games[0].name.toUpperCase()}
              </button>
            </div>
          )}
        </div>
      </div>
    </section>
  );

  const renderDivineCensus = () => (
    <section className="census-section">
      <div className="census-grid">
        <div className="census-item">
          <span className="census-label">MANIFESTATIONS</span>
          <span className="census-value">{stats?.manifestations || '4'}</span>
        </div>
        <div className="census-item">
          <span className="census-label">FOUNDATIONS</span>
          <span className="census-value">{stats?.foundations || '3'}</span>
        </div>
        <div className="census-item">
          <span className="census-label">ATOMIC WEIGHT (LOC)</span>
          <span className="census-value">{stats?.atomicWeight || '8469'}</span>
        </div>
        <div className="census-item">
          <span className="census-label">SACRED COMMUNION</span>
          <a href="https://x.com/liwawil" target="_blank" rel="noopener noreferrer" className="census-value link-value">
            {stats?.communion || '@liwawil'}
          </a>
        </div>
        <div className="census-item">
          <span className="census-label">SANCTUARY STATUS</span>
          <span className="census-value status-glow">{stats?.status || 'SANCTIFIED'}</span>
        </div>
      </div>
    </section>
  );

  const renderCommandmentRitual = () => (
    <div className={`help-overlay ${showHelp ? 'visible' : ''}`} onClick={() => setShowHelp(false)}>
      <div className="help-content" onClick={e => e.stopPropagation()}>
        <h2>SACRED RITUALS</h2>
        <div className="ritual-grid">
          <div className="ritual-item"><span>ESC</span> <p>RETURN TO SANCTUARY</p></div>
          <div className="ritual-item"><span>?</span> <p>TOGGLE COMMANDMENTS</p></div>
          <div className="ritual-item"><span>Q</span> <p>INVOKE PRAYER / PARRY</p></div>
          <div className="ritual-item"><span>WASD</span> <p>NAVIGATE THE VOID</p></div>
          <div className="ritual-item"><span>SPACE</span> <p>DODGE ROLL / ASCEND</p></div>
        </div>
        <button className="close-help" onClick={() => setShowHelp(false)}>UNDERSTOOD</button>
      </div>
    </div>
  );

  const renderVirtueFilter = () => {
    const virtues = ['ALL', 'REDEMPTION', 'CLARITY', 'FORTITUDE', 'TEMPERANCE'];
    return (
      <div className="virtue-filter">
        {virtues.map(v => (
          <button 
            key={v} 
            className={`filter-btn ${activeVirtue === v ? 'active' : ''}`}
            onClick={() => setActiveVirtue(v)}
          >
            {v}
          </button>
        ))}
      </div>
    );
  };

  const renderGameList = () => (
    <div className="game-list">
      <div className="section-header">
        <h2>SACRED LOGIC</h2>
        <div className="divider"></div>
      </div>
      
      {renderVirtueFilter()}

      {filteredGames.length === 0 && !error ? (
        <p className="loading-text">Awaiting the Word to manifest in code...</p>
      ) : (
        <div className="game-cards-container">
          {filteredGames.map((game) => (
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
                <div className="card-virtue-tag">{game.virtue}</div>
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

  const renderTechnicalFoundations = () => (
    <div className="foundations-section animate-in">
      <div className="section-header">
        <h2>TECHNICAL FOUNDATIONS</h2>
        <div className="divider"></div>
      </div>
      <p className="section-subtitle">Dedicated manifestations for geeks and developers exploring the atomic order of the Divine Code.</p>
      
      <div className="foundations-grid">
        {foundations.map((item) => (
          <div key={item.id} className="foundation-card" onClick={() => viewGameDetails(item)}>
            <div className="foundation-header">
              <span className="foundation-virtue">{item.virtue}</span>
              <h3>{item.name}</h3>
            </div>
            <p>{item.description}</p>
            <button className="foundation-view-btn">GATHER FRAGMENTS</button>
          </div>
        ))}
      </div>
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

      {selectedGameDetails?.logicSnippet && (
        <div className="sacred-script-section">
          <h3>FRAGMENTS OF LOGIC (C++)</h3>
          <div className="code-block">
            <SyntaxHighlighter 
              language="cpp" 
              style={isDarkMode ? vscDarkPlus : prism}
              customStyle={{
                background: 'transparent',
                padding: '0',
                margin: '0',
                fontSize: '0.9rem'
              }}
            >
              {selectedGameDetails.logicSnippet}
            </SyntaxHighlighter>
          </div>
        </div>
      )}
    </div>
  );

  return (
    <div className="main-canvas">
      <div className="scroll-progress-container">
        <div className="scroll-progress-bar" ref={progressBarRef}></div>
      </div>

      <header className="site-header">
        <div className="logo" onClick={backToList} style={{ display: 'flex', alignItems: 'center' }}>
          <CodexIcon />
          <span>THE DIVINE CODE</span>
        </div>
        <div className="header-actions">
          <button className="help-trigger" onClick={() => setShowHelp(true)}>PROTOCOLS (?)</button>
          <button className="theme-toggle-minimal" onClick={() => setIsDarkMode(!isDarkMode)}>
            {isDarkMode ? 'CLARITY' : 'OBSCURITY'}
          </button>
        </div>
      </header>
      
      {error && (
        <div className="error-banner">
          <span>{error}</span>
          <button onClick={() => window.location.reload()} className="retry-button">INVOKE AGAIN</button>
        </div>
      )}

      <main className="content-area">
        {!selectedGameDetails ? (
          <>
            {renderHero()}
            <div className="games-grid-wrapper">
              {renderGameList()}
              {renderDivineCensus()}
              {renderTechnicalFoundations()}
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

      {renderCommandmentRitual()}

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