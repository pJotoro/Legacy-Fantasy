#define ENTITY_LEFT_COLLISION(entity) entity->pos.x = SDL_max(entity->pos.x, tile.max.x - hitbox.min.x);

#define ENTITY_RIGHT_COLLISION(entity) entity->pos.x = SDL_min(entity->pos.x, tile.min.x - hitbox.max.x);

#define ENTITY_UP_COLLISION(entity) entity->pos.y = SDL_max(entity->pos.y, tile.max.y - hitbox.min.y);

#define ENTITY_DOWN_COLLISION(entity) entity->pos.y = SDL_min(entity->pos.y, tile.min.y - hitbox.max.y);

void UpdatePlayer(Context* ctx, Entity* player) {
    if (!HAS_FLAG(player->flags, EntityFlags_Active)) {
    	return;
    }

	if (SpritesEqual(player->anim.sprite, player_die)) {
		UpdateAnim(ctx, &player->anim, false);
		if (player->anim.ended) {
			ResetGame(ctx);
		}
		return;
	}

	if (player->touching_floor && ctx->button_attack) {
		SetSprite(player, player_attack);
	}

	int32_t input_x = 0;
	if (ctx->gamepad) {
		if (ctx->gamepad_left_stick.x == 0.0f) input_x = 0;
		else if (ctx->gamepad_left_stick.x > 0.0f) input_x = 1;
		else if (ctx->gamepad_left_stick.x < 0.0f) input_x = -1;
	} else {
		input_x = ctx->button_right - ctx->button_left;
	}

	if (SpritesEqual(player->anim.sprite, player_attack)) {
		size_t n_enemies; Entity* enemies = GetEnemies(ctx, &n_enemies);
		for (size_t enemy_idx = 0; enemy_idx < n_enemies; ++enemy_idx) {
			Entity* enemy = &enemies[enemy_idx];
			if (EntitiesIntersect(ctx, player, enemy)) {
				--enemy->health;
				if (enemy->health <= 0) {
					if (HAS_FLAG(enemy->flags, EntityFlags_Boar)) {
						SetSprite(enemy, boar_hit);
					} else {
						// TODO
					}

				}
			}
		}
	} else {
		// PlayerCollision
		Rect hitbox, lh, rh, uh, dh;
		GetEntityHitboxes(ctx, player, &hitbox, &lh, &rh, &uh, &dh);
		EntityMoveY(player, PLAYER_ACC);

		player->touching_floor = SDL_max(player->touching_floor - 1, 0);

		if (player->touching_floor) {
			if (ctx->button_jump) {
				player->touching_floor = 0;
				EntityMoveY(player, PLAYER_JUMP);
				SetSprite(player, player_jump_start);
			} else {
				float acc;
				if (!ctx->gamepad) {
					acc = (float)input_x * PLAYER_ACC;
				} else {
					acc = ctx->gamepad_left_stick.x * PLAYER_ACC;
				}
				EntityMoveX(player, acc);
				#if 0
				EntityApplyFriction(player, PLAYER_FRIC, PLAYER_MAX_VEL);
				#endif
			
			}	
		} else if (ctx->button_jump_released && !HAS_FLAG(player->flags, EntityFlags_JumpReleased)) {
			vec2s vel = EntityVel(player);
			if (vel.y < 0.0f) {
				player->flags |= EntityFlags_JumpReleased;
				#if 0
				player->vel.y /= 2.0f;
				#endif

			}
		}

		Level* level = GetCurrentLevel(ctx);
		vec2s vel = EntityVel(player);
		if (vel.x < 0.0f) {
			Rect tile;
			if (RectIntersectsLevel(level, lh, &tile)) {
				ENTITY_LEFT_COLLISION(player);
			}
		} else if (vel.x > 0.0f) {
			Rect tile;
			if (RectIntersectsLevel(level, rh, &tile)) {
				ENTITY_RIGHT_COLLISION(player);
			}
		}
		if (vel.y < 0.0f) {
			Rect tile;
			if (RectIntersectsLevel(level, uh, &tile)) {
				ENTITY_UP_COLLISION(player);
			}
		} else if (vel.y > 0.0f) {
			Rect tile;
			if (RectIntersectsLevel(level, dh, &tile)) {
				ENTITY_DOWN_COLLISION(player);
				player->touching_floor = PLAYER_JUMP_REMAINDER;
				player->flags &= ~EntityFlags_JumpReleased;
			} else {
				SetSprite(player, player_jump_end);
			}
		}

		if (player->touching_floor) {
			vec2s vel = EntityVel(player);
			if (input_x == 0 && vel.x == 0.0f) {
				SetSprite(player, player_idle);
			} else {
				SetSprite(player, player_run);
				if (vel.x != 0.0f) {
					player->dir = (int32_t)glm_signf(vel.x);
				} else if (input_x != 0) {
					player->dir = input_x;
				}
			}
		} else {
			if (SpritesEqual(player->anim.sprite, player_jump_start) && player->anim.ended) {
				SetSprite(player, player_jump_end);
			}
		}
	}

	if (SpritesEqual(player->anim.sprite, player_attack) && player->anim.ended) {
		vec2s vel = EntityVel(player);
		if (input_x == 0 && vel.x == 0.0f) {
			SetSprite(player, player_idle);
		} else {
			SetSprite(player, player_run);
			if (vel.x != 0.0f) {
				player->dir = (int32_t)glm_signf(vel.x);
			} else if (input_x != 0) {
				player->dir = input_x;
			}
		}
	}

	bool loop = true;
	if (SpritesEqual(player->anim.sprite, player_jump_start) || SpritesEqual(player->anim.sprite, player_jump_end) || SpritesEqual(player->anim.sprite, player_attack)) {
		loop = false;
	}
	UpdateAnim(ctx, &player->anim, loop);

	// TODO: Use actual level bounds.
	if (player->pos.y > 1000.0f) {
		ResetGame(ctx);
	}
}

void UpdateBoar(Context* ctx, Entity* boar) {
	if (!HAS_FLAG(boar->flags, EntityFlags_Active)) {
		return;
	}

	if (SpritesEqual(boar->anim.sprite, boar_hit)) {
		UpdateAnim(ctx, &boar->anim, false);
		if (boar->anim.ended) {
			boar->flags &= ~EntityFlags_Active;
			return;
		}
	}

	if (SpritesEqual(boar->anim.sprite, boar_attack)) {
		if (EntitiesIntersect(ctx, boar, ctx->player)) {
			--ctx->player->health;
			if (ctx->player->health <= 0) {
				SetSprite(ctx->player, player_die);
			}
		}
	} else {
		Rect hitbox, lh, rh, uh, dh;
		GetEntityHitboxes(ctx, boar, &hitbox, &lh, &rh, &uh, &dh);

		EntityMoveY(boar, GRAVITY);

		Level* level = GetCurrentLevel(ctx);

		boar->touching_floor = false;
		vec2s vel = EntityVel(boar);
		if (vel.y < 0.0f) {
			Rect tile;
			if (RectIntersectsLevel(level, uh, &tile)) {
				ENTITY_UP_COLLISION(boar);
			}
		} else if (vel.y > 0.0f) {
			Rect tile;
			if (RectIntersectsLevel(level, dh, &tile)) {
				ENTITY_DOWN_COLLISION(boar);
				boar->touching_floor = true;
			}
		}

		// BoarChasePlayer
		if (boar->touching_floor) {
			if (ctx->player->touching_floor && SDL_fabsf(boar->pos.x - ctx->player->pos.x) < TILE_SIZE*15) {
				if (boar->pos.x < ctx->player->pos.x - TILE_SIZE) {
					SetSprite(boar, boar_run);
					EntityMoveX(boar, BOAR_ACC);
					boar->dir = -1; // Boar sprite is flipped
				} else if (boar->pos.x > ctx->player->pos.x + TILE_SIZE) {
					SetSprite(boar, boar_run);
					EntityMoveX(boar, -BOAR_ACC);
					boar->dir = 1; // Boar sprite is flipped
				} else {
					SetSprite(boar, boar_attack);
				}
			}

			#if 0
			if (!EntityApplyFriction(boar, BOAR_FRIC, BOAR_MAX_VEL) && !SpritesEqual(boar->anim.sprite, boar_attack)) {
				SetSprite(boar, boar_idle);
			}
			#endif
		}

		vel = EntityVel(boar);
		if (vel.x < 0.0f) {
			Rect tile;
			if (RectIntersectsLevel(level, lh, &tile)) {
				ENTITY_LEFT_COLLISION(boar);
			}
		} else if (vel.x > 0.0f) {
			Rect tile;
			if (RectIntersectsLevel(level, rh, &tile)) {
				ENTITY_RIGHT_COLLISION(boar);
			}
		}
	}

	bool loop = true;
	if (SpritesEqual(boar->anim.sprite, boar_hit) || SpritesEqual(boar->anim.sprite, boar_attack)) {
		loop = false;
	}
	UpdateAnim(ctx, &boar->anim, loop);
	if (boar->anim.ended && SpritesEqual(boar->anim.sprite, boar_attack)) {
		SetSprite(boar, boar_idle);
		boar->anim.timer = 30;
	}
}

void GetEntityHitboxes(Context* ctx, Entity* entity, Rect* h, Rect* lh, Rect* rh, Rect* uh, Rect* dh) {
	SDL_assert(h && lh && rh && uh && dh);
	*h = GetEntityHitbox(ctx, entity);

	vec2s vel = EntityVel(entity);

	lh->min.x = entity->pos.x + h->min.x + (int32_t)SDL_floorf(vel.x);
	lh->min.y = entity->pos.y + h->min.y + 1;
	lh->max.x = entity->pos.x + h->min.x + (int32_t)SDL_floorf(vel.x) + 1;
	lh->max.y = entity->pos.y + h->max.y - 1;
	#if 0
	if (HAS_FLAG(entity->flags, EntityFlags_Player)) {
		lh->min.x -= 22;
		lh->max.x -= 22;
	}
	#endif

	rh->min.x = entity->pos.x + h->max.x + (int32_t)SDL_floorf(vel.x) - 1;
	rh->min.y = entity->pos.y + h->min.y + 1;
	rh->max.x = entity->pos.x + h->max.x + (int32_t)SDL_floorf(vel.x);
	rh->max.y = entity->pos.y + h->max.y - 1;

	uh->min.x = entity->pos.x + h->min.x + 1;
	uh->min.y = entity->pos.y + h->min.y + (int32_t)SDL_floorf(vel.y);
	uh->max.x = entity->pos.x + h->max.x - 1;
	uh->max.y = entity->pos.y + h->min.y + (int32_t)SDL_floorf(vel.y) + 1;

	dh->min.x = entity->pos.x + h->min.x + 1;
	dh->min.y = entity->pos.y + h->max.y + (int32_t)SDL_floorf(vel.y) - 1;
	dh->max.x = entity->pos.x + h->max.x - 1;
	dh->max.y = entity->pos.y + h->max.y + (int32_t)SDL_floorf(vel.y);
}

void EntityMoveX(Entity* entity, float acc) {
	float prev_pos_x = entity->pos.x;
	entity->pos.x += (entity->pos.x - entity->prev_pos.x) + acc*dt*dt;
	entity->prev_pos.x = prev_pos_x;
}

void EntityMoveY(Entity* entity, float acc) {
	float prev_pos_y = entity->pos.y;
	entity->pos.y += (entity->pos.y - entity->prev_pos.y) + acc*dt*dt;
	entity->prev_pos.y = prev_pos_y;
}

vec2s EntityVel(Entity* entity) {
	return glms_vec2_divs(glms_vec2_sub(entity->pos, entity->prev_pos), dt);
}

#if 0
bool EntityApplyFriction(Entity* entity, float fric, float max_vel) {
    float vel_save = entity->vel.x;
    if (entity->vel.x < 0.0f) entity->vel.x = SDL_min(0.0f, entity->vel.x + fric);
    else if (entity->vel.x > 0.0f) entity->vel.x = SDL_max(0.0f, entity->vel.x - fric);
    entity->vel.x = SDL_clamp(entity->vel.x, -max_vel, max_vel);
    return entity->vel.x != vel_save;
}
#endif