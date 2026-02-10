import { useState, useEffect, useRef } from 'react'
import ReactMarkdown from 'react-markdown'
import { Prism as SyntaxHighlighter } from 'react-syntax-highlighter'
import { vscDarkPlus, prism } from 'react-syntax-highlighter/dist/esm/styles/prism'
import Editor from '@monaco-editor/react'
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

const renderChronicleOfLight = () => (
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
          <h4>THE DIVINE RECKONING</h4>
          <p>Our flagship third-person Soulslike manifestation was released today. It represents a significant evolution in our technical ability, featuring complex projectile geometries and a deep narrative meditation on the ritual of redemption through digital grace.</p>
        </div>
      </div>

      <div className="update-entry">
        <div className="update-meta">
          <span className="update-date">FEB 04</span>
          <div className="update-dot"></div>
        </div>
        <div className="update-content">
          <h4>ASHES OF THE SCROLL</h4>
          <p>The first 3D landscape of the Divine Codebase has been manifested. Seekers are invited to navigate a world built on intentional movement and modular logic, proving that beauty and performance can coexist in a high-fidelity WebAssembly environment.</p>
        </div>
      </div>

      <div className="update-entry">
        <div className="update-meta">
          <span className="update-date">FEB 04</span>
          <div className="update-dot"></div>
        </div>
        <div className="update-content">
          <h4>ASCENSION: THE LIGHT OF LOGIC</h4>
          <p>This manifestation pushed the boundaries of parallel computation within the browser. Utilizing a custom multithreaded architecture, it allows thousands of logical entities to operate in perfect harmony, reflecting the interconnected nature of Divine Design.</p>
        </div>
      </div>

      <div className="update-entry">
        <div className="update-meta">
          <span className="update-date">FEB 03</span>
          <div className="update-dot"></div>
        </div>
        <div className="update-content">
          <h4>ASHES OF THE BULLET</h4>
          <p>Our inaugural challenge of focus and stillness was born. Centered around the 'Gracious Parry,' this title established our philosophy of Non-Violent Sophistication, where victory is found not in strife, but in the perfect balance of the spirit.</p>
        </div>
      </div>

      <div className="update-entry">
        <div className="update-meta">
          <span className="update-date">FEB 02</span>
          <div className="update-dot"></div>
        </div>
        <div className="update-content">
          <h4>THE ECHO OF WILL</h4>
          <p>A fundamental study in responsive interaction was released. This foundation manifestation demonstrated how the digital void can be trained to listen and react to the seeker's intent with absolute precision and event-driven grace.</p>
        </div>
      </div>

      <div className="update-entry">
        <div className="update-meta">
          <span className="update-date">FEB 02</span>
          <div className="update-dot"></div>
        </div>
        <div className="update-content">
          <h4>THE GEOMETRY OF TRUTH</h4>
          <p>We established the visual laws of our universe through this Raylib manifestation. It serves as a permanent record of how simple geometric shapes can be orchestrated to reflect the underlying order and mathematical truth of creation.</p>
        </div>
      </div>

      <div className="update-entry">
        <div className="update-meta">
          <span className="update-date">FEB 01</span>
          <div className="update-dot"></div>
        </div>
        <div className="update-content">
          <h4>INITIATION</h4>
          <p>The digital genesis of the Sanctuary occurred today. The first word was output to the console, marking the successful bridge between C++ and the modern web, and initiating our pilgrimage toward the Infinite through the power of code.</p>
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
  const [showPlayground, setShowPlayground] = useState(false);
  const [playgroundCode, setPlaygroundCode] = useState<string>(`#include "raylib.h"

int main() {
    InitWindow(800, 450, "The Divine Code - Playground");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        DrawText("LOGOS MANIFESTED", 190, 200, 40, WHITE);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}`);
  const [compileLogs, setCompileLogs] = useState<string>('');
  const [isCompiling, setIsCompiling] = useState(false);
  const [manifestedAssets, setManifestedAssets] = useState<string[]>([]);
  const [settings, setSettings] = useState({ width: 800, height: 450, fps: 60, optLevel: '2' });
  const [playgroundFiles, setPlaygroundFiles] = useState<string[]>([]);
  const [activeFileName, setActiveFileName] = useState<string>('manifestation.cpp');
  const [autoCompile, setAutoCompile] = useState(false);
  const [snapshots, setSnapshots] = useState<{name: string, timestamp: number}[]>([]);

  const fetchSnapshots = async () => {
    try {
      const res = await fetch('/api/playground-snapshots');
      const data = await res.json();
      if (data.success) setSnapshots(data.snapshots);
    } catch (e) {}
  };

  const saveSnapshot = async () => {
    const name = prompt('Enter a name for this snapshot (e.g., "Movement Prototype"):');
    if (!name) return;
    try {
      const res = await fetch('/api/save-snapshot', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ name, code: playgroundCode, fileName: activeFileName })
      });
      const data = await res.json();
      if (data.success) fetchSnapshots();
    } catch (e) {}
  };

  const loadSnapshot = async (snapshotName: string) => {
    try {
      const res = await fetch(`/api/load-snapshot?name=${encodeURIComponent(snapshotName)}`);
      const data = await res.json();
      if (data.success) {
        setPlaygroundCode(data.code);
        setActiveFileName(data.fileName);
      }
    } catch (e) {}
  };
  
  // Phase 2: Live Reload Logic
  useEffect(() => {
    if (!autoCompile || !showPlayground) return;
    
    const timeout = setTimeout(() => {
      handleCompile();
    }, 2000);
    
    return () => clearTimeout(timeout);
  }, [playgroundCode, autoCompile]);

  const playgroundExamples = {
    basic: `#include "raylib.h"

int main() {
    InitWindow(800, 450, "The Divine Code - Playground");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        DrawText("LOGOS MANIFESTED", 190, 200, 40, WHITE);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}`,
    shapes: `#include "raylib.h"

int main() {
    InitWindow(800, 450, "Geometric Truth");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawCircle(400, 225, 100, MAROON);
        DrawRectangle(200, 100, 150, 150, BLUE);
        DrawTriangle({400, 50}, {300, 150}, {500, 150}, GOLD);
        DrawText("ORDER IN GEOMETRY", 250, 350, 30, DARKGRAY);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}`,
    input: `#include "raylib.h"

int main() {
    InitWindow(800, 450, "The Echo of Will");
    SetTargetFPS(60);
    Vector2 ballPosition = { (float)800/2, (float)450/2 };

    while (!WindowShouldClose()) {
        if (IsKeyDown(KEY_RIGHT)) ballPosition.x += 2.0f;
        if (IsKeyDown(KEY_LEFT)) ballPosition.x -= 2.0f;
        if (IsKeyDown(KEY_UP)) ballPosition.y -= 2.0f;
        if (IsKeyDown(KEY_DOWN)) ballPosition.y += 2.0f;

        BeginDrawing();
        ClearBackground(BLACK);
        DrawCircleV(ballPosition, 50, MAROON);
        DrawText("MOVE WITH WASD OR ARROWS", 10, 10, 20, DARKGRAY);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}`,
    threeD: `#include "raylib.h"

int main() {
    InitWindow(800, 450, "Ashes of the Scroll - 3D");
    Camera camera = { 0 };
    camera.position = (Vector3){ 10.0f, 10.0f, 10.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        UpdateCamera(&camera, CAMERA_ORBITAL);

        BeginDrawing();
        ClearBackground(RAYWHITE);
        BeginMode3D(camera);
            DrawCube((Vector3){ 0, 0, 0 }, 2.0f, 2.0f, 2.0f, RED);
            DrawCubeWires((Vector3){ 0, 0, 0 }, 2.0f, 2.0f, 2.0f, MAROON);
            DrawGrid(10, 1.0f);
        EndMode3D();
        DrawText("3D SACRED GEOMETRY", 10, 40, 20, DARKGRAY);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}`,
    audio: `#include "raylib.h"

int main() {
    InitWindow(800, 450, "The Harmony of Logic");
    InitAudioDevice();
    Sound fx = LoadSound("resources/target.wav");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_SPACE)) PlaySound(fx);
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("PRESS SPACE TO INVOKE SOUND", 200, 200, 20, MAROON);
        EndDrawing();
    }

    UnloadSound(fx);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}`,
    texture: `#include "raylib.h"

int main() {
    InitWindow(800, 450, "The Icon of Truth");
    Texture2D tex = LoadTexture("resources/preview.png");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexture(tex, 400 - tex.width/2, 225 - tex.height/2, WHITE);
        DrawText("TEXTURE MANIFESTED", 10, 10, 20, RAYWHITE);
        EndDrawing();
    }

    UnloadTexture(tex);
    CloseWindow();
    return 0;
}`,
    collision: `#include "raylib.h"

int main() {
    InitWindow(800, 450, "The Boundary of Being");
    Rectangle player = { 400, 225, 50, 50 };
    Rectangle wall = { 200, 100, 200, 200 };
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        if (IsKeyDown(KEY_RIGHT)) player.x += 4;
        if (IsKeyDown(KEY_LEFT)) player.x -= 4;
        if (IsKeyDown(KEY_UP)) player.y -= 4;
        if (IsKeyDown(KEY_DOWN)) player.y += 4;

        bool collision = CheckCollisionRecs(player, wall);

        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawRectangleRec(wall, GRAY);
        DrawRectangleRec(player, collision ? RED : BLUE);
        DrawText(collision ? "COLLISION DETECTED" : "NAVIGATE THE VOID", 10, 10, 20, DARKGRAY);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}`,
    shader: `#include "raylib.h"

int main() {
    InitWindow(800, 450, "The Light of Logos");
    // Shaders require external .fs files, usually preloaded
    // This is a placeholder for the shader manifestation study
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        DrawText("SHADER LOGIC UNDER STUDY", 200, 200, 20, GOLD);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}`
  };

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
    const params = new URLSearchParams(window.location.search);
    const sharedCode = params.get('code');
    if (sharedCode) {
      try {
        setPlaygroundCode(atob(sharedCode));
        setShowPlayground(true);
      } catch (e) {
        console.error("Failed to decode shared code.");
      }
    }
    
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
    setShowPlayground(false);
    const url = new URL(window.location.href);
    url.searchParams.delete('manifest');
    window.history.pushState({}, '', url);
    setError(null);
    setShowHelp(false);
  };

  const handleCompile = async () => {
    setIsCompiling(true);
    setCompileLogs('GATHERING FRAGMENTS...\n');
    try {
      const res = await fetch('/api/compile', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ code: playgroundCode, settings, fileName: activeFileName })
      });
      const data = await res.json();
      setCompileLogs(data.logs || (data.success ? 'ORDER ACHIEVED.' : 'THE LOGIC IS UNSOUND.'));
      
      // Always fetch files to show new fragments even if compile fails
      fetchFiles();

      if (data.success) {
        // Automatically open the manifestation in a new window
        setTimeout(() => {
          window.open('/wasm/playground/', 'manifestation_window');
        }, 500);
      }
    } catch (e) {
      setCompileLogs('COMMUNION INTERRUPTED.');
    } finally {
      setIsCompiling(false);
    }
  };

  const handleSave = async () => {
    try {
      const res = await fetch('/api/save-playground-file', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ name: activeFileName, code: playgroundCode })
      });
      const data = await res.json();
      if (data.success) {
        setCompileLogs('FRAGMENT PRESERVED.');
        fetchFiles();
      } else {
        setCompileLogs('PRESERVATION FAILED.');
      }
    } catch (e) {
      setCompileLogs('COMMUNION INTERRUPTED.');
    }
  };

  const saveNewFile = async (name: string, content: string) => {
    try {
      await fetch('/api/save-playground-file', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ name, code: content })
      });
      fetchFiles();
    } catch (e) {}
  };

  const fetchFiles = async () => {
    try {
      const res = await fetch('/api/playground-files');
      const data = await res.json();
      if (data.success) setPlaygroundFiles(data.files);
    } catch (e) {}
  };

  const openFile = async (name: string) => {
    if (name.startsWith('resources/')) return; // Can't edit binary assets yet
    try {
      const res = await fetch(`/api/playground-file-content?name=${encodeURIComponent(name)}`);
      const data = await res.json();
      if (data.success) {
        setPlaygroundCode(data.content);
        setActiveFileName(name);
      }
    } catch (e) {}
  };

  const deleteFile = async (name: string) => {
    if (!confirm(`Are you sure you want to discard this fragment: ${name}?`)) return;
    try {
      const res = await fetch(`/api/delete-playground-file?name=${encodeURIComponent(name)}`, { method: 'DELETE' });
      const data = await res.json();
      if (data.success) {
        setTimeout(() => fetchFiles(), 300);
        if (activeFileName === name) {
          setPlaygroundCode('// Fragment discarded.');
          setActiveFileName('manifestation.cpp');
        }
      }
    } catch (e) {}
  };

  useEffect(() => {
    if (showPlayground) {
      fetchFiles();
      fetchSnapshots();
    }
  }, [showPlayground]);

  const handleFileUpload = async (file: File) => {
    const isCode = file.name.endsWith('.cpp') || file.name.endsWith('.h') || file.name.endsWith('.hpp');
    const reader = new FileReader();
    
    reader.onload = async (event) => {
      const data = event.target?.result as string;
      try {
        const endpoint = isCode ? '/api/upload-code' : '/api/upload-asset';
        const res = await fetch(endpoint, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ name: file.name, data })
        });
        const result = await res.json();
        if (result.success) {
          // Add a small delay for filesystem sync
          setTimeout(() => fetchFiles(), 300);
          if (isCode) {
            setCompileLogs(prev => prev + `\nFragment manifested: ${file.name}`);
          } else {
            setManifestedAssets(prev => [...new Set([...prev, file.name])]);
            setCompileLogs(prev => prev + `\nAsset manifested: ${file.name}`);
          }
        }
      } catch (err) {
        setCompileLogs(prev => prev + `\nFailed to manifest: ${file.name}`);
      }
    };

    if (isCode) {
      reader.readAsText(file);
    } else {
      reader.readAsDataURL(file);
    }
  };

  const handleAssetUpload = async (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0];
    if (file) handleFileUpload(file);
  };

  const handleDrop = async (e: React.DragEvent) => {
    e.preventDefault();
    const files = Array.from(e.dataTransfer.files);
    for (const file of files) {
      await handleFileUpload(file);
    }
  };

  const sharePlayground = () => {
    const encodedCode = btoa(playgroundCode);
    const url = new URL(window.location.href);
    url.searchParams.set('code', encodedCode);
    navigator.clipboard.writeText(url.toString());
    alert('Manifestation link copied to clipboard.');
  };

  // --- RENDER HELPERS ---

  const renderPlayground = () => (
    <div className="playground-view animate-in">
      <div className="details-header">
        <button className="back-button-top" onClick={backToList}>← RETURN TO THE SANCTUARY</button>
        <div className="title-share-row">
          <h2>MANIFEST LOGOS</h2>
          <div className="example-selector">
            <span style={{ fontSize: '0.7rem', fontWeight: 900, marginRight: '10px' }}>SELECT TEMPLATE:</span>
            <button className="help-trigger" onClick={() => setPlaygroundCode(playgroundExamples.basic)}>BASIC</button>
            <button className="help-trigger" onClick={() => setPlaygroundCode(playgroundExamples.shapes)}>GEOMETRY</button>
            <button className="help-trigger" onClick={() => setPlaygroundCode(playgroundExamples.input)}>INPUT</button>
            <button className="help-trigger" onClick={() => setPlaygroundCode(playgroundExamples.threeD)}>3D</button>
            <button className="help-trigger" onClick={() => setPlaygroundCode(playgroundExamples.audio)}>AUDIO</button>
            <button className="help-trigger" onClick={() => setPlaygroundCode(playgroundExamples.texture)}>TEXTURE</button>
            <button className="help-trigger" onClick={() => setPlaygroundCode(playgroundExamples.collision)}>COLLISION</button>
            <button className="help-trigger" style={{ marginLeft: '20px' }} onClick={() => window.location.href='/api/export-playground'}>EXPORT BUNDLE</button>
            <button className="share-button" style={{ marginLeft: '10px' }} onClick={sharePlayground}>SHARE</button>
          </div>
        </div>
      </div>

      <div 
        className="playground-layout"
        onDragOver={(e) => e.preventDefault()}
        onDrop={handleDrop}
      >
        <div className="playground-sidebar">
          <div className="sidebar-header">
            <h3>FRAGMENTS</h3>
            <button className="help-trigger mini-btn" onClick={async () => {
              const name = prompt('Enter fragment name (e.g. tools.h):');
              if (name) {
                const initialCode = '// New fragment of logic';
                setActiveFileName(name);
                setPlaygroundCode(initialCode);
                await saveNewFile(name, initialCode);
              }
            }}>+</button>
          </div>
          <div className="file-list">
            {playgroundFiles.map(file => (
              <div 
                key={file} 
                className={`file-item ${activeFileName === file ? 'active' : ''}`}
                onClick={() => openFile(file)}
                style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}
              >
                <span style={{ overflow: 'hidden', textOverflow: 'ellipsis' }}>
                  {file.startsWith('resources/') ? '📦 ' : '📄 '}
                  {file.replace('resources/', '')}
                </span>
                {!file.startsWith('resources/') && (
                  <button 
                    className="delete-btn" 
                    onClick={(e) => { e.stopPropagation(); deleteFile(file); }}
                    title="Discard Fragment"
                  >
                    ×
                  </button>
                )}
              </div>
            ))}
          </div>

          <div className="sidebar-header" style={{ marginTop: '30px' }}>
            <h3>SNAPSHOTS</h3>
            <button className="help-trigger mini-btn" onClick={saveSnapshot}>SAVE</button>
          </div>
          <div className="file-list">
            {snapshots.map(s => (
              <div 
                key={s.name} 
                className="file-item"
                onClick={() => loadSnapshot(s.name)}
                title={new Date(s.timestamp).toLocaleString()}
              >
                <span>🏺 {s.name}</span>
              </div>
            ))}
            {snapshots.length === 0 && (
              <span style={{ fontSize: '0.65rem', color: 'var(--text-dim)', padding: '10px' }}>No snapshots preserved.</span>
            )}
          </div>
        </div>

        <div className="editor-section">
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '10px' }}>
            <h3>SACRED SCRIPT: {activeFileName}</h3>
            <label className="auto-compile-toggle">
              <input 
                type="checkbox" 
                checked={autoCompile} 
                onChange={(e) => setAutoCompile(e.target.checked)} 
              />
              <span style={{ fontSize: '0.7rem', fontWeight: 900, marginLeft: '8px', color: 'var(--text-dim)' }}>LIVE RELOAD</span>
            </label>
          </div>
          <div className="monaco-container">
            <Editor
              height="500px"
              defaultLanguage="cpp"
              theme={isDarkMode ? "vs-dark" : "light"}
              value={playgroundCode}
              onChange={(value) => setPlaygroundCode(value || '')}
              onMount={(_, monaco) => {
                // Add basic Raylib keywords to completion provider
                monaco.languages.registerCompletionItemProvider('cpp', {
                  provideCompletionItems: (model: any, position: any) => {
                    const word = model.getWordUntilPosition(position);
                    const range = {
                      startLineNumber: position.lineNumber,
                      endLineNumber: position.lineNumber,
                      startColumn: word.startColumn,
                      endColumn: word.endColumn,
                    };
                    const suggestions = [
                      'InitWindow', 'SetTargetFPS', 'WindowShouldClose', 'BeginDrawing', 'EndDrawing', 
                      'ClearBackground', 'DrawText', 'DrawCircle', 'DrawRectangle', 'DrawTriangle',
                      'CheckCollisionRecs', 'IsKeyDown', 'IsKeyPressed', 'KEY_RIGHT', 'KEY_LEFT',
                      'KEY_UP', 'KEY_DOWN', 'KEY_SPACE', 'RAYWHITE', 'BLACK', 'MAROON', 'GOLD',
                      'Vector2', 'Rectangle', 'Color', 'LoadTexture', 'UnloadTexture', 'DrawTexture',
                      'InitAudioDevice', 'LoadSound', 'UnloadSound', 'PlaySound', 'CloseAudioDevice'
                    ].map(label => ({
                      label,
                      kind: monaco.languages.CompletionItemKind.Function,
                      insertText: label,
                      range
                    }));
                    return { suggestions };
                  }
                });
              }}
              options={{
                minimap: { enabled: false },
                fontSize: 14,
                lineNumbers: 'on',
                scrollBeyondLastLine: false,
                automaticLayout: true,
                padding: { top: 20, bottom: 20 }
              }}
            />
          </div>
          <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '20px' }}>
            <button 
              className="action-play-button" 
              onClick={handleCompile}
              disabled={isCompiling}
            >
              {isCompiling ? 'ORDERING...' : 'ORDER FRAGMENT (COMPILE)'}
            </button>
            <button 
              className="action-play-button" 
              onClick={handleSave}
              style={{ background: 'var(--glass-bg)', color: 'var(--text-color)', border: '1px solid var(--glass-border)' }}
            >
              PRESERVE (SAVE)
            </button>
          </div>

          <div className="playground-settings" style={{ marginTop: '20px', marginBottom: '20px' }}>
            <h3>SACRED PARAMETERS</h3>
            <div className="settings-grid" style={{ display: 'grid', gridTemplateColumns: 'repeat(4, 1fr)', gap: '15px' }}>
              <div className="setting-item">
                <label>WIDTH</label>
                <input type="number" value={settings.width} onChange={e => setSettings({...settings, width: parseInt(e.target.value)})} />
              </div>
              <div className="setting-item">
                <label>HEIGHT</label>
                <input type="number" value={settings.height} onChange={e => setSettings({...settings, height: parseInt(e.target.value)})} />
              </div>
              <div className="setting-item">
                <label>FPS</label>
                <input type="number" value={settings.fps} onChange={e => setSettings({...settings, fps: parseInt(e.target.value)})} />
              </div>
              <div className="setting-item">
                <label>OPT LEVEL</label>
                <select 
                  value={settings.optLevel} 
                  onChange={e => setSettings({...settings, optLevel: e.target.value})}
                  style={{ background: 'rgba(0,0,0,0.3)', border: '1px solid var(--glass-border)', color: 'inherit', padding: '8px', borderRadius: '6px' }}
                >
                  <option value="0">O0 (DEBUG)</option>
                  <option value="1">O1 (FAST)</option>
                  <option value="2">O2 (FASTER)</option>
                  <option value="3">O3 (FASTEST)</option>
                  <option value="s">Os (SIZE)</option>
                  <option value="z">Oz (MIN SIZE)</option>
                </select>
              </div>
            </div>
          </div>
          
          <div className="asset-management">
            <h3>MANIFEST ASSETS (RESOURCES/)</h3>
            <div className="asset-actions">
              <label className="help-trigger" style={{ cursor: 'pointer', display: 'inline-block' }}>
                UPLOAD ASSET
                <input type="file" style={{ display: 'none' }} onChange={handleAssetUpload} />
              </label>
            </div>
            <div className="manifested-assets-list">
              {manifestedAssets.length > 0 ? (
                manifestedAssets.map(asset => (
                  <span key={asset} className="asset-tag">{asset}</span>
                ))
              ) : (
                <span style={{ fontSize: '0.7rem', color: 'var(--text-dim)' }}>No fragments manifested yet.</span>
              )}
            </div>
          </div>
        </div>

        <div className="logs-section">
          <h3>CHRONICLE OF CREATION</h3>
          <div className="compile-logs-wrapper">
            <SyntaxHighlighter 
              language="bash" 
              style={isDarkMode ? vscDarkPlus : prism}
              customStyle={{
                background: 'transparent',
                padding: '0',
                margin: '0',
                fontSize: '0.8rem'
              }}
            >
              {compileLogs || 'Awaiting the Word...'}
            </SyntaxHighlighter>
          </div>
          {compileLogs.includes('Manifestation complete.') && (
            <button className="action-play-button" onClick={() => window.open('/wasm/playground/', '_blank')}>
              ASCEND (RUN MANIFESTATION)
            </button>
          )}
        </div>
      </div>
    </div>
  );

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
          <span className="census-value">{stats?.atomicWeight || '8485'}</span>
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
          <button className="help-trigger" onClick={() => { setShowPlayground(true); window.scrollTo(0,0); }}>MANIFEST</button>
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
        {showPlayground ? (
          renderPlayground()
        ) : !selectedGameDetails ? (
          <>
            {renderHero()}
            <div className="games-grid-wrapper">
              {renderGameList()}
              {renderDivineCensus()}
              {renderTechnicalFoundations()}
            </div>
            <div className="home-secondary-content">
              {renderChronicleOfLight()}
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