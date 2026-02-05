                    moveSpeed *= 0.85f;
                }
            }

            // Attack decision
            Vector3 eFacing = {sinf(e.rotation * DEG2RAD), 0.0f, cosf(e.rotation * DEG2RAD)};
            float dot = Vector3DotProduct(eFacing, Vector3Normalize(toPlayer));
            if (distToPlayer <= ATTACK_RANGE + 1.8f && dot > 0.55f && e.attackCooldown <= 0.0f &&
                e.stamina >= 26.0f && !e.isAttacking && !e.isDodging && !e.isBlocking && e.stunTimer <= 0.0f) {
                bool wantHeavy = (e.type == TANK && GetRandomValue(0, 100) < 40);
                bool canHeavy = (e.stamina >= 48.0f);
                e.isHeavyAttack = wantHeavy && canHeavy;
                float staminaCost = e.isHeavyAttack ? 48.0f : 26.0f;
                float durMult = e.isHeavyAttack ? 1.75f : 1.0f;
                e.attackTimer = e.attackDur * durMult;
                e.currentAttack = e.isHeavyAttack ? LIGHT_1 : static_cast<AttackType>(GetRandomValue(0, 2));
                e.isAttacking = true;
                e.stamina -= staminaCost;
                e.staminaRegenDelay = e.isHeavyAttack ? 1.4f : 0.8f;
                float baseCd = (e.type == AGILE) ? 0.9f : ((e.type == TANK) ? 2.5f : 1.6f);
                baseCd += e.isHeavyAttack ? 1.3f : 0.0f;
                e.attackCooldown = baseCd + (float)GetRandomValue(0, 15) / 10.0f;
            }
        }

        // Commit to attack (no movement)
        if (e.isAttacking) {
            moveDir = {0,0,0};
        }

        e.velocity = Vector3Lerp(e.velocity, Vector3Scale(moveDir, moveSpeed), 12.0f * dt);
        e.position = Vector3Add(e.position, Vector3Scale(e.velocity, dt));

        // Dodge player attack
        if (player.isAttacking && distToPlayer < 9.0f && e.stamina >= 32.0f &&
            !e.isDodging && !e.isAttacking && !e.isBlocking &&
            GetRandomValue(0, 100) < (int)(e.dodgeChance * 100.0f)) {
            e.isDodging = true;
            e.dodgeTimer = ROLL_DURATION;
            e.dodgeStartPos = e.position;
            Vector3 dodgeDir = Vector3Normalize(Vector3Subtract(e.position, player.position));
            if (e.type == AGILE && GetRandomValue(0, 100) < 60) {
                Vector3 side = {dodgeDir.z, 0.0f, -dodgeDir.x};
                side = Vector3Scale(side, GetRandomValue(0, 1) ? 1.0f : -1.0f);
                dodgeDir = Vector3Normalize(Vector3Add(dodgeDir, side));
            }
            e.dodgeDirection = dodgeDir;
            e.stamina -= 32.0f;
            e.staminaRegenDelay = REGEN_DELAY_AFTER_ACTION;
        }

        if (e.isDodging) {
            e.dodgeTimer -= dt;
            float progress = 1.0f - (e.dodgeTimer / ROLL_DURATION);
            float eased = progress * progress;
            e.position = Vector3Add(e.dodgeStartPos,
                                   Vector3Scale(e.dodgeDirection, 12.5f * eased));
            if (e.dodgeTimer <= 0.0f) e.isDodging = false;
        }

        // Tank block
        if (e.type == TANK && !e.isBlocking && !e.isAttacking && !e.isDodging &&
            player.isAttacking && distToPlayer < ATTACK_RANGE + 3.0f && e.stamina >= 22.0f &&
            GetRandomValue(0, 100) < 75) {
            e.isBlocking = true;
            e.blockTimer = 0.7f;
            e.stamina -= 22.0f;
            e.staminaRegenDelay = REGEN_DELAY_AFTER_ACTION;
        }
        if (e.isBlocking) {
            e.blockTimer -= dt;
            if (e.blockTimer <= 0.0f) e.isBlocking = false;
        }

        // Attack execution
        if (e.isAttacking) {
            float dur = (e.type == BOSS) ? ((e.comboStep == 3 || e.comboStep == 5) ? 0.85f : 0.55f)
                                        : e.attackDur * (e.isHeavyAttack ? 1.75f : 1.0f);
            float progress = 1.0f - (e.attackTimer / dur);

            // Boss combo animations
            if (e.type == BOSS) {
                switch(e.comboStep) {
                    case 1: e.swingYaw = Lerp(80.0f, -80.0f, progress);
                            e.swingPitch = Lerp(90.0f, -70.0f, progress); break;
                    case 2: e.swingYaw = Lerp(-120.0f, 120.0f, progress);
                            e.swingPitch = Lerp(40.0f, -40.0f, progress); break;
                    case 3: e.swingYaw = Lerp(-180.0f, 180.0f, progress);
                            e.swingPitch = Lerp(0.0f, 0.0f, progress); break;
                    case 4: e.swingYaw = Lerp(60.0f, -60.0f, progress);
                            e.swingPitch = Lerp(-100.0f, 100.0f, progress); break;
                    case 5: {
                        float pp = progress * 3.0f;
                        if (pp < 1.0f) {
                            e.swingYaw = Lerp(100.0f, -100.0f, pp);
                            e.swingPitch = Lerp(160.0f, -110.0f, pp);
                        } else if (pp < 2.0f) {
                            e.swingYaw = Lerp(-100.0f, 200.0f, pp - 1.0f);
                            e.swingPitch = -110.0f;
                        } else {
                            e.swingYaw = Lerp(200.0f, 0.0f, pp - 2.0f);
