      float ringWave = sinf(GetTime() * 10.0f) * 0.2f + 1.5f;
      DrawCircle3D(game.player.position, ringWave, {0, 1, 0}, 90.0f,
                   ColorAlpha(ORANGE, 0.4f));
    }

    // Draw Bullets (Player)
    for (const auto &b : game.playerBullets) {
      if (b.active) {
        // Trail
        DrawLine3D(
            b.position,
            Vector3Subtract(b.position,
                            Vector3Scale(Vector3Normalize(b.velocity), 1.0f)),
            b.color);
        // Glow
        DrawSphere(b.position, b.radius * 2.5f, ColorAlpha(b.color, 0.4f));
        // Core
        DrawSphere(b.position, b.radius, WHITE);
      }
    }
    // Draw Bullets (Enemy)
    for (const auto &b : game.enemyBullets) {
      if (b.active) {
        // Trail
        DrawLine3D(
            b.position,
            Vector3Subtract(b.position,
                            Vector3Scale(Vector3Normalize(b.velocity), 1.0f)),
            b.color);
        // Glow (Increased for better readability)
        DrawSphere(b.position, b.radius * 4.0f, ColorAlpha(b.color, 0.5f));
        // Core
        DrawSphere(b.position, b.radius, WHITE);
      }
    }

    // Draw Enemies
    for (const auto &enemy : game.enemies) {
      if (enemy.active) {
        Color ec;
        if (enemy.hitTimer > 0.0f) {
          ec = WHITE;
        } else {
          if (enemy.type == 2)
            ec = PURPLE;
          else if (enemy.type == 3)
            ec = DARKGREEN;
          else if (enemy.type == 1)
            ec = MAROON;
          else if (enemy.type == 4)
            ec = MAGENTA; // PHANTOM
          else if (enemy.type == 5)
            ec = LIME; // SPLITTER
          else if (enemy.type == 6)
            ec = SKYBLUE; // SUPPORT
          else
            ec = RED;
        }

        if (enemy.type == 2) { // boss
          DrawCube(enemy.position, 3.0f, 3.0f, 3.0f, ec);
          DrawCubeWires(enemy.position, 3.0f, 3.0f, 3.0f, DARKPURPLE);
        } else if (enemy.type == 3) { // Tank
          DrawCube(enemy.position, 1.5f, 1.5f, 1.5f, ec);
          DrawCubeWires(enemy.position, 1.5f, 1.5f, 1.5f, GREEN);
        } else {
          DrawCube(enemy.position, 1.0f, 1.0f, 1.0f, ec);
          DrawCubeWires(enemy.position, 1.0f, 1.0f, 1.0f, DARKGRAY);
        }

        // Debug Visuals
        if (game.debugMode) {
          float radius =
              (enemy.type == 2) ? 3.0f : (enemy.type == 3 ? 0.8f : 0.5f);
          DrawSphereWires(enemy.position, radius, 8, 8, GREEN);
        }
      }
    }

    // Draw Particles
    for (const auto &p : game.particles) {
      if (p.active) {
        Color c = p.color;
        c.a = (unsigned char)(255.0f * (p.life > 1.0f ? 1.0f : p.life));
        DrawCube(p.position, p.size, p.size, p.size, c);
      }
    }
    // Draw Floating Text (3D)
    for (const auto &ft : game.floatingTexts) {
      if (ft.active) {
        Vector2 screenPos = GetWorldToScreen(ft.position, game.camera);
        DrawText(ft.text, (int)screenPos.x - MeasureText(ft.text, 12) / 2,
                 (int)screenPos.y, 12, ColorAlpha(ft.color, ft.life));
      }
    }

    rlPopMatrix();
    EndMode3D();

  } else if (game.currentScreen == SCREEN_UPGRADE) {
    ClearBackground(BLACK); // Ensure clean background
