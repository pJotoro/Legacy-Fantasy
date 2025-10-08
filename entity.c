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

	int32_t input_dir = 0;
	if (ctx->gamepad) {
		if (ctx->gamepad_left_stick.x == 0.0f) input_dir = 0;
		else if (ctx->gamepad_left_stick.x > 0.0f) input_dir = 1;
		else if (ctx->gamepad_left_stick.x < 0.0f) input_dir = -1;
	} else {
		input_dir = ctx->button_right - ctx->button_left;
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

		if (HAS_FLAG(player->flags, EntityFlags_TouchingFloor)) {
			if (ctx->button_jump) {
				player->flags &= ~EntityFlags_TouchingFloor;
				EntityMoveY(player, -PLAYER_JUMP);
				SetSprite(player, player_jump_start);
			} else {
				float acc;
				if (!ctx->gamepad) {
					acc = (float)input_dir * PLAYER_ACC;
				} else {
					acc = ctx->gamepad_left_stick.x * PLAYER_ACC;
				}
				EntityMoveX(player, acc);
				EntityApplyFriction(player, PLAYER_FRIC, PLAYER_MAX_VEL);
			}
		} else if (ctx->button_jump_released && player->vel.y < 0.0f) {
			player->flags |= EntityFlags_JumpReleased;
			player->vel.y /= 2.0f;
		}

		if (player->vel.x < 0.0f) {
			Rect tile;
			if (RectIntersectsLevel(level, lh, &tile)) {
				player->pos.x += tile.max.x - hitbox.min.x;
				if (HAS_FLAG(player->flags, EntityFlags_TouchingFloor) && input_dir == 0) player->vel.x = 0.0f;
			} else if (!HAS_FLAG(player->flags, EntityFlags_TouchingFloor)) {
				EntityMoveX(player, 0.0f);
			}
		} else if (player->vel.x > 0.0f) {
			Rect tile;
			if (RectIntersectsLevel(level, rh, &tile)) {
				player->pos.x += tile.min.x - hitbox.max.x;
				if (HAS_FLAG(player->flags, EntityFlags_TouchingFloor) && input_dir == 0) player->vel.x = 0.0f;
			} else if (!HAS_FLAG(player->flags, EntityFlags_TouchingFloor)) {
				EntityMoveX(player, 0.0f);
			}
		}
		
		player->flags &= ~EntityFlags_TouchingFloor;
		if (player->vel.y < 0.0f) {
			Rect tile;
			if (RectIntersectsLevel(level, uh, &tile)) {
				player->pos.y += tile.max.y - hitbox.min.y + 1.0f;
				player->vel.y = 0.0f;
				SetSprite(player, player_jump_end);
			}
		} else if (player->vel.y > 0.0f) {
			Rect tile;
			if (RectIntersectsLevel(level, dh, &tile)) {
				player->pos.y += tile.min.y - hitbox.max.y;
				player->vel.y = 0.0f;
				player->flags |= EntityFlags_TouchingFloor;
				player->flags &= ~EntityFlags_JumpReleased;
			} else {
				SetSprite(player, player_jump_end);
			}
		}

		if (HAS_FLAG(player->flags, EntityFlags_TouchingFloor)) {
			if (input_dir == 0 && player->vel.x == 0.0f) {
				SetSprite(player, player_idle);
			} else {
				SetSprite(player, player_run);
				if (player->vel.x != 0.0f) {
					player->dir = (int32_t)glm_signf(player->vel.x);
				} else if (input_dir != 0) {
					player->dir = input_dir;
				}
			}
		} else {
			if (SpritesEqual(player->anim.sprite, player_jump_start) && player->anim.ended) {
				SetSprite(player, player_jump_end);
			}
		}
	}

	if (SpritesEqual(player->anim.sprite, player_attack) && player->anim.ended) {
		if (input_dir == 0 && player->vel.x == 0.0f) {
			SetSprite(player, player_idle);
		} else {
			SetSprite(player, player_run);
			if (player->vel.x != 0.0f) {
				player->dir = (int32_t)glm_signf(player->vel.x);
			} else if (input_dir != 0) {
				player->dir = input_dir;
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
			boar->pos.y += tile.max.y - hitbox.min.y;
			boar->vel.y = -boar->vel.y / 2.0f;
		}
	} else if (boar->vel.y > 0.0f) {
		Rect tile;
		if (RectIntersectsLevel(level, dh, &tile)) {
			boar->pos.y += tile.min.y - hitbox.max.y;
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
			boar->pos.x += tile.max.x - hitbox.min.x;
			boar->vel.x = 0.0f;
		}
	} else if (boar->vel.x > 0.0f) {
		Rect tile;
		if (RectIntersectsLevel(level, rh, &tile)) {
			boar->pos.x += tile.min.x - hitbox.max.x;
			boar->vel.x = 0.0f;
		}
	}

	bool loop = true;
	if (SpritesEqual(boar->anim.sprite, boar_hit)) {
		loop = false;
	}
	UpdateAnim(ctx, &boar->anim, loop);
}

Rect GetEntityHitbox(Context* ctx, Entity* entity) {
	Rect hitbox = {0};
	SpriteDesc* sd = GetSpriteDesc(ctx, entity->anim.sprite);

	/* 	
	I'll admit this part of the function is kind of weird. I might end up changing it later.
	The way it works is: we start from the current frame and go backward.
	For each frame, check if there is a corresponding hitbox. If so, pick that one.
	*/
	{
		bool res; ssize_t frame_idx;
		for (res = false, frame_idx = entity->anim.frame_idx; !res && frame_idx >= 0; --frame_idx) {
			res = GetSpriteHitbox(ctx, entity->anim.sprite, (size_t)frame_idx, entity->dir, &hitbox); 
		}
		if (!res && entity->anim.frame_idx == 0) {
			for (frame_idx = 1; !res && frame_idx < (ssize_t)sd->n_frames; ++frame_idx) {
				res = GetSpriteHitbox(ctx, entity->anim.sprite, (size_t)frame_idx, entity->dir, &hitbox);
			}
		}
		SDL_assert(res);
	}

	// AdjustEntityHitbox
	{
		ivec2s origin = GetEntityOrigin(ctx, entity);
		SDL_assert(entity->dir == 1 || entity->dir == -1);
		if (entity->dir == -1) {
			float prev_min_x = hitbox.min.x;
			hitbox.min.x = origin.x - (hitbox.max.x - origin.x);
			hitbox.max.x = hitbox.min.x + (hitbox.max.x - prev_min_x);

			// This causes the hitbox to be way to the right of where it's supposed to be while turned left.
			#if 0
			hitbox.min.x += sd->size.x;
			hitbox.max.x += sd->size.x;
			#endif
		}
		vec2s offset = glms_vec2_add(entity->pos, vec2_from_ivec2(origin));
		hitbox.min = glms_vec2_add(hitbox.min, offset);
		hitbox.max = glms_vec2_add(hitbox.max, offset);
	}

	return hitbox;
}

void GetEntityHitboxes(Context* ctx, Entity* entity, Rect* h, Rect* lh, Rect* rh, Rect* uh, Rect* dh) {
	SDL_assert(h && lh && rh && uh && dh);
	*h = GetEntityHitbox(ctx, entity);

	lh->min.x = h->min.x;
	lh->min.y = h->min.y + 1;
	lh->max.x = h->min.x;
	lh->max.y = h->max.y - 1;

	rh->min.x = h->max.x;
	rh->min.y = h->min.y + 1;
	rh->max.x = h->max.x;
	rh->max.y = h->max.y - 1;

	uh->min.x = h->min.x + 1;
	uh->min.y = h->min.y;
	uh->max.x = h->max.x - 1;
	uh->max.y = h->min.y + 1;

	dh->min.x = h->min.x + 1;
	dh->min.y = h->max.y - 1;
	dh->max.x = h->max.x - 1;
	dh->max.y = h->max.y;

	if (entity->dir == 1) {
		lh->max.x += 1;
		rh->min.x -= 1;
	} else {
		lh->min.x -= 1;
		rh->max.x += 1;
	}
}

bool EntitiesIntersect(Context* ctx, Entity* a, Entity* b) {
    if (!HAS_FLAG(a->flags, EntityFlags_Active) || !HAS_FLAG(b->flags, EntityFlags_Active)) return false;
    Rect ha = GetEntityHitbox(ctx, a);
    Rect hb = GetEntityHitbox(ctx, b);
    return RectsIntersect(ha, hb);
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

ivec2s GetEntityOrigin(Context* ctx, Entity* entity) {
	return GetSpriteOrigin(ctx, entity->anim.sprite);
}