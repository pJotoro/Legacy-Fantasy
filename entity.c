void UpdatePlayer(Context* ctx) {
	Entity* player = GetPlayer(ctx);
	Level* level = GetCurrentLevel(ctx);

	int32_t input_dir = 0;
	if (ctx->gamepad) {
		if (ctx->gamepad_left_stick.x == 0.0f) input_dir = 0;
		else if (ctx->gamepad_left_stick.x > 0.0f) input_dir = 1;
		else if (ctx->gamepad_left_stick.x < 0.0f) input_dir = -1;
	} else {
		input_dir = ctx->button_right - ctx->button_left;
	}

	if (player->pos.y > (float)level->size.y) {
		ResetGame(ctx);
		return;
	}

	switch (player->state) {
	case EntityState_Inactive:
    	return;
    case EntityState_Die: {
		SetSprite(player, player_die);
		bool loop = false;
		UpdateAnim(ctx, &player->anim, loop);
		if (player->anim.ended) {
			ResetGame(ctx);
		}
		return;
	}

    case EntityState_Attack: {
		SetSprite(player, player_attack);

		size_t n_enemies; Entity* enemies = GetEnemies(ctx, &n_enemies);
		for (size_t enemy_idx = 0; enemy_idx < n_enemies; ++enemy_idx) {
			Entity* enemy = &enemies[enemy_idx];
			if (EntitiesIntersect(ctx, player, enemy)) {
				switch (enemy->type) {
				case EntityType_Boar:
					SetSprite(enemy, boar_hit);
					break;
				default:
					break;
				}
			}
		}
		
		bool loop = false;
		UpdateAnim(ctx, &player->anim, loop);
		if (player->anim.ended) {
			player->state = EntityState_Free;
		}
		return;
	}
    	
    case EntityState_Fall: {
    	SetSprite(player, player_jump_end);
    	bool loop = false;
    	UpdateAnim(ctx, &player->anim, loop);

		EntityMoveY(player, GRAVITY);
    	/*
		if (hit_ground) {
    		player->state = EntityState_Free;
		}
    	*/
		return;
	}
    	
	case EntityState_Jump: {
		SetSprite(player, player_jump_start);
		bool loop = false;
    	UpdateAnim(ctx, &player->anim, loop);

    	EntityMoveX(player, 0.0f);
		EntityMoveY(player, GRAVITY);

		/*
		if (hit_ground) {
			player->state = EntityState_Free;
		} else if (hit_ceiling || player->vel.y >= 0.0f) {
			player->state = EntityState_Fall;
		}
		*/

		return;
	}

	case EntityState_Free: {
		if (ctx->button_attack) {
			player->state = EntityState_Attack;
		} else if (ctx->button_jump) {
			player->state = EntityState_Jump;
		} else {
			EntityMoveY(player, GRAVITY);

			float acc;
			if (!ctx->gamepad) {
				acc = (float)input_dir * PLAYER_ACC;
			} else {
				acc = ctx->gamepad_left_stick.x * PLAYER_ACC;
			}
			EntityMoveX(player, acc);
			EntityApplyFriction(player, PLAYER_FRIC, PLAYER_MAX_VEL);

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

			// TODO: collision detection
		}

		return;		
	}

	}
}

void UpdateBoar(Context* ctx, Entity* boar) {
	if (boar->state == EntityState_Inactive) {
		return;
	}

#if 0
	if (SpritesEqual(boar->anim.sprite, boar_hit)) {
		UpdateAnim(ctx, &boar->anim, false);
		if (boar->anim.ended) {
			boar->state = EntityState_Inactive;
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

#endif
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
	If no hitbox is found and the frame index is 0, loop forward instead.
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
			int32_t prev_min_x = hitbox.min.x;
			hitbox.min.x = origin.x - (hitbox.max.x - origin.x);
			hitbox.max.x = hitbox.min.x + (hitbox.max.x - prev_min_x);
		}
		ivec2s offset = glms_ivec2_add(entity->pos, origin);
		hitbox.min = glms_ivec2_add(hitbox.min, offset);
		hitbox.max = glms_ivec2_add(hitbox.max, offset);
	}

	return hitbox;
}

bool EntitiesIntersect(Context* ctx, Entity* a, Entity* b) {
    if (a->state == EntityState_Inactive || b->state == EntityState_Inactive) return false;
    Rect ha = GetEntityHitbox(ctx, a);
    Rect hb = GetEntityHitbox(ctx, b);
    return RectsIntersect(ha, hb);
}

void EntityMoveX(Entity* entity, float acc) {
	entity->pos_remainder.x += entity->vel.x;
	entity->vel.x += acc;
}

void EntityMoveY(Entity* entity, float acc) {
	entity->pos_remainder.y += entity->vel.y;
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