        Vector3 shootDir = Vector3Normalize(toAim);
        Vector3 muzzle = Vector3Add(player.pos, Vector3Scale(shootDir, 2.0f));
        muzzle.y = 1.5f;
        SpawnBullet(muzzle, Vector3Scale(shootDir, player.bulletSpeed), SKYBLUE, true);
        player.shootCD = player.shootRate;
        SpawnParticles(muzzle, YELLOW, 6, 8.0f);
    }

    if (IsKeyPressed(KEY_SPACE) && player.stamina >= PARRY_COST && !player.isParrying) {
        player.isParrying = true;
        player.parryTimer = player.parryWindow;
        player.stamina -= PARRY_COST;
    }
    if (player.isParrying) {
        player.parryTimer -= dt;
        if (player.parryTimer <= 0.0f) player.isParrying = false;
    }

    if (IsKeyPressed(KEY_E) && player.flasks > 0 && !player.isHealing) {
        player.isHealing = true;
        player.healTimer = FLASK_TIME;
        player.flasks--;
    }
    if (player.isHealing && player.healTimer <= 0.5f) {
        player.health = std::min(player.health + (int)FLASK_HEAL_BASE, player.maxHealth);
    }

    if (player.isRolling) {
        player.rollTimer -= dt;
        player.pos = Vector3Add(player.pos, Vector3Scale(player.rollDir, ROLL_SPEED * dt));
        if (player.rollTimer <= 0.0f) {
