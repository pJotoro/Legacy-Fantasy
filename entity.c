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
	} else switch (player->state) {
	case EntityState_Inactive:
		break;
    case EntityState_Die: {
		SetSprite(player, player_die);
		bool loop = false;
		UpdateAnim(ctx, &player->anim, loop);
		if (player->anim.ended) {
			ResetGame(ctx);
		}
	} break;

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
	} break;
    	
    case EntityState_Fall: {
    	SetSprite(player, player_jump_end);

    	vec2s acc = {0.0f, GRAVITY};
    	EntityMoveAndCollide(ctx, player, acc, PLAYER_FRIC, PLAYER_MAX_VEL);

    	bool loop = false;
    	UpdateAnim(ctx, &player->anim, loop);
	} break;
    	
	case EntityState_Jump: {
		vec2s acc = {0.0f};
		if (SetSprite(player, player_jump_start)) {
			acc.y -= PLAYER_JUMP;
		}

    	acc.y += GRAVITY;
    	EntityMoveAndCollide(ctx, player, acc, PLAYER_FRIC, PLAYER_MAX_VEL);

		bool loop = false;
    	UpdateAnim(ctx, &player->anim, loop);
	} break;

	case EntityState_Free: {
		vec2s acc = {0.0f, 0.0f};

		acc.y += GRAVITY;

		if (!ctx->gamepad) {
			acc.x = (float)input_dir * PLAYER_ACC;
		} else {
			acc.x = ctx->gamepad_left_stick.x * PLAYER_ACC;
		}

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

		EntityMoveAndCollide(ctx, player, acc, PLAYER_FRIC, PLAYER_MAX_VEL);

		bool loop = true;
		UpdateAnim(ctx, &player->anim, loop);
	} break;

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

	EntityAccelerateY(boar, GRAVITY);

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

	hitbox.min = glms_ivec2_add(hitbox.min, entity->pos);
	hitbox.max = glms_ivec2_add(hitbox.max, entity->pos);

	return hitbox;
}

bool EntitiesIntersect(Context* ctx, Entity* a, Entity* b) {
    if (a->state == EntityState_Inactive || b->state == EntityState_Inactive) return false;
    Rect ha = GetEntityHitbox(ctx, a);
    Rect hb = GetEntityHitbox(ctx, b);
    return RectsIntersect(ha, hb);
}

// Why does this not come with glm?
FORCEINLINE vec2s glms_vec2_round(vec2s v) {
	v.x = SDL_roundf(v.x);
	v.y = SDL_roundf(v.y);
	return v;
}

FORCEINLINE ssize_t GetTileIdx(Level* level, ivec2s pos) {
	return (ssize_t)((pos.x + pos.y*level->size.x)/TILE_SIZE);
}

/*
Do this in two passes. The second pass should be what is already below, more or less. In the first pass, check to see if there is a tile exactly one pixel away from the player, before applying velocity. If there is, don't apply velocity (or accelerate?) in that direction, and don't check for a collision in that direction. Simple enough, right?

We wouldn't have been able to do it that way before, so it was good that we changed the way tiles worked. The problem with the way it worked before was that tiles weren't placed in the tiles array based on where they were in the level. As a result, there was no way to access them efficiently. You had to loop through every single tile for every single entity. Now that we've changed how tiles work, we can easily loop through the small amount of tiles necessary twice, no problem. It makes things much simpler.
*/

void EntityMoveAndCollide(Context* ctx, Entity* entity, vec2s acc, float fric, float max_vel) {
	Level* level = GetCurrentLevel(ctx);
	size_t n_tiles; Tile* tiles = GetLevelTiles(level, &n_tiles);
	Rect prev_hitbox = GetEntityHitbox(ctx, entity);

	entity->vel = glms_vec2_add(entity->vel, acc);

	if (entity->state == EntityState_Free) {
		if (entity->vel.x < 0.0f) entity->vel.x = SDL_min(0.0f, entity->vel.x + fric);
		else if (entity->vel.x > 0.0f) entity->vel.x = SDL_max(0.0f, entity->vel.x - fric);
		entity->vel.x = SDL_clamp(entity->vel.x, -max_vel, max_vel);
	}

	bool horizontal_collision_happened = false;
	vec2s vel = entity->vel;
	if (entity->vel.x < 0.0f && prev_hitbox.min.x % TILE_SIZE == 0) {
		ivec2s grid_pos;
		grid_pos.x = (prev_hitbox.min.x-TILE_SIZE)/TILE_SIZE;
		for (grid_pos.y = prev_hitbox.min.y/TILE_SIZE; grid_pos.y <= prev_hitbox.max.y/TILE_SIZE; ++grid_pos.y) {
			size_t tile_idx = (size_t)(grid_pos.x + grid_pos.y*level->size.x);
			SDL_assert(tile_idx < n_tiles);
			Tile tile = tiles[tile_idx];
			if (tile.type == TileType_Level) {
				vel.x = 0.0f;
				horizontal_collision_happened = true;
				break;
			}
		}
	} else if (entity->vel.x > 0.0f && (prev_hitbox.max.x+1) % TILE_SIZE == 0) {
		ivec2s grid_pos;
		grid_pos.x = (prev_hitbox.max.x+1)/TILE_SIZE;
		for (grid_pos.y = prev_hitbox.min.y/TILE_SIZE; grid_pos.y <= prev_hitbox.max.y/TILE_SIZE; ++grid_pos.y) {
			size_t tile_idx = (size_t)(grid_pos.x + grid_pos.y*level->size.x);
			SDL_assert(tile_idx < n_tiles);
			Tile tile = tiles[tile_idx];
			if (tile.type == TileType_Level) {
				vel.x = 0.0f;
				horizontal_collision_happened = true;
				break;
			}
		}
	}
	bool touching_floor = false;
	bool vertical_collision_happened = false;
	if (entity->vel.y < 0.0f && prev_hitbox.min.y % TILE_SIZE == 0) {
		ivec2s grid_pos;
		grid_pos.y = (prev_hitbox.min.y-TILE_SIZE)/TILE_SIZE;
		for (grid_pos.x = prev_hitbox.min.x/TILE_SIZE; grid_pos.x <= prev_hitbox.max.x/TILE_SIZE; ++grid_pos.x) {
			size_t tile_idx = (size_t)(grid_pos.x + grid_pos.y*level->size.x);
			SDL_assert(tile_idx < n_tiles);
			Tile tile = tiles[tile_idx];
			if (tile.type == TileType_Level) {
				vel.y = 0.0f;
				vertical_collision_happened = true;
				break;
			}
		}
	} else if (entity->vel.y > 0.0f && (prev_hitbox.max.y+1) % TILE_SIZE == 0) {
		ivec2s grid_pos;
		grid_pos.y = (prev_hitbox.max.y+1)/TILE_SIZE;
		for (grid_pos.x = prev_hitbox.min.x/TILE_SIZE; grid_pos.x <= prev_hitbox.max.x/TILE_SIZE; ++grid_pos.x) {
			size_t tile_idx = (size_t)(grid_pos.x + grid_pos.y*level->size.x);
			SDL_assert(tile_idx < n_tiles);
			Tile tile = tiles[tile_idx];
			if (tile.type == TileType_Level) {
				vel.y = 0.0f;
				entity->vel.y = 0.0f;
				entity->state = EntityState_Free;
				touching_floor = true;
				vertical_collision_happened = true;
				break;
			}
		}
	}

    entity->pos_remainder = glms_vec2_add(entity->pos_remainder, vel);
    entity->pos = glms_ivec2_add(entity->pos, ivec2_from_vec2(glms_vec2_round(entity->pos_remainder)));
    entity->pos_remainder = glms_vec2_sub(entity->pos_remainder, glms_vec2_round(entity->pos_remainder));

    Rect hitbox = GetEntityHitbox(ctx, entity);

	ivec2s grid_pos;
	for (grid_pos.y = hitbox.min.y/TILE_SIZE; (!horizontal_collision_happened || !vertical_collision_happened) && grid_pos.y <= hitbox.max.y/TILE_SIZE; ++grid_pos.y) {
		for (grid_pos.x = hitbox.min.x/TILE_SIZE; (!horizontal_collision_happened || !vertical_collision_happened) && grid_pos.x <= hitbox.max.x/TILE_SIZE; ++grid_pos.x) {
			size_t tile_idx = (size_t)(grid_pos.x + grid_pos.y*level->size.x);
			SDL_assert(tile_idx < n_tiles);
			Tile tile = tiles[tile_idx];
			if (tile.type == TileType_Level) {
				Rect tile_rect;
				tile_rect.min = glms_ivec2_scale(grid_pos, TILE_SIZE);
				tile_rect.max = glms_ivec2_adds(tile_rect.min, TILE_SIZE);
				if (RectsIntersect(hitbox, tile_rect)) {
					if (RectsIntersect(prev_hitbox, tile_rect)) continue;

					if (!horizontal_collision_happened) {
						Rect h = prev_hitbox;
						h.min.x = hitbox.min.x;
						h.max.x = hitbox.max.x;
						if (RectsIntersect(h, tile_rect)) {
							int32_t amount = 0;
							int32_t incr = (int32_t)glm_signf(entity->vel.x);
							while (RectsIntersect(h, tile_rect)) {
								h.min.x -= incr;
								h.max.x -= incr;
								amount += incr;
							}
							entity->pos.x -= amount;
							horizontal_collision_happened = true;
						}

					}
					if (!vertical_collision_happened) {
						Rect h = prev_hitbox;
						h.min.y = hitbox.min.y;
						h.max.y = hitbox.max.y;
						if (RectsIntersect(h, tile_rect)) {
							int32_t amount = 0;
							int32_t incr = (int32_t)glm_signf(entity->vel.y);
							while (RectsIntersect(h, tile_rect)) {
								h.min.y -= incr;
								h.max.y -= incr;
								amount += incr;
							}
							entity->pos.y -= amount;
							
							vertical_collision_happened = true;
						}
					}
				}
			}
		}
	}

	if (!touching_floor && entity->vel.y > 0.0f) {
		entity->state = EntityState_Fall;
	} else if (entity->state == EntityState_Free && ctx->button_jump) {
		entity->state = EntityState_Jump;
	}
}