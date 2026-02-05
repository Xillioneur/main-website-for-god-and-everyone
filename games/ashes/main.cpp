#include "game.h"
#include "raylib.h"

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Divine Sentinel – The Celestial Nexus");

    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    HideCursor();
    DisableCursor();
    InitAudioDevice();
    InitGame();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // Update Logic
        UpdateCamera(dt);
        UpdateParticles(dt);

        if (gameState == TITLE_SCREEN) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_ENTER)) {
                gameState = INSTRUCTIONS;
            }
        }
        else if (gameState == INSTRUCTIONS) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_ENTER)) {
                currentLevel = 1;
                ResetLevel();  // This sets gameState = PLAYING internally
            }
        }
        else if (gameState == PLAYING || gameState == PAUSED) {
            if (IsKeyPressed(KEY_ESCAPE)) {
                if (IsCursorHidden()) {
                    EnableCursor();
                    ShowCursor();
                } else {
                    DisableCursor();
                    HideCursor(); 
                }
                gameState = (gameState == PLAYING) ? PAUSED : PLAYING;
            }

            if (gameState == PAUSED) {
                Vector2 mousePos = GetMousePosition();
                Rectangle quitBtn = { SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 + 80, 300, 60 };

                if (CheckCollisionPointRec(mousePos, quitBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    break; 
                }
            }

            if (gameState == PLAYING) {
                UpdateGame(dt);

                if (currentLevel == 1 && exitActive && Vector3Distance(player.position, exitPosition) < 9.0f) {
                    currentLevel = 2;
                    ResetLevel();
                }

                if (player.health <= 0 && !player.isDead) {
                    player.isDead = true;
                    player.deathTimer = 3.2f;
                    player.deathFallAngle = 0.0f;
                    gameState = DEAD;
                    currentDeathMessage = deathMessages[GetRandomValue(0, (int)deathMessages.size()-1)].c_str();
                }
            }
        }
        else if (gameState == DEAD) {
            UpdatePlayer(dt);
            if (IsKeyPressed(KEY_R)) {
                ResetLevel();
            }
        }
        else if (gameState == VICTORY) {
            if (IsKeyPressed(KEY_ESCAPE)) break;
        }

        // --- DRAWING HARMONY ---
        BeginDrawing();
        ClearBackground(BLACK);

        if (gameState != TITLE_SCREEN && gameState != INSTRUCTIONS) {
            // Render 3D Scene to Texture
            BeginTextureMode(target);
                ClearBackground({12, 12, 22, 255});
                BeginMode3D(camera);
                    Draw3DScene();
                EndMode3D();
            EndTextureMode();

            // Draw Texture to Screen with Divine Shader
            BeginShaderMode(bloomShader);
                // Flip texture vertically (RenderTexture has 0,0 at bottom-left)
                DrawTextureRec(target.texture, {0, 0, (float)target.texture.width, (float)-target.texture.height}, {0, 0}, WHITE);
            EndShaderMode();

            DrawHUD();
        } else {
            // Title and Instructions do not use the bloom shader
            ClearBackground({12, 12, 22, 255});
            if (gameState == TITLE_SCREEN) DrawTitleScreen();
            else if (gameState == INSTRUCTIONS) DrawInstructionsScreen();
        }

        if (gameState == DEAD) DrawDeathScreen();
        if (gameState == VICTORY) DrawVictoryScreen();
        
        if (gameState == PAUSED) {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.65f));
            DrawText("PAUSED", SCREEN_WIDTH/2 - MeasureText("PAUSED", 80)/2,
                     SCREEN_HEIGHT/2 - 60, 80, GOLD);
            DrawText("ESC to Resume", SCREEN_WIDTH/2 - MeasureText("ESC to Resume", 40)/2,
                     SCREEN_HEIGHT/2 + 40, 40, LIGHTGRAY);
            
            Rectangle quitBtn = { SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 + 80, 300, 60 };
            bool hover = CheckCollisionPointRec(GetMousePosition(), quitBtn);
            DrawRectangleRec(quitBtn, hover ? RED : MAROON);
            DrawRectangleLinesEx(quitBtn, 6, hover ? GOLD : ORANGE);
            DrawText("Quit Game",
                    quitBtn.x + (quitBtn.width - MeasureText("Quit Game", 40)) / 2,
                    quitBtn.y + 10, 40, WHITE);
        }

        EndDrawing();
    }

    // Sacred Cleanup
    UnloadRenderTexture(target);
    UnloadShader(bloomShader);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
