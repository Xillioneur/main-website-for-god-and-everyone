
void DrawCrosshairAndAimMarker() {
    Vector2 mousePos = GetMousePosition();

    DrawLineEx({mousePos.x - 12, mousePos.y}, {mousePos.x + 12, mousePos.y}, 2.0f, WHITE);
    DrawLineEx({mousePos.x, mousePos.y - 12}, {mousePos.x, mousePos.y + 12}, 2.0f, WHITE);
    DrawCircleLines((int)mousePos.x, (int)mousePos.y, 18.0f, WHITE);
    DrawCircleLines((int)mousePos.x, (int)mousePos.y, 10.0f, WHITE);
}

void DrawHUD() {
    int y = 30;

    // Health
    DrawRectangle(30, y, 400, 40, Fade(BLACK, 0.7f));
    DrawRectangle(35, y+5, 390 * (float)player.health / player.maxHealth, 30, RED);
    DrawText("HEALTH", 40, y+8, 28, WHITE);
    y += 60;

    // Stamina
    DrawRectangle(30, y, 400, 30, Fade(BLACK, 0.7f));
    DrawRectangle(35, y+5, 390 * player.stamina / player.maxStamina, 20, LIME);
    y += 50;

    // Score & Combo
    DrawText(TextFormat("SCORE: %d", player.score), 30, y, 40, GOLD);
    if (player.combo > 1) DrawText(TextFormat("COMBO x%d", player.combo), 30, y+50, 50, ORANGE);
    y += 100;

    // Accuracy
    if (totalEnemyBullets > 0) {
        accuracy = 100.0f * neutralized / totalEnemyBullets;
        Color accCol = accuracy > 80 ? LIME : accuracy > 50 ? YELLOW : RED;
        DrawText(TextFormat("ACCURACY: %.1f%%", accuracy), 30, y, 40, accCol);
    }
    y += 60;

    // Souls & Stats
    DrawText(TextFormat("Souls: %d", player.souls), SCREEN_WIDTH - 320, 30, 50, YELLOW);
    DrawText(TextFormat("VIT %d | END %d | STR %d | DEX %d", 
                        player.vitality, player.endurance, player.strength, player.dexterity),
             SCREEN_WIDTH - 520, 90, 40, DARKGRAY);

    // Wave & Flasks
    DrawText(TextFormat("WAVE %d", wave), SCREEN_WIDTH - 300, 150, 50, GOLD);
    DrawText(TextFormat("FLASKS: %d", player.flasks), SCREEN_WIDTH - 300, 210, 40, ORANGE);

    // Boss health
    for (const auto& e : enemies) {
        if (e.alive && e.type == BOSS) {
            float ratio = (float)e.health / e.maxHealth;
            DrawRectangle(SCREEN_WIDTH/2 - 400, 40, 800, 50, Fade(BLACK, 0.8f));
            DrawRectangle(SCREEN_WIDTH/2 - 390, 50, 780 * ratio, 30, RED);
            DrawText("BULLET LORD", SCREEN_WIDTH/2 - MeasureText("BULLET LORD", 60)/2, 20, 60, GOLD);
        }
    }
}

void DrawBonfireMenu() {
    DrawRectangle(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,Fade(BLACK,0.85f));
    DrawText("SITE OF GRACE - LEVEL UP", SCREEN_WIDTH/2 - MeasureText("SITE OF GRACE - LEVEL UP", 70)/2, 120, 70, GOLD);
    DrawText(TextFormat("Souls: %d", player.souls), SCREEN_WIDTH/2 - 120, 220, 60, YELLOW);

    int y = 320;
    const char* stats[] = {"Vitality (+12 HP)", "Endurance (+15 Stamina)", "Strength (+5 Bullet Speed)", "Dexterity (Faster Fire/Parry)"};
    int levels[] = {player.vitality, player.endurance, player.strength, player.dexterity};
    for (int i = 0; i < 4; i++) {
        int cost = GetUpgradeCost(levels[i]);
        Color col = player.souls >= cost ? LIME : RED;
        DrawText(TextFormat("%d - %s (Lv %d) - Cost %d", i+1, stats[i], levels[i], cost), 300, y, 45, col);
        y += 70;
    }

    DrawText("SPACE to Continue Into the Storm", SCREEN_WIDTH/2 - MeasureText("SPACE to Continue Into the Storm", 40)/2, SCREEN_HEIGHT - 140, 40, LIGHTGRAY);
}

void DrawTitle() {
    DrawRectangle(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,Fade(BLACK,0.85f));
    DrawText("PARRY THE STORM", SCREEN_WIDTH/2 - MeasureText("PARRY THE STORM", 100)/2, 150, 100, GOLD);
    DrawText("Ashes of the Bullet - Dark Souls Edition", SCREEN_WIDTH/2 - MeasureText("Ashes of the Bullet - Dark Souls Edition", 50)/2, 270, 50, YELLOW);
    DrawText("WASD Move • Mouse Aim/Shoot • SPACE Parry • SHIFT Roll • E Flask", 200, 420, 36, LIGHTGRAY);
    DrawText("Return to the start and grow. Git Gud eternally.", 200, 480, 36, ORANGE);
    DrawText("Click or ENTER to begin the trial", SCREEN_WIDTH/2 - MeasureText("Click or ENTER to begin the trial", 40)/2, SCREEN_HEIGHT - 120, 40, WHITE);
}

void DrawRenewal() {
    DrawRectangle(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,Fade(BLACK,0.9f));
    DrawText("SOUL ASCENDED", SCREEN_WIDTH/2 - MeasureText("SOUL ASCENDED", 140)/2, SCREEN_HEIGHT/2 - 100, 140, RED);
    const char* quote = renewalQuotes[GetRandomValue(0, renewalQuotes.size()-1)].c_str();
    DrawText(quote, SCREEN_WIDTH/2 - MeasureText(quote, 60)/2, SCREEN_HEIGHT/2 + 40, 60, ORANGE);
    DrawText("All souls and upgrades lost...", SCREEN_WIDTH/2 - MeasureText("All souls and upgrades lost...", 50)/2, SCREEN_HEIGHT/2 + 120, 50, DARKGRAY);
    if (totalEnemyBullets > 0) {
        DrawText(TextFormat("Final Accuracy: %.1f%%", accuracy), SCREEN_WIDTH/2 - MeasureText("Final Accuracy: 100.0%", 50)/2, SCREEN_HEIGHT/2 + 180, 50, accuracy > 80 ? LIME : RED);
    }
    DrawText("R to Try Again From the Beginning", SCREEN_WIDTH/2 - MeasureText("R to Try Again From the Beginning", 40)/2, SCREEN_HEIGHT/2 + 260, 40, WHITE);
}

void DrawVictory() {
    DrawRectangle(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,Fade(BLACK,0.8f));
    DrawText("VICTORY – THE STORM IS PARRIED", SCREEN_WIDTH/2 - MeasureText("VICTORY – THE STORM IS PARRIED", 80)/2, 150, 80, GOLD);
    DrawText(TextFormat("FINAL SCORE: %d", player.score), SCREEN_WIDTH/2 - MeasureText("FINAL SCORE: 999999", 60)/2, 280, 60, YELLOW);
