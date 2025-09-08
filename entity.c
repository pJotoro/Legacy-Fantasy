void UpdatePlayer(Context* ctx, Entity* player) {
	if (player->touching_floor && ctx->button_attack) {
		SetSprite(player, player_attack);
	}

	int32_t input_x = 0;
	vec2s conserved_vel = {0.0f, 0.0f};
	if (ctx->gamepad) {
		if (ctx->gamepad_left_stick.x == 0.0f) input_x = 0;
		else if (ctx->gamepad_left_stick.x > 0.0f) input_x = 1;
		else if (ctx->gamepad_left_stick.x < 0.0f) input_x = -1;
	} else {
		input_x = ctx->button_right - ctx->button_left;
	}

	if (SpritesEqual(player->anim.sprite, player_attack)) {
		size_t n_entities; Entity* entities = GetEntities(ctx, &n_entities);
		for (size_t entity_idx = 0; entity_idx < n_entities; ++entity_idx) {
			if (HAS_FLAG(entities[entity_idx].flags, EntityFlags_Enemy) && HAS_FLAG(entities[entity_idx].flags, EntityFlags_Active)) {
				Entity* enemy = &entities[entity_idx];
				if (EntitiesIntersect(ctx, player, enemy)) {
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
		player->vel.y += GRAVITY;
		player->touching_floor = SDL_max(player->touching_floor - 1, 0);

		if (player->touching_floor) {
			if (ctx->button_jump) {
				player->touching_floor = 0;
				player->vel.y = -PLAYER_JUMP;
				SetSprite(player, player_jump_start);
			} else {
				float acc;
				if (!ctx->gamepad) {
					acc = (float)input_x * PLAYER_ACC;
				} else {
					acc = ctx->gamepad_left_stick.x * PLAYER_ACC;
				}
				player->vel.x += acc;
				EntityApplyFriction(player, PLAYER_FRIC, PLAYER_MAX_VEL);
			
			}	
		} else if (ctx->button_jump_released && !HAS_FLAG(player->flags, EntityFlags_JumpReleased) && player->vel.y < 0.0f) {
			player->flags |= EntityFlags_JumpReleased;
			player->vel.y /= 2.0f;
		}

		Level* level = GetCurrentLevel(ctx);
		if (player->vel.x < 0.0f) {
			Rect tile;
			if (RectIntersectsLevel(level, lh, &tile)) {
				// conserved_vel.x = player->vel.x;
				ENTITY_LEFT_COLLISION(player);
			}
		} else if (player->vel.x > 0.0f) {
			Rect tile;
			if (RectIntersectsLevel(level, rh, &tile)) {
				// conserved_vel.x = player->vel.x;
				ENTITY_RIGHT_COLLISION(player);
			}
		}
		if (player->vel.y < 0.0f) {
			Rect tile;
			if (RectIntersectsLevel(level, uh, &tile)) {
				ENTITY_UP_COLLISION(player);
			}
		} else if (player->vel.y > 0.0f) {
			Rect tile;
			if (RectIntersectsLevel(level, dh, &tile)) {
				ENTITY_DOWN_COLLISION(player);
				player->touching_floor = PLAYER_JUMP_REMAINDER;
				player->flags &= ~EntityFlags_JumpReleased;
			} else {
				SetSprite(player, player_jump_end);
			}
		}

		MoveEntity(player);

		if (player->touching_floor) {
			if (input_x == 0 && player->vel.x == 0.0f) {
				SetSprite(player, player_idle);
			} else {
				SetSprite(player, player_run);
				if (player->vel.x != 0.0f) {
					player->dir = (int32_t)glm_signf(player->vel.x);
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
		if (input_x == 0 && player->vel.x == 0.0f) {
			SetSprite(player, player_idle);
		} else {
			SetSprite(player, player_run);
			if (player->vel.x != 0.0f) {
				player->dir = (int32_t)glm_signf(player->vel.x);
			} else if (input_x != 0) {
				player->dir = input_x;
			}
		}
	}

	if (conserved_vel.x != 0.0f) player->vel.x = conserved_vel.x;
	if (conserved_vel.y != 0.0f) player->vel.y = conserved_vel.y;

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
	if (SpritesEqual(boar->anim.sprite, boar_hit)) {
		if (boar->anim.ended) {
			boar->flags &= ~EntityFlags_Active;
		}
	} else {
		Rect hitbox, lh, rh, uh, dh;
		GetEntityHitboxes(ctx, boar, &hitbox, &lh, &rh, &uh, &dh);

		boar->vel.y += GRAVITY;

		Level* level = GetCurrentLevel(ctx);

		boar->touching_floor = false;
		if (boar->vel.y < 0.0f) {
			Rect tile;
			if (RectIntersectsLevel(level, uh, &tile)) {
				ENTITY_UP_COLLISION(boar);
			}
		} else if (boar->vel.y > 0.0f) {
			Rect tile;
			if (RectIntersectsLevel(level, dh, &tile)) {
				ENTITY_DOWN_COLLISION(boar);
				boar->touching_floor = true;
			}
		}

		// BoarChasePlayer
		if (boar->touching_floor) {
			if (ctx->player->touching_floor && SDL_abs(boar->pos.x - ctx->player->pos.x) < TILE_SIZE*15) {
				if (boar->pos.x < ctx->player->pos.x - TILE_SIZE) {
					SetSprite(boar, boar_run);
					boar->vel.x += BOAR_ACC;
					boar->vel.x = SDL_min(boar->vel.x, BOAR_MAX_VEL);
					boar->dir = -1; // Boar sprite is flipped
				} else if (boar->pos.x > ctx->player->pos.x + TILE_SIZE) {
					SetSprite(boar, boar_run);
					boar->vel.x -= BOAR_ACC;
					boar->vel.x = SDL_max(boar->vel.x, -BOAR_MAX_VEL);
					boar->dir = 1; // Boar sprite is flipped
				}
			}

			if (!EntityApplyFriction(boar, BOAR_FRIC, BOAR_MAX_VEL)) {
				SetSprite(boar, boar_idle);
			}
		}

		if (boar->vel.x < 0.0f) {
			Rect tile;
			if (RectIntersectsLevel(level, lh, &tile)) {
				ENTITY_LEFT_COLLISION(boar);
			}
		} else if (boar->vel.x > 0.0f) {
			Rect tile;
			if (RectIntersectsLevel(level, rh, &tile)) {
				ENTITY_RIGHT_COLLISION(boar);
			}
		}

		MoveEntity(boar);
	}

	bool loop = true;
	if (SpritesEqual(boar->anim.sprite, boar_hit)) {
		loop = false;
	}
	UpdateAnim(ctx, &boar->anim, loop);
}

void GetEntityHitboxes(Context* ctx, Entity* entity, Rect* h, Rect* lh, Rect* rh, Rect* uh, Rect* dh) {
	SDL_assert(h && lh && rh && uh && dh);
	*h = GetEntityHitbox(ctx, entity);

	lh->min.x = entity->pos.x + h->min.x + (int32_t)SDL_floorf(entity->vel.x);
	lh->min.y = entity->pos.y + h->min.y + 1;
	lh->max.x = entity->pos.x + h->min.x + (int32_t)SDL_floorf(entity->vel.x) + 1;
	lh->max.y = entity->pos.y + h->max.y - 1;
	if (HAS_FLAG(entity->flags, EntityFlags_Player)) {
		lh->min.x -= 22;
		lh->max.x -= 22;
	}

	rh->min.x = entity->pos.x + h->max.x + (int32_t)SDL_floorf(entity->vel.x) - 1;
	rh->min.y = entity->pos.y + h->min.y + 1;
	rh->max.x = entity->pos.x + h->max.x + (int32_t)SDL_floorf(entity->vel.x);
	rh->max.y = entity->pos.y + h->max.y - 1;

	uh->min.x = entity->pos.x + h->min.x + 1;
	uh->min.y = entity->pos.y + h->min.y + (int32_t)SDL_floorf(entity->vel.y);
	uh->max.x = entity->pos.x + h->max.x - 1;
	uh->max.y = entity->pos.y + h->min.y + (int32_t)SDL_floorf(entity->vel.y) + 1;

	dh->min.x = entity->pos.x + h->min.x + 1;
	dh->min.y = entity->pos.y + h->max.y + (int32_t)SDL_floorf(entity->vel.y) - 1;
	dh->max.x = entity->pos.x + h->max.x - 1;
	dh->max.y = entity->pos.y + h->max.y + (int32_t)SDL_floorf(entity->vel.y);
}

void MoveEntity(Entity* entity) {
    entity->pos_remainder = glms_vec2_add(entity->pos_remainder, entity->vel);
    vec2s move = glms_vec2_floor(entity->pos_remainder);
    entity->pos = glms_ivec2_add(entity->pos, ivec2_from_vec2(move));
    entity->pos_remainder = glms_vec2_sub(entity->pos_remainder, move);
}

bool EntityApplyFriction(Entity* entity, float fric, float max_vel) {
    float vel_save = entity->vel.x;
    if (entity->vel.x < 0.0f) entity->vel.x = SDL_min(0.0f, entity->vel.x + fric);
    else if (entity->vel.x > 0.0f) entity->vel.x = SDL_max(0.0f, entity->vel.x - fric);
    entity->vel.x = SDL_clamp(entity->vel.x, -max_vel, max_vel);
    return entity->vel.x != vel_save;
}
