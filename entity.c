void UpdatePlayer(Context* ctx) {
	Entity* player = GetPlayer(ctx);
	Level* level = GetCurrentLevel(ctx);

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

	if (HAS_FLAG(player->flags, EntityFlags_TouchingFloor) && ctx->button_attack) {
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
				if (HAS_FLAG(enemy->flags, EntityFlags_Boar)) {
					SetSprite(enemy, boar_hit);
				} else {
					// TODO
				}
			}
		}
	} else {
		// PlayerCollision
		Rect hitbox, lh, rh, uh, dh;
		GetEntityHitboxes(ctx, player, &hitbox, &lh, &rh, &uh, &dh);
		EntityMoveY(player, GRAVITY);

		player->coyote_time = SDL_max(player->coyote_time - dt, 0.0f);

		if (player->coyote_time > 0.0f && ctx->button_jump) {
			player->coyote_time = 0.0f;
			player->flags &= ~EntityFlags_TouchingFloor;
			EntityMoveY(player, -PLAYER_JUMP);
			SetSprite(player, player_jump_start);
		} else if (HAS_FLAG(player->flags, EntityFlags_TouchingFloor)) {
			float acc;
			if (!ctx->gamepad) {
				acc = (float)input_x * PLAYER_ACC;
			} else {
				acc = ctx->gamepad_left_stick.x * PLAYER_ACC;
			}
			EntityMoveX(player, acc);
			EntityApplyFriction(player, PLAYER_FRIC, PLAYER_MAX_VEL);
		} else if (ctx->button_jump_released && player->vel.y < 0.0f) {
			player->flags |= EntityFlags_JumpReleased;
			player->vel.y /= 2.0f;
		}

		if (player->vel.x < 0.0f) {
			Rect tile;
			if (RectIntersectsLevel(level, lh, &tile)) {
				player->pos.x = SDL_max(player->pos.x, tile.max.x - hitbox.min.x);
				if (HAS_FLAG(player->flags, EntityFlags_TouchingFloor) && input_x == 0) player->vel.x = 0.0f;
			} else if (!HAS_FLAG(player->flags, EntityFlags_TouchingFloor)) {
				EntityMoveX(player, 0.0f);
			}
		} else if (player->vel.x > 0.0f) {
			Rect tile;
			if (RectIntersectsLevel(level, rh, &tile)) {
				player->pos.x = SDL_min(player->pos.x, tile.min.x - hitbox.max.x);
				if (HAS_FLAG(player->flags, EntityFlags_TouchingFloor) && input_x == 0) player->vel.x = 0.0f;
			} else if (!HAS_FLAG(player->flags, EntityFlags_TouchingFloor)) {
				EntityMoveX(player, 0.0f);
			}
		}
		if (player->vel.y < 0.0f) {
			Rect tile;
			if (RectIntersectsLevel(level, uh, &tile)) {
				player->pos.y = SDL_max(player->pos.y, tile.max.y - hitbox.min.y);
				player->vel.y = -player->vel.y/2.0f;
				SetSprite(player, player_jump_end);
			}
		} else if (player->vel.y > 0.0f) {
			Rect tile;
			if (RectIntersectsLevel(level, dh, &tile)) {
				if (dh.min.x != 0.0f && dh.min.y != 0.0f && dh.max.x != 0.0f && dh.max.y != 0.0f) {
					SDL_Log("Collision: hitbox = {%d,%d,%d,%d}, tile = {%d,%d,%d,%d}", dh.min.x, dh.min.y, dh.max.x-dh.min.x, dh.max.y-dh.min.y, tile.min.x, tile.min.y, tile.max.x-tile.min.x, tile.max.y-tile.min.y);
				}
				

				player->pos.y = SDL_min(player->pos.y, tile.min.y - hitbox.max.y);
				player->vel.y = 0.0f;
				player->coyote_time = PLAYER_JUMP_REMAINDER;
				player->flags |= EntityFlags_TouchingFloor;
				player->flags &= ~EntityFlags_JumpReleased;
			} else {
				SetSprite(player, player_jump_end);
			}
		}

		if (player->coyote_time > 0.0f) {
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

	bool loop = true;
	if (SpritesEqual(player->anim.sprite, player_jump_start) || SpritesEqual(player->anim.sprite, player_jump_end) || SpritesEqual(player->anim.sprite, player_attack)) {
		loop = false;
	}
	UpdateAnim(ctx, &player->anim, loop);

	if (player->pos.y > (float)level->size.y) {
		ResetGame(ctx);
		return;
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
		}
		return;
	}

	Rect hitbox, lh, rh, uh, dh;
	GetEntityHitboxes(ctx, boar, &hitbox, &lh, &rh, &uh, &dh);

	EntityMoveY(boar, GRAVITY);

	Level* level = GetCurrentLevel(ctx);

	boar->flags &= ~EntityFlags_TouchingFloor;
	if (boar->vel.y < 0.0f) {
		Rect tile;
		if (RectIntersectsLevel(level, uh, &tile)) {
			boar->pos.y = SDL_max(boar->pos.y, tile.max.y - hitbox.min.y);
			boar->vel.y = -boar->vel.y / 2.0f;
		}
	} else if (boar->vel.y > 0.0f) {
		Rect tile;
		if (RectIntersectsLevel(level, dh, &tile)) {
			boar->pos.y = SDL_min(boar->pos.y, tile.min.y - hitbox.max.y);
			boar->vel.y = 0.0f;
			boar->flags |= EntityFlags_TouchingFloor;
		}
	}

	// BoarChasePlayer
	if (HAS_FLAG(boar->flags, EntityFlags_TouchingFloor)) {
		if (!EntityApplyFriction(boar, BOAR_FRIC, BOAR_MAX_VEL) && !SpritesEqual(boar->anim.sprite, boar_attack)) {
			SetSprite(boar, boar_idle);
		}
	}

	if (boar->vel.x < 0.0f) {
		Rect tile;
		if (RectIntersectsLevel(level, lh, &tile)) {
			boar->pos.x = SDL_max(boar->pos.x, tile.max.x - hitbox.min.x);
			boar->vel.x = 0.0f;
		}
	} else if (boar->vel.x > 0.0f) {
		Rect tile;
		if (RectIntersectsLevel(level, rh, &tile)) {
			boar->pos.x = SDL_min(boar->pos.x, tile.min.x - hitbox.max.x);
			boar->vel.x = 0.0f;
		}
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

	lh->min.x = entity->pos.x + entity->dir*h->min.x;
	lh->min.y = entity->pos.y + h->min.y + 1;
	lh->max.x = entity->pos.x + entity->dir*h->min.x + 1;
	lh->max.y = entity->pos.y + h->max.y - 1;

	rh->min.x = entity->pos.x + entity->dir*h->max.x - 1;
	rh->min.y = entity->pos.y + h->min.y + 1;
	rh->max.x = entity->pos.x + entity->dir*h->max.x;
	rh->max.y = entity->pos.y + h->max.y - 1;

	uh->min.x = entity->pos.x + entity->dir*h->min.x + 1;
	uh->min.y = entity->pos.y + h->min.y;
	uh->max.x = entity->pos.x + entity->dir*h->max.x - 1;
	uh->max.y = entity->pos.y + h->min.y + 1;

	dh->min.x = entity->pos.x + entity->dir*h->min.x + 1;
	dh->min.y = entity->pos.y + h->max.y - 1;
	dh->max.x = entity->pos.x + entity->dir*h->max.x - 1;
	dh->max.y = entity->pos.y + h->max.y;
}

void EntityMoveX(Entity* entity, float acc) {
	entity->pos.x += entity->vel.x;
	entity->vel.x += acc;
}

void EntityMoveY(Entity* entity, float acc) {
	entity->pos.y += entity->vel.y;
	entity->vel.y += acc;
}

bool EntityApplyFriction(Entity* entity, float fric, float max_vel) {
    float vel_save = entity->vel.x;
    if (entity->vel.x < 0.0f) entity->vel.x = SDL_min(0.0f, entity->vel.x + fric);
    else if (entity->vel.x > 0.0f) entity->vel.x = SDL_max(0.0f, entity->vel.x - fric);
    entity->vel.x = SDL_clamp(entity->vel.x, -max_vel, max_vel);
    return entity->vel.x != vel_save;
}
