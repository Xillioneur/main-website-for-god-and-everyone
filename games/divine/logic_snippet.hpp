        it->lifetime -= dt;
        
        if(Vector3Distance(player.pos, it->pos) < 2.0f && !it->collected) {
            it->collected = true;
            player.inventory.push_back(it->equipment);
            world.message = "Found: " + it->equipment.name + "!";
            world.messageTimer = 3.0f;
            SpawnParticles(it->pos, GetRarityColor(it->equipment.rarity), 25, 15.0f);
        }
        
        if(it->collected || it->lifetime <= 0) {
            it = world.equipmentDrops.erase(it);
        } else {
            ++it;
        }
    }
    
    if(world.messageTimer > 0) world.messageTimer -= dt;
}

void UpdateEnemies(float dt) {
    for(auto& e : world.enemies) {
        if(!e.isAlive) continue;
        
                    // Defeat Animation Logic
                    if (e.aiState == 5) {
                        e.defeatTimer -= dt;
                        e.pos.y += 30.0f * dt; // Rapid beam up
                        e.scale = e.defeatTimer; // Shrink to zero
                        if (e.defeatTimer <= 0) e.isAlive = false;
                        continue;
                    }        
        e.shootTimer -= dt;
        e.actionTimer = std::max(0.0f, e.actionTimer - dt);
        e.abilityCooldown = std::max(0.0f, e.abilityCooldown - dt);
        
        Vector3 toPlayer = Vector3Subtract(player.pos, e.pos);
        toPlayer.y = 0;
        float dist = Vector3Length(toPlayer);
        Vector3 dirToPlayer = Vector3Normalize(toPlayer);
        
        if(dist > 1.0f) {
            e.rotation = atan2f(toPlayer.x, toPlayer.z);
        }
        
        // --- AI LOGIC ---
        
        // 1. Dodge Logic (For agile enemies)
        if (e.aiState != 2 && e.abilityCooldown <= 0 && (e.type == WATCHER || e.type == WHISPERER || e.type == GLITCH_SPECTRE)) {
            // Check for incoming bullets
            for (const auto& b : world.bullets) {
                if (b.playerBullet && Vector3Distance(e.pos, b.pos) < 15.0f) {
                    // Dodge perpendicular to bullet
                    Vector3 side = Vector3CrossProduct(b.vel, {0, 1, 0});
                    side = Vector3Normalize(side);
                    if (GetRandomValue(0, 1) == 0) side = Vector3Negate(side);
                    
                    e.aiState = 2; // DODGING
                    e.actionTimer = 0.4f; // Dodge duration
                    e.vel = Vector3Scale(side, 40.0f); // DASH SPEED
                    e.abilityCooldown = 2.0f + (GetRandomValue(0, 20)/10.0f);
                    SpawnParticles(e.pos, e.color, 10, 5.0f);
                    break;
                }
            }
        }
        
        // 2. Sprint/Chase Logic (For aggressors)
        if (e.aiState == 0 && (e.type == ASHBOUND || e.type == BOSS_KEEPER) && dist > 40.0f && e.abilityCooldown <= 0) {
            e.aiState = 3; // CHARGING
            e.actionTimer = 1.5f;
            e.abilityCooldown = 4.0f;
            e.chargeDir = dirToPlayer;
            if(e.type == BOSS_KEEPER) {
                world.message = "DIVINE WRATH IMMINENT";
                world.messageTimer = 1.0f;
                SpawnParticles(e.pos, GOLD, 30, 10.0f);
            }
        }

        // 3. Boss Defensive Logic (Neural Barrier)
        if (e.isBoss && e.aiState == 0 && e.abilityCooldown <= 0 && GetRandomValue(0, 100) < 30) {
            e.aiState = 4; // SHIELDING (Re-using state 4)
            e.actionTimer = 2.0f;
            e.abilityCooldown = 6.0f;
            world.message = "HEAVENLY AEGIS MANIFESTED";
            world.messageTimer = 1.5f;
        }
        
        // --- MOVEMENT APPLICATION ---
        
        Vector3 moveForce = {0, 0, 0};
        
        // State-based movement override
        if (e.aiState == 2) { // Dodging
            // Velocity is already set, just decay it less for smooth dash
            // No steering input while dodging
        } 
        else if (e.aiState == 3) { // Charging
            // Accelerate heavily in charge direction
            moveForce = Vector3Scale(e.chargeDir, e.moveSpeed * 8.0f); 
