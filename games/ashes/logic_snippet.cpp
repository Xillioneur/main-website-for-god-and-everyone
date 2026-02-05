    // Parry
    if (IsKeyPressed(KEY_LEFT_CONTROL) && player.stamina >= STAMINA_PARRY_COST &&
        !player.isAttacking && !player.isRolling && !player.isHealing && player.staggerTimer <= 0) {
        player.isParrying = true;
        player.parryTimer = 0.38f;
        player.stamina -= STAMINA_PARRY_COST;
        player.staminaRegenDelay = REGEN_DELAY_AFTER_ACTION;
    }
    if (player.isParrying) {
        player.parryTimer -= dt;
        if (player.parryTimer <= 0.0f) player.isParrying = false;
    }

    // Attack input
    bool attackInput = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    bool attackRelease = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);

    if (attackInput && !player.isCharging && !player.isAttacking && !player.isRolling &&
        !player.isParrying && !player.isHealing && player.stamina >= STAMINA_POWER_COST &&
        player.staggerTimer <= 0) {
        player.isCharging = true;
        player.chargeTimer = 0.0f;
        player.powerReady = false;
    }

    if (player.isCharging) {
        player.chargeTimer += dt;
        if (player.chargeTimer >= POWER_ATTACK_CHARGE) player.powerReady = true;
    }

    if (attackRelease && player.isCharging) {
