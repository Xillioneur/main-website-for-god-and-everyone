    // Parry
    if(IsKeyPressed(KEY_Q) && player.stamina >= PRAYER_COST && !player.isParrying) {
        player.isParrying = true;
        player.parryTimer = PRAYER_WINDOW;
        player.stamina -= PRAYER_COST;
        SpawnParticles(player.pos, GOLD, 15, 8.0f);
    }
    
    // Lamp toggle
    if(IsKeyPressed(KEY_L)) {
        player.lampActive = !player.lampActive;
    }
    
    // Flask
    if(IsKeyPressed(KEY_E) && player.flasks > 0 && !player.isHealing && player.health < player.maxHealth) {
        player.isHealing = true;
        player.healTimer = 1.2f;
        player.flasks--;
    }
    if(player.healTimer <= 0.5f && player.healTimer > 0) {
        int healAmount = 40 + player.faith * 2;
        player.health = std::min(player.health + healAmount, player.maxHealth);
    }
    
    // Protocol Override (Ultimate)
    if (IsKeyPressed(KEY_R) && player.syncMeter >= player.maxSyncMeter) {
        player.syncMeter = 0;
        ShakeScreen(2.5f);
        world.message = "PROTOCOL OVERRIDE: RE-CODING...";
        world.messageTimer = 2.0f;
        
