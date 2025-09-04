#include <SDL.h>
#include <SDL_main.h>
#include <SDL_ttf.h>

#include <cglm/struct.h>

#include <raddbg_markup.h>

#define XXH_STATIC_LINKING_ONLY /* access advanced declarations */
#define XXH_IMPLEMENTATION      /* access definitions */
#include <xxhash.h>

typedef int64_t ssize_t;

#include "infl.h"

#define DBL_EPSILON 2.2204460492503131e-016
#include "json.h"

#include "aseprite.h"

#include "main.h"

#include "util.c"
#include "level.c"
#include "sprite.c"

static Sprite player_idle;
static Sprite player_run;
static Sprite player_jump_start;
static Sprite player_jump_end;
static Sprite player_attack;

static Sprite boar_idle;
static Sprite boar_walk;
static Sprite boar_run;
static Sprite boar_attack;
static Sprite boar_hit;

void InitSprites(void) {
	player_idle = GetSprite("assets\\legacy_fantasy_high_forest\\Character\\Idle\\Idle.aseprite");
	player_run = GetSprite("assets\\legacy_fantasy_high_forest\\Character\\Run\\Run.aseprite");
	player_jump_start = GetSprite("assets\\legacy_fantasy_high_forest\\Character\\Jump-Start\\Jump-Start.aseprite");
	player_jump_end = GetSprite("assets\\legacy_fantasy_high_forest\\Character\\Jump-End\\Jump-End.aseprite");
	player_attack = GetSprite("assets\\legacy_fantasy_high_forest\\Character\\Attack-01\\Attack-01.aseprite");

	boar_idle = GetSprite("assets\\legacy_fantasy_high_forest\\Mob\\Boar\\Idle\\Idle.aseprite");
	boar_walk = GetSprite("assets\\legacy_fantasy_high_forest\\Mob\\Boar\\Walk\\Walk-Base.aseprite");
	boar_run = GetSprite("assets\\legacy_fantasy_high_forest\\Mob\\Boar\\Run\\Run.aseprite");
	boar_attack = GetSprite("assets\\legacy_fantasy_high_forest\\Mob\\Boar\\Attack\\Attack.aseprite");
	boar_hit = GetSprite("assets\\legacy_fantasy_high_forest\\Mob\\Boar\\Hit-Vanish\\Hit.aseprite");
}

void ResetGame(Context* ctx) {
	ctx->dt = ctx->display_mode->refresh_rate;
	ctx->level_idx = 0;
	for (size_t level_idx = 0; level_idx < ctx->n_levels; ++level_idx) {
		Level* level = GetCurrentLevel(ctx);
		for (size_t entity_idx = 0; entity_idx < level->n_entities; ++entity_idx) {
			Entity* entity = &level->entities[entity_idx];
			entity->flags |= EntityFlags_Active;
			entity->pos = entity->start_pos;
			entity->pos_remainder = (vec2s){0.0f, 0.0f};
			entity->vel = (vec2s){0, 0};
			entity->dir = 1;
			if (HAS_FLAG(entity->flags, EntityFlags_Player)) {
				entity->touching_floor = PLAYER_JUMP_REMAINDER;
				SetSpriteFromPath(entity, "assets\\legacy_fantasy_high_forest\\Character\\Idle\\Idle.aseprite");
				ctx->player = entity; // TODO: Don't just do this in ResetGame. Also do it when changing levels.
			} else if (HAS_FLAG(entity->flags, EntityFlags_Boar)) {
				SetSpriteFromPath(entity, "assets\\legacy_fantasy_high_forest\\Mob\\Boar\\Idle\\Idle.aseprite");
			}
		}
	}
}

int32_t main(int32_t argc, char* argv[]) {
	UNUSED(argc);
	UNUSED(argv);

	SDL_CHECK(SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD));
	SDL_CHECK(TTF_Init());

	Context* ctx = SDL_calloc(1, sizeof(Context)); SDL_CHECK(ctx);

	InitSprites();

	// LoadLevels
	{
		JSON_Node* head;
		{
			size_t file_len;
			void* file_data = SDL_LoadFile("assets\\levels\\test.ldtk", &file_len); SDL_CHECK(file_data);
			head = JSON_ParseWithLength((const char*)file_data, file_len);
			SDL_free(file_data);
		}
		SDL_assert(HAS_FLAG(head->type, JSON_Object));

		JSON_Node* levels = JSON_GetObjectItem(head, "levels", true);
		JSON_Node* level; JSON_ArrayForEach(level, levels) {
			++ctx->n_levels;
		}
		ctx->levels = SDL_calloc(ctx->n_levels, sizeof(Level)); SDL_CHECK(ctx->levels);
		size_t level_idx = 0;
		JSON_ArrayForEach(level, levels) {
			ctx->levels[level_idx++] = LoadLevel(level);
		}

		int32_t* tiles_collide = NULL; size_t n_tiles_collide = 0;
		JSON_Node* defs = JSON_GetObjectItem(head, "defs", true);
		JSON_Node* tilesets = JSON_GetObjectItem(defs, "tilesets", true);
		bool break_all = false;
		JSON_Node* tileset; JSON_ArrayForEach(tileset, tilesets) {
			JSON_Node* enum_tags = JSON_GetObjectItem(tileset, "enumTags", true);
			JSON_Node* enum_tag; JSON_ArrayForEach(enum_tag, enum_tags) {
				JSON_Node* id_node = JSON_GetObjectItem(enum_tag, "enumValueId", true);
				char* id = JSON_GetStringValue(id_node);
				if (SDL_strcmp(id, "Collide") != 0) continue;
				
				JSON_Node* tile_ids = JSON_GetObjectItem(enum_tag, "tileIds", true);
				JSON_Node* tile_id; JSON_ArrayForEach(tile_id, tile_ids) {
					++n_tiles_collide;
				}
				tiles_collide = SDL_calloc(n_tiles_collide, sizeof(int32_t)); SDL_CHECK(tiles_collide);
				size_t i = 0;
				JSON_ArrayForEach(tile_id, tile_ids) {
					tiles_collide[i++] = JSON_GetIntValue(tile_id);
				}
				break_all = true;
				break;
			}
			if (break_all) break;
		}
		break_all = false;

		for (size_t level_idx = 0; level_idx < ctx->n_levels; ++level_idx) {
			Level* level = &ctx->levels[level_idx];
			for (size_t entity_idx = 0; entity_idx < level->n_entities; ++entity_idx) {
				Entity* entity = &level->entities[entity_idx];
				if (HAS_FLAG(entity->flags, EntityFlags_Tile)) {
					ivec2s src = entity->src_pos;
					for (size_t tiles_collide_idx = 0; tiles_collide_idx < n_tiles_collide; ++tiles_collide_idx) {
						int32_t i = tiles_collide[tiles_collide_idx];
						int32_t j = (src.x + src.y*25)/TILE_SIZE; // TODO: Replace 25 with tileset width.
						if (i == j) {
							entity->flags |= EntityFlags_Solid;
							break;
						}
					}
				}
			}
		}

		SDL_free(tiles_collide);
		JSON_Delete(head);
	}

	// InitTime
	{
		SDL_CHECK(SDL_GetCurrentTime(&ctx->time));
	}

	// CreateWindowAndRenderer
	{
		SDL_DisplayID display = SDL_GetPrimaryDisplay();
		ctx->display_mode = (SDL_DisplayMode *)SDL_GetDesktopDisplayMode(display); SDL_CHECK(ctx->display_mode);
		SDL_WindowFlags flags = SDL_WINDOW_HIGH_PIXEL_DENSITY;
		int32_t w = ctx->display_mode->w / 2;
		int32_t h = ctx->display_mode->h / 2;

#if FULLSCREEN
		flags |= SDL_WINDOW_FULLSCREEN;
		w = ctx->display_mode->w;
		h = ctx->display_mode->h;
#endif

		SDL_CHECK(SDL_CreateWindowAndRenderer("LegacyFantasy", w, h, flags, &ctx->window, &ctx->renderer));

		ctx->vsync = SDL_SetRenderVSync(ctx->renderer, SDL_RENDERER_VSYNC_ADAPTIVE);
		if (!ctx->vsync) {
			ctx->vsync = SDL_SetRenderVSync(ctx->renderer, 1); // fixed refresh rate
		}

		ctx->display_content_scale = SDL_GetDisplayContentScale(display);

		SDL_SetRenderDrawBlendMode(ctx->renderer, SDL_BLENDMODE_BLEND);
	}

#if 0
	// InitTextEngine
	{
		ctx->text_engine = TTF_CreateRendererTextEngine(ctx->renderer); SDL_CHECK(ctx->text_engine);
	}

	// LoadFont
	{
		SDL_PropertiesID props = SDL_CreateProperties(); SDL_CHECK(props);
		SDL_SetStringProperty(props, TTF_PROP_FONT_CREATE_FILENAME_STRING, "assets\\fonts\\Roboto-Regular.ttf");
		SDL_SetFloatProperty(props, TTF_PROP_FONT_CREATE_SIZE_FLOAT, 15.0f);
		SDL_SetNumberProperty(props, TTF_PROP_FONT_CREATE_HORIZONTAL_DPI_NUMBER, (int64_t)(72.0f*ctx->display_content_scale));
		SDL_SetNumberProperty(props, TTF_PROP_FONT_CREATE_VERTICAL_DPI_NUMBER, (int64_t)(72.0f*ctx->display_content_scale));
		ctx->font_roboto_regular = TTF_OpenFontWithProperties(props); SDL_CHECK(ctx->font_roboto_regular);
		SDL_DestroyProperties(props);
	}
#endif

	SDL_CHECK(SDL_EnumerateDirectory("assets\\legacy_fantasy_high_forest", EnumerateDirectoryCallback, ctx));

	for (size_t sprite_idx = 0; sprite_idx < MAX_SPRITES; ++sprite_idx) {
		SpriteDesc* sd = &ctx->sprites[sprite_idx];
		if (sd->path) {
			for (size_t frame_idx = 0; frame_idx < sd->n_frames; ++frame_idx) {
				SpriteFrame* sf = &sd->frames[frame_idx];
				SDL_qsort(sf->cells, sf->n_cells, sizeof(SpriteCell), (SDL_CompareCallback)CompareSpriteCells);
			}
		}
	}

	ResetGame(ctx);

	ctx->running = true;
	while (ctx->running) {
		if (!ctx->gamepad) {
			int joystick_count = 0;
			SDL_JoystickID* joysticks = SDL_GetGamepads(&joystick_count);
			if (joystick_count != 0) {
				ctx->gamepad = SDL_OpenGamepad(joysticks[0]);
			}
		}

		ctx->button_jump = false;
		ctx->button_jump_released = false;
		ctx->button_attack = false;

		ctx->left_mouse_pressed = false;

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_EVENT_KEY_DOWN:
				if (event.key.key == SDLK_ESCAPE) {
					ctx->running = false;
				}
				if (!ctx->gamepad) {
					switch (event.key.key) {
					case SDLK_0:
						break;
					case SDLK_X:
						ctx->button_attack = true;
						break;
					case SDLK_SPACE:
						break;
					case SDLK_LEFT:
						if (!event.key.repeat) {
							ctx->button_left = 1;
						}
						break;
					case SDLK_RIGHT:
						if (!event.key.repeat) {
							ctx->button_right = 1;
						}
						break;
					case SDLK_UP:
						if (!event.key.repeat) {
							ctx->button_jump = true;
						}
						break;
					case SDLK_DOWN:
						break;
					case SDLK_R:
						ResetGame(ctx);
						break;
					}
				}
				break;
			case SDL_EVENT_MOUSE_MOTION:
				ctx->mouse_pos.x = event.motion.x;
				ctx->mouse_pos.y = event.motion.y;
				break;
			case SDL_EVENT_MOUSE_BUTTON_DOWN:
				if (event.button.button == SDL_BUTTON_LEFT) {
					ctx->left_mouse_pressed = true;
				}
				break;
			case SDL_EVENT_KEY_UP:
				if (!ctx->gamepad) {
					switch (event.key.key) {
					case SDLK_LEFT:
						ctx->button_left = 0;
						break;
					case SDLK_RIGHT:
						ctx->button_right = 0;
						break;
					case SDLK_UP:
						ctx->button_jump_released = true;
						break;
					}					
				}
				break;
			case SDL_EVENT_GAMEPAD_AXIS_MOTION:
				switch (event.gaxis.axis) {
				case SDL_GAMEPAD_AXIS_LEFTX:
					ctx->gamepad_left_stick.x = (float)event.gaxis.value / 32767.0f;
					break;
				case SDL_GAMEPAD_AXIS_LEFTY:
					ctx->gamepad_left_stick.y = (float)event.gaxis.value / 32767.0f;
					break;
				}
				break;
			case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
				switch (event.gbutton.button) {
				case SDL_GAMEPAD_BUTTON_SOUTH:
					ctx->button_jump = true;
					break;
				case SDL_GAMEPAD_BUTTON_WEST:
					ctx->button_attack = true;
					break;
				}
				break;
			case SDL_EVENT_GAMEPAD_BUTTON_UP:
				switch (event.gbutton.button) {
				case SDL_GAMEPAD_BUTTON_SOUTH:
					ctx->button_jump_released = true;
					break;
				}
				break;
			case SDL_EVENT_QUIT:
				ctx->running = false;
				break;
			}		
		}

		if (ctx->gamepad_left_stick.x < 0.5f && ctx->gamepad_left_stick.x > -0.5f) {
			ctx->gamepad_left_stick.x = 0.0f;
		}

	#if DELTA_TIME
	#ifndef _DEBUG
		{
			SDL_Time current_time;
			SDL_CHECK(SDL_GetCurrentTime(&current_time));
			SDL_Time dt_int = current_time - ctx->time;
			const double NANOSECONDS_IN_SECOND = 1000000000.0;
			double dt_double = (double)dt_int / NANOSECONDS_IN_SECOND;
			ctx->dt = (float)dt_double;
			ctx->time = current_time;
		}
	#else
		ctx->dt = ctx->display_mode->refresh_rate;
	#endif
	#else
		ctx->dt = 1.0f;
	#endif

		if (!ctx->vsync) {
			SDL_Delay(16); // TODO
		}

		for (size_t entity_idx = 0; entity_idx < ctx->levels[ctx->level_idx].n_entities; ++entity_idx) {
			Entity* entity = &ctx->levels[ctx->level_idx].entities[entity_idx];
			if (HAS_FLAG(entity->flags, EntityFlags_Active)) {
				if (HAS_FLAG(entity->flags, EntityFlags_Player)) {
					UpdatePlayer(ctx, entity);
				} else if (HAS_FLAG(entity->flags, EntityFlags_Boar)) {
					UpdateBoar(ctx, entity);
				}
			}
		}
		
		// RenderBegin
		{
			SDL_CHECK(SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, 0));
			SDL_CHECK(SDL_RenderClear(ctx->renderer));
		}

		ivec2s render_area;
		SDL_CHECK(SDL_GetRenderOutputSize(ctx->renderer, &render_area.x, &render_area.y));

		// RenderLevel
		{
			static Sprite spr_tiles;
			static bool initialized_sprites = false;
			if (!initialized_sprites) {
				initialized_sprites = true;
				spr_tiles = GetSprite("assets\\legacy_fantasy_high_forest\\Assets\\Tiles.aseprite");
			}

			for (size_t entity_idx = 0; entity_idx < ctx->levels[ctx->level_idx].n_entities; ++entity_idx) {
				Entity* entity = &ctx->levels[ctx->level_idx].entities[entity_idx];
				if (HAS_FLAG(entity->flags, EntityFlags_Active)) {
					if (HAS_FLAG(entity->flags, EntityFlags_Tile)) {
						DrawSpriteTile(ctx, spr_tiles, entity->src_pos, entity->pos);
					} else {
						DrawEntity(ctx, entity);
					}
				}
			}
		}

		// RenderEnd
		{
			SDL_RenderPresent(ctx->renderer);

			if (!ctx->vsync) {
				// TODO
			}
		}
	}
	
	SDL_Quit();
	return 0;
}

size_t HashString(char* key, size_t len) {
	if (len == 0) {
		len = SDL_strlen(key);
	}
	return XXH3_64bits((const void*)key, len);
}

SDL_EnumerationResult EnumerateDirectoryCallback(void *userdata, const char *dirname, const char *fname) {
	Context* ctx = userdata;

	// dirname\fname\file

	char dir_path[1024];
	SDL_CHECK(SDL_snprintf(dir_path, 1024, "%s%s", dirname, fname) >= 0);

	int32_t n_files;
	char** files = SDL_GlobDirectory(dir_path, "*.aseprite", 0, &n_files); SDL_CHECK(files);
	if (n_files > 0) {
		for (size_t file_idx = 0; file_idx < (size_t)n_files; ++file_idx) {
			char* file = files[file_idx];

			char sprite_path[2048]; const size_t SPRITE_PATH_SIZE = 2048;
			SDL_CHECK(SDL_snprintf(sprite_path, SPRITE_PATH_SIZE, "%s\\%s", dir_path, file) >= 0);
			SDL_CHECK(SDL_GetPathInfo(sprite_path, NULL));

			Sprite sprite = GetSprite(sprite_path);
			SpriteDesc* sprite_desc = &ctx->sprites[sprite.idx];
			SDL_assert(!sprite_desc->path && "Collision");

			{
				size_t buf_len = SDL_strlen(sprite_path) + 1;
				sprite_desc->path = SDL_malloc(buf_len); SDL_CHECK(sprite_desc->path);
				SDL_strlcpy(sprite_desc->path, sprite_path, buf_len);
			}
			
			SDL_IOStream* fs = SDL_IOFromFile(sprite_path, "r"); SDL_CHECK(fs);
			LoadSprite(ctx->renderer, fs, sprite_desc);

			SDL_CloseIO(fs);
		}
	} else if (n_files == 0) {
		SDL_CHECK(SDL_EnumerateDirectory(dir_path, EnumerateDirectoryCallback, ctx));
	}
	
	SDL_free(files);
	return SDL_ENUM_CONTINUE;
}

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
		Rect hitbox = GetEntityHitbox(ctx, player);
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
			Rect side;
			side.min.x = player->pos.x + hitbox.min.x + (int32_t)SDL_floorf(player->vel.x);
			side.min.y = player->pos.y + hitbox.min.y + 1;
			side.max.x = player->pos.x + hitbox.min.x + (int32_t)SDL_floorf(player->vel.x) + 1;
			side.max.y = player->pos.y + hitbox.max.y - 1;
			Rect tile;
			if (RectIntersectsLevel(level, side, &tile)) {
				player->pos.x = SDL_max(player->pos.x, tile.max.x - hitbox.min.x);
				conserved_vel.x = player->vel.x;
				player->vel.x = 0.0f;
				player->pos_remainder.x = 0.0f;
			}
		} else if (player->vel.x > 0.0f) {
			Rect side;
			side.min.x = player->pos.x + hitbox.max.x + (int32_t)SDL_floorf(player->vel.x) - 1;
			side.min.y = player->pos.y + hitbox.min.y + 1;
			side.max.x = player->pos.x + hitbox.max.x + (int32_t)SDL_floorf(player->vel.x);
			side.max.y = player->pos.y + hitbox.max.y - 1;
			Rect tile;
			if (RectIntersectsLevel(level, side, &tile)) {
				player->pos.x = SDL_min(player->pos.x, tile.min.x - hitbox.max.x);
				conserved_vel.x = player->vel.x;
				player->vel.x = 0.0f;
				player->pos_remainder.x = 0.0f;
			}
		}
		if (player->vel.y < 0.0f) {
			Rect side;
			side.min.x = player->pos.x + hitbox.min.x + 1;
			side.min.y = player->pos.y + hitbox.min.y + (int32_t)SDL_floorf(player->vel.y);
			side.max.x = player->pos.x + hitbox.max.x - 1;
			side.max.y = player->pos.y + hitbox.min.y + (int32_t)SDL_floorf(player->vel.y) + 1;
			Rect tile;
			if (RectIntersectsLevel(level, side, &tile)) {
				player->pos.y = SDL_max(player->pos.y, tile.max.y - hitbox.min.y);
				player->vel.y = 0.0f;
				player->pos_remainder.y = 0.0f;
			}
		} else if (player->vel.y > 0.0f) {
			Rect side;
			side.min.x = player->pos.x + hitbox.min.x + 1;
			side.min.y = player->pos.y + hitbox.max.y + (int32_t)SDL_floorf(player->vel.y) - 1;
			side.max.x = player->pos.x + hitbox.max.x - 1;
			side.max.y = player->pos.y + hitbox.max.y + (int32_t)SDL_floorf(player->vel.y);
			Rect tile;
			if (RectIntersectsLevel(level, side, &tile)) {
				player->pos.y = SDL_min(player->pos.y, tile.min.y - hitbox.max.y);
				player->vel.y = 0.0f;
				player->pos_remainder.y = 0.0f;
				player->touching_floor = PLAYER_JUMP_REMAINDER;
				player->flags &= ~EntityFlags_JumpReleased;
			}
		}

		{
			player->pos_remainder = glms_vec2_add(player->pos_remainder, player->vel);
			vec2s move = glms_vec2_floor(player->pos_remainder);
			player->pos = glms_ivec2_add(player->pos, ivec2_from_vec2(move));
			player->pos_remainder = glms_vec2_sub(player->pos_remainder, move);
		}

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
		Rect hitbox = GetEntityHitbox(ctx, boar);

		boar->vel.y += GRAVITY;

		Level* level = GetCurrentLevel(ctx);

		boar->touching_floor = false;
		if (boar->vel.y < 0.0f) {
			Rect side;
			side.min.x = boar->pos.x + hitbox.min.x + 1;
			side.min.y = boar->pos.y + hitbox.min.y + (int32_t)SDL_floorf(boar->vel.y);
			side.max.x = boar->pos.x + hitbox.max.x - 1;
			side.max.y = boar->pos.y + hitbox.min.y + (int32_t)SDL_floorf(boar->vel.y) + 1;
			Rect tile;
			if (RectIntersectsLevel(level, side, &tile)) {
				boar->pos.y = tile.min.y - hitbox.min.y;
				boar->vel.y = 0.0f;
			}
		} else if (boar->vel.y > 0.0f) {
			Rect side;
			side.min.x = boar->pos.x + hitbox.min.x + 1;
			side.min.y = boar->pos.y + hitbox.max.y + (int32_t)SDL_floorf(boar->vel.y) - 1;
			side.max.x = boar->pos.x + hitbox.max.x - 1;
			side.max.y = boar->pos.y + hitbox.max.y + (int32_t)SDL_floorf(boar->vel.y);
			Rect tile;
			if (RectIntersectsLevel(level, side, &tile)) {
				boar->pos.y = tile.min.y - hitbox.max.y;
				boar->vel.y = 0.0f;
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
			Rect side;
			side.min.x = boar->pos.x + hitbox.min.x + (int32_t)SDL_floorf(boar->vel.x);
			side.min.y = boar->pos.y + hitbox.min.y + 1;
			side.max.x = boar->pos.x + hitbox.min.x + (int32_t)SDL_floorf(boar->vel.x) + 1;
			side.max.y = boar->pos.y + hitbox.max.y - 1;
			Rect tile;
			if (RectIntersectsLevel(level, side, &tile)) {
				int32_t old_pos = boar->pos.x;
				boar->pos.x = tile.max.x - hitbox.min.x;
				if (boar->pos.x > old_pos) {
					boar->pos.x = old_pos;
				}
				boar->vel.x = 0.0f;
			}
		} else if (boar->vel.x > 0.0f) {
			Rect side;
			side.min.x = boar->pos.x + hitbox.max.x + (int32_t)SDL_floorf(boar->vel.x) - 1;
			side.min.y = boar->pos.y + hitbox.min.y + 1;
			side.max.x = boar->pos.x + hitbox.max.x + (int32_t)SDL_floorf(boar->vel.x);
			side.max.y = boar->pos.y + hitbox.max.y - 1;
			Rect tile;
			if (RectIntersectsLevel(level, side, &tile)) {
				int32_t old_pos = boar->pos.x;
				boar->pos.x = tile.min.x - hitbox.max.x;
				if (boar->pos.x < old_pos) {
					boar->pos.x = old_pos;
				}
				boar->vel.x = 0.0f;
			}
		}

		{
			boar->pos_remainder = glms_vec2_add(boar->pos_remainder, boar->vel);
			vec2s move = glms_vec2_floor(boar->pos_remainder);
			boar->pos = glms_ivec2_add(boar->pos, ivec2_from_vec2(move));
			boar->pos_remainder = glms_vec2_sub(boar->pos_remainder, move);
		}
	}

	bool loop = true;
	if (SpritesEqual(boar->anim.sprite, boar_hit)) {
		loop = false;
	}
	UpdateAnim(ctx, &boar->anim, loop);
}

Entity* GetEntities(Context* ctx, size_t* n_entities) {
	SDL_assert(n_entities);
	*n_entities = ctx->levels[ctx->level_idx].n_entities;
	return ctx->levels[ctx->level_idx].entities;
}

bool EntitiesIntersect(Context* ctx, Entity* a, Entity* b) {
	Rect ha = GetEntityHitbox(ctx, a);
	ha.min = glms_ivec2_add(ha.min, a->pos);
	ha.max = glms_ivec2_add(ha.max, a->pos);
	Rect hb = GetEntityHitbox(ctx, b);
	hb.min = glms_ivec2_add(hb.min, b->pos);
	hb.max = glms_ivec2_add(hb.max, b->pos);
	return RectsIntersect(ha, hb);
}

Level* GetCurrentLevel(Context* ctx) {
	return &ctx->levels[ctx->level_idx];
}

bool EntityApplyFriction(Entity* entity, float fric, float max_vel) {
	float vel_save = entity->vel.x;
	if (entity->vel.x < 0.0f) entity->vel.x = SDL_min(0.0f, entity->vel.x + fric);
	else if (entity->vel.x > 0.0f) entity->vel.x = SDL_max(0.0f, entity->vel.x - fric);
	entity->vel.x = SDL_clamp(entity->vel.x, -max_vel, max_vel);
	return entity->vel.x != vel_save;
}
