#include <SDL.h>
#include <SDL_main.h>
#include <SDL_ttf.h>

#include <cglm/struct.h>

#if 0
#include "nuklear_defines.h"
#pragma warning(push, 0)
#include <nuklear.h>
#pragma warning(pop)
#endif

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

#include "variables.c"
#include "util.c"
#include "level.c"
#include "nuklear_util.c"
#include "sprite.c"

void ResetGame(Context* ctx) {
	ctx->dt = ctx->display_mode->refresh_rate;
	ctx->player = (Entity){
		.pos.x = TILE_SIZE*0, 
		.pos.y = TILE_SIZE*0,
		.dir = 1.0f,
		.touching_floor = 10,
	};
	SetSpriteFromPath(ctx, &ctx->player, "assets\\legacy_fantasy_high_forest\\Character\\Idle\\Idle.aseprite");
}

// https://ldtk.io/json/#ldtk-Tile
void ParseTile(JSON_Node* cur, Tile* tile) {
	JSON_Node* px = cur->child; SDL_assert(px); SDL_assert(HAS_FLAG(px->type, JSON_Array));
	JSON_Node* px_x = px->child; SDL_assert(px_x);
	JSON_Node* px_y = px_x->next; SDL_assert(px_y);

	JSON_Node* src = px->next; SDL_assert(src); SDL_assert(HAS_FLAG(src->type, JSON_Array));
	JSON_Node* src_x = src->child; SDL_assert(src_x);
	JSON_Node* src_y = src_x->next; SDL_assert(src_y);

	tile->src.x = src_x->valueint;
	tile->src.y = src_y->valueint;
	tile->dst.x = px_x->valueint;
	tile->dst.y = px_y->valueint;
}

// Really ParsePlayer for now
void ParseEntity(JSON_Node* cur, Entity* entity) {
	UNUSED(entity);
	UNUSED(cur);	
}

int32_t main(int32_t argc, char* argv[]) {
	UNUSED(argc);
	UNUSED(argv);

	SDL_CHECK(SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD));
	SDL_CHECK(TTF_Init());

	Context* ctx = SDL_calloc(1, sizeof(Context)); SDL_CHECK(ctx);

	// LoadLevel
	{
		JSON_Node* head;
		{
			size_t file_len;
			void* file_data = SDL_LoadFile("assets\\levels\\test.ldtk", &file_len); SDL_CHECK(file_data);
			head = JSON_ParseWithLength((const char*)file_data, file_len);
			SDL_free(file_data);
		}
		
		SDL_assert(HAS_FLAG(head->type, JSON_Object));

		bool parsed_tiles = false;
		bool parsed_entities = false;

		JSON_Node* cur;
		JSON_ArrayForEach(cur, head) {
			if (HAS_FLAG(cur->type, JSON_Array) && SDL_strcmp(cur->string, "levels") == 0) {
				JSON_Node* levels = cur;
				JSON_ArrayForEach(cur, levels) {
					SDL_assert(HAS_FLAG(cur->type, JSON_Object));
					JSON_Node* level = cur;
					JSON_ArrayForEach(cur, level) {
						if (HAS_FLAG(cur->type, JSON_Array) && SDL_strcmp(cur->string, "layerInstances") == 0) {
							JSON_Node* layer = cur;
							JSON_ArrayForEach(cur, layer) {
								SDL_assert(HAS_FLAG(cur->type, JSON_Object));
								JSON_Node* layer_prop = cur;
								JSON_ArrayForEach(cur, layer_prop) {
									if (HAS_FLAG(cur->type, JSON_Array)) {
										if (!parsed_tiles && SDL_strcmp(cur->string, "gridTiles") == 0) {
											parsed_tiles = true;

											JSON_Node* tiles = cur;
											JSON_ArrayForEach(cur, tiles) {
												SDL_assert(HAS_FLAG(cur->type, JSON_Object));
												ctx->level.n_tiles += 1;
											}
											ctx->level.tiles = SDL_malloc(sizeof(Tile) * ctx->level.n_tiles); SDL_CHECK(ctx->level.tiles);
											cur = tiles;
											size_t tile_idx = 0;
											JSON_ArrayForEach(cur, tiles) {
												ParseTile(cur, &ctx->level.tiles[tile_idx]);
												tile_idx += 1;
											}
											cur = tiles;
										} else if (!parsed_entities && SDL_strcmp(cur->string, "entityInstances") == 0) {
											parsed_entities = true;

											JSON_Node* entities = cur;
											JSON_ArrayForEach(cur, entities) {
												SDL_assert(HAS_FLAG(cur->type, JSON_Object));
												ParseEntity(cur, &ctx->player);
											}
											cur = entities;
										}

									}
								}
								cur = layer_prop;
							}
							cur = layer;
						}
					}
					cur = level;
				}
				cur = levels;
			}
		}

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

#if 0
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

	// InitNuklear
	{
		#if 0
		ctx->nk.font.userdata.ptr = ctx->font_roboto_regular;
		ctx->nk.font.height = (float)TTF_GetFontHeight(ctx->font_roboto_regular);
		ctx->nk.font.width = NK_TextWidthCallback;
		const size_t MEM_SIZE = MEGABYTE(64);
		void* mem = SDL_malloc(MEM_SIZE); SDL_CHECK(mem);
		bool ok = nk_init_fixed(&ctx->nk.ctx, mem, MEM_SIZE, &ctx->nk.font); SDL_assert(ok);
		#endif
	}

	// LoadLevel(ctx);

	SDL_CHECK(SDL_EnumerateDirectory("assets\\legacy_fantasy_high_forest", EnumerateDirectoryCallback, ctx));
	if (ctx->sprite_tests_failed > 0) {
		SDL_Log("Sprite tests failed: %llu", ctx->sprite_tests_failed);
	}

	// SortSpriteCells (uses insertion sort)
	for (size_t sprite_idx = 0; sprite_idx < MAX_SPRITES; sprite_idx += 1) {
		SpriteDesc* sd = &ctx->sprites[sprite_idx];
		if (sd->path) {
			for (size_t frame_idx = 0; frame_idx < sd->n_frames; frame_idx += 1) {
				SpriteFrame* sf = &sd->frames[frame_idx];
				for (size_t i = 1; i < sf->n_cells; i += 1) {
					SpriteCell key = sf->cells[i];
					size_t j = i - 1;
					while (j >= 0 && CompareSpriteCells(&sf->cells[j], &key) == 1) {
						sf->cells[j + 1] = sf->cells[j];
						j -= 1;
					}
					sf->cells[j + 1] = key;
				}
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

#if 0
		nk_input_begin(&ctx->nk.ctx);
#endif

		ctx->button_jump = false;
		ctx->button_jump_released = false;
		ctx->button_attack = false;

		ctx->left_mouse_pressed = false;

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			#if 0
			NK_HandleEvent(ctx, &event);
			#endif
			switch (event.type) {
			case SDL_EVENT_KEY_DOWN:
			#if 1
				if (event.key.key == SDLK_ESCAPE) {
					ctx->running = false;
				}
				else if (event.key.key == SDLK_0) {
					ctx->draw_selected_anim = true;
					ResetAnim(&ctx->selected_anim);
					do {
						ctx->selected_anim.sprite.idx += 1;
						if (ctx->selected_anim.sprite.idx >= MAX_SPRITES) ctx->selected_anim.sprite.idx = 0;
					} while (!ctx->sprites[ctx->selected_anim.sprite.idx].path);
				}
			#endif
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

#if 0
		nk_input_end(&ctx->nk.ctx);
#endif

	#if 0
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

#if 0
		NK_UpdateUI(ctx);
#endif

		UpdatePlayer(ctx);
		
		if (ctx->draw_selected_anim) {
			UpdateAnim(ctx, &ctx->selected_anim, true);
		}

		// RenderBegin
		{
			SDL_CHECK(SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, 0));
			SDL_CHECK(SDL_RenderClear(ctx->renderer));
		}

		ivec2s render_area;
		SDL_CHECK(SDL_GetRenderOutputSize(ctx->renderer, &render_area.x, &render_area.y));

		// RenderPlayer
		{
			ivec2s save_pos = ctx->player.pos;
			ctx->player.pos = glms_ivec2_divs(render_area, 2);
			DrawEntity(ctx, &ctx->player);
			ctx->player.pos = save_pos;
		}

		// RenderLevel
		{
			static Sprite spr_tiles;
			static bool initialized_sprites = false;
			if (!initialized_sprites) {
				initialized_sprites = true;
				spr_tiles = GetSprite("assets\\legacy_fantasy_high_forest\\Assets\\Tiles.aseprite");
			}

			for (size_t tile_idx = 0; tile_idx < ctx->level.n_tiles; tile_idx += 1) {
				DrawSpriteTile(ctx, spr_tiles, ctx->level.tiles[tile_idx].src, ctx->level.tiles[tile_idx].dst);
			}

			// for (ivec2s level_pos = {0, 0}; level_pos.y < ctx->level.size.y; level_pos.y += 1) {
			// 	for (level_pos.x = 0; level_pos.x < ctx->level.size.x; level_pos.x += 1) {
			// 		Tile tile = GetTile(&ctx->level, level_pos);
			// 		ivec2s pos = glms_ivec2_add(level_pos, glms_ivec2_divs(ctx->player.pos, 2));
			// 		DrawSpriteTile(ctx, spr_tiles, tile, pos);
			// 	}
			// }
		}

		// RenderSelectedTexture
		{
			// SDL_FRect dst = { 0.0f, 0.0f, (float)ctx->sprites[ctx->sprite_idx].w, (float)ctx->sprites[ctx->sprite_idx].h };
			// SDL_CHECK(SDL_RenderTexture(ctx->renderer, ctx->sprites[ctx->sprite_idx].texture, NULL, &dst));
		}

		if (ctx->draw_selected_anim) {
			DrawAnim(ctx, &ctx->selected_anim, (ivec2s){300, 300}, 1.0f);
		}

		// {
		// 	static Sprite tiles;
		// 	static bool initialized_tiles = false;
		// 	if (!initialized_tiles) {
		// 		initialized_tiles = true;
		// 		tiles = GetSprite("assets\\legacy_fantasy_high_forest\\Assets\\Tiles.aseprite");
		// 	}
		// 	ivec2s mouse = ivec2_from_vec2(ctx->mouse_pos);
		// 	SDL_RenderRect(ctx->renderer, &(SDL_FRect){(float)(mouse.x - mouse.x%TILE_SIZE - 1), (float)(mouse.y - mouse.y%TILE_SIZE - 1), (float)(TILE_SIZE+2), (float)(TILE_SIZE+2)});
		// 	for (ivec2s tile = {0, 0}; tile.y < 25; ++tile.y) {
		// 		for (tile.x = 0; tile.x < 25; ++tile.x) {
		// 			vec2s tile_pos = vec2_from_ivec2(glms_ivec2_scale(tile, TILE_SIZE));
		// 			DrawSpriteTile(ctx, tiles, tile, tile_pos);
		// 			if (mouse.x >= tile_pos.x && mouse.x < tile_pos.x + TILE_SIZE && mouse.y >= tile_pos.y && mouse.y < tile_pos.y + TILE_SIZE) {
		// 				if (ctx->left_mouse_pressed) {
		// 					ctx->selected_tile = tile;
		// 					SDL_Log("{%d, %d}", ctx->selected_tile.x, ctx->selected_tile.y);
		// 				}
		// 			}

		// 		}
		// 	}

		// 	DrawSpriteTile(ctx, tiles, ctx->selected_tile, glms_vec2_sub(ctx->mouse_pos, glms_vec2_mods(ctx->mouse_pos, (float)TILE_SIZE)));
		// }

		// RenderEnd
		{
			#if 0
			NK_Render(ctx);
			#endif

			SDL_RenderPresent(ctx->renderer);

#if 0
			nk_clear(&ctx->nk.ctx);
#endif

			if (!ctx->vsync) {
				// TODO
			}
		}
	}

	// WriteVariables
	{
		#if 0
		SDL_IOStream* fs = SDL_IOFromFile("variables.c", "w"); SDL_CHECK(fs);
		#define SDL_IOWriteVarI(fs, var) SDL_CHECK(SDL_IOprintf(fs, STRINGIFY(static int32_t var = %d;\n), var) != 0)
		#define SDL_IOWriteVarF(fs, var) SDL_CHECK(SDL_IOprintf(fs, STRINGIFY(static float var = %ff;\n), var) != 0)
		SDL_IOWriteVarI(fs, TILE_SIZE);
		SDL_IOWriteVarI(fs, PLAYER_JUMP_PERIOD);
		SDL_IOWriteVarF(fs, GRAVITY);
		SDL_IOWriteVarF(fs, PLAYER_ACC);
		SDL_IOWriteVarF(fs, PLAYER_FRIC);
		SDL_IOWriteVarF(fs, PLAYER_MAX_VEL);
		SDL_IOWriteVarF(fs, PLAYER_JUMP);
		SDL_CloseIO(fs);
		#endif
	}
	
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
		for (size_t file_idx = 0; file_idx < (size_t)n_files; file_idx += 1) {
			char* file = files[file_idx];

			char sprite_path[2048]; const size_t SPRITE_PATH_SIZE = 2048;
			SDL_CHECK(SDL_snprintf(sprite_path, SPRITE_PATH_SIZE, "%s\\%s", dir_path, file) >= 0);
			SDL_CHECK(SDL_GetPathInfo(sprite_path, NULL));

			Sprite sprite = GetSprite(sprite_path);
			SpriteDesc* sprite_desc = &ctx->sprites[sprite.idx];
			if (sprite_desc->path) {
				SDL_LogMessage(SDL_LOG_CATEGORY_ERROR, SDL_LOG_PRIORITY_CRITICAL, "Collision: %s", file);
				ctx->n_collisions += 1;
				continue;
			} else {
				// SDL_Log("No collision: %s", file);
			}

			{
				size_t buf_len = SDL_strlen(sprite_path) + 1;
				sprite_desc->path = SDL_malloc(buf_len); SDL_CHECK(sprite_desc->path);
				SDL_strlcpy(sprite_desc->path, sprite_path, buf_len);
			}
			
			SDL_IOStream* fs = SDL_IOFromFile(sprite_path, "r"); SDL_CHECK(fs);
			LoadSprite(ctx->renderer, fs, sprite_desc);

			ctx->n_sprites += 1;

			SDL_CloseIO(fs);
		}
	} else if (n_files == 0) {
		SDL_CHECK(SDL_EnumerateDirectory(dir_path, EnumerateDirectoryCallback, ctx));
	}
	
	SDL_free(files);
	return SDL_ENUM_CONTINUE;
}

void UpdatePlayer(Context* ctx) {
	static Sprite player_idle;
	static Sprite player_run;
	static Sprite player_jump_start;
	static Sprite player_jump_end;
	static Sprite player_attack;

	static bool sprites_initialized = false;
	if (!sprites_initialized) {
		sprites_initialized = true;

		player_idle = GetSprite("assets\\legacy_fantasy_high_forest\\Character\\Idle\\Idle.aseprite");
		player_run = GetSprite("assets\\legacy_fantasy_high_forest\\Character\\Run\\Run.aseprite");
		player_jump_start = GetSprite("assets\\legacy_fantasy_high_forest\\Character\\Jump-Start\\Jump-Start.aseprite");
		player_jump_end = GetSprite("assets\\legacy_fantasy_high_forest\\Character\\Jump-End\\Jump-End.aseprite");
		player_attack = GetSprite("assets\\legacy_fantasy_high_forest\\Character\\Attack-01\\Attack-01.aseprite");
	}

	if (ctx->player.touching_floor && ctx->button_attack) {
		SetSprite(&ctx->player, player_attack);
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

	if (ctx->player.anim.sprite.idx != player_attack.idx) {
		ctx->player.vel.y += GRAVITY;
		ctx->player.touching_floor = SDL_max(ctx->player.touching_floor - 1, 0);

		if (ctx->player.touching_floor) {
			if (ctx->button_jump) {
				ctx->player.touching_floor = 0;
				ctx->player.vel.y = -PLAYER_JUMP;
				SetSprite(&ctx->player, player_jump_start);
			} else {
				float acc;
				if (!ctx->gamepad) {
					acc = (float)input_x * PLAYER_ACC;
				} else {
					acc = ctx->gamepad_left_stick.x * PLAYER_ACC;
				}
				ctx->player.vel.x += acc;
				if (ctx->player.vel.x < 0.0f) ctx->player.vel.x = SDL_min(0.0f, ctx->player.vel.x + PLAYER_FRIC);
				else if (ctx->player.vel.x > 0.0f) ctx->player.vel.x = SDL_max(0.0f, ctx->player.vel.x - PLAYER_FRIC);
				ctx->player.vel.x = SDL_clamp(ctx->player.vel.x, -PLAYER_MAX_VEL, PLAYER_MAX_VEL);
			
			}	
		} else if (ctx->button_jump_released && !ctx->player.jump_released && ctx->player.vel.y < 0.0f) {
			ctx->player.jump_released = true;
			ctx->player.vel.y /= 2.0f;
		}

		// PlayerCollision
		// if (ctx->player.vel.x < 0.0f) {
		// 	Rect side;
		// 	side.min.x = ctx->player.pos.x + (int32_t)SDL_floorf(ctx->player.vel.x);
		// 	side.min.y = ctx->player.pos.y + 1;
		// 	side.max.x = side.min.x + 1;
		// 	side.max.y = side.min.y + ctx->player.size.y - 2;
		// 	Rect tile;
		// 	if (RectIntersectsLevel(&ctx->level, side, &tile)) {
		// 		int32_t old_pos = ctx->player.pos.x;
		// 		ctx->player.pos.x = tile.max.x;
		// 		if (ctx->player.pos.x > old_pos) {
		// 			ctx->player.pos.x = old_pos;
		// 		}
		// 		conserved_vel.x = ctx->player.vel.x;
		// 		ctx->player.vel.x = 0.0f;
		// 	}
		// } else if (ctx->player.vel.x > 0.0f) {
		// 	Rect side;
		// 	side.min.x = (ctx->player.pos.x + ctx->player.size.x - 1) + (int32_t)SDL_floorf(ctx->player.vel.x);
		// 	side.min.y = ctx->player.pos.y + 1;
		// 	side.max.x = side.min.x + 1;
		// 	side.max.y = side.min.y + ctx->player.size.y - 2;
		// 	Rect tile;
		// 	if (RectIntersectsLevel(&ctx->level, side, &tile)) {
		// 		int32_t old_pos = ctx->player.pos.x;
		// 		ctx->player.pos.x = tile.min.x - ctx->player.size.x;
		// 		if (ctx->player.pos.x < old_pos) {
		// 			ctx->player.pos.x = old_pos;
		// 		}
		// 		conserved_vel.x = ctx->player.vel.x;
		// 		ctx->player.vel.x = 0.0f;
		// 	}
		// }

		// if (ctx->player.vel.y < 0.0f) {
		// 	Rect side;
		// 	side.min.x = ctx->player.pos.x + 1; 
		// 	side.min.y = ctx->player.pos.y + (int32_t)SDL_floorf(ctx->player.vel.y);
		// 	side.max.x = side.min.x + ctx->player.size.x - 2;
		// 	side.max.y = side.min.y + 1;
		// 	Rect tile;
		// 	if (RectIntersectsLevel(&ctx->level, side, &tile)) {
		// 		ctx->player.pos.y = tile.min.y + ctx->player.size.y;
		// 		ctx->player.vel.y = 0.0f;
		// 	}
		// } else if (ctx->player.vel.y > 0.0f) {
		// 	Rect side;
		// 	side.min.x = ctx->player.pos.x + 1; 
		// 	side.min.y = (ctx->player.pos.y + ctx->player.size.y - 1) + (int32_t)SDL_floorf(ctx->player.vel.y);
		// 	side.max.x = side.min.x + ctx->player.size.x - 2;
		// 	side.max.y = side.min.y + 1;
		// 	Rect tile;
		// 	if (RectIntersectsLevel(&ctx->level, side, &tile)) {
		// 		ctx->player.pos.y = tile.min.y - ctx->player.size.y;
		// 		ctx->player.vel.y = 0.0f;
		// 		ctx->player.touching_floor = 10;
		// 		ctx->player.jump_released = false;
		// 	}
		// }

		{
			ctx->player.pos_remainder = glms_vec2_add(ctx->player.pos_remainder, ctx->player.vel);
			vec2s move = glms_vec2_floor(ctx->player.pos_remainder);
			ctx->player.pos = glms_ivec2_add(ctx->player.pos, ivec2_from_vec2(move));
			ctx->player.pos_remainder = glms_vec2_sub(ctx->player.pos_remainder, move);
		}

		if (ctx->player.touching_floor) {
			if (input_x == 0 && ctx->player.vel.x == 0.0f) {
				SetSprite(&ctx->player, player_idle);
			} else {
				SetSprite(&ctx->player, player_run);
				if (ctx->player.vel.x != 0.0f) {
					ctx->player.dir = glm_signf(ctx->player.vel.x);
				} else if (input_x != 0) {
					ctx->player.dir = (float)input_x;
				}
			}
		} else {
			if (ctx->player.anim.sprite.idx == player_jump_start.idx && ctx->player.anim.ended) {
				SetSprite(&ctx->player, player_jump_end);
			}
		}
	}

	if (ctx->player.anim.sprite.idx == player_attack.idx && ctx->player.anim.ended) {
		if (input_x == 0 && ctx->player.vel.x == 0.0f) {
			SetSprite(&ctx->player, player_idle);
		} else {
			SetSprite(&ctx->player, player_run);
			if (ctx->player.vel.x != 0.0f) {
				ctx->player.dir = glm_signf(ctx->player.vel.x);
			} else if (input_x != 0) {
				ctx->player.dir = (float)input_x;
			}
		}
	}

	if (conserved_vel.x != 0.0f) ctx->player.vel.x = conserved_vel.x;
	if (conserved_vel.y != 0.0f) ctx->player.vel.y = conserved_vel.y;

	bool loop = true;
	if (ctx->player.anim.sprite.idx == player_jump_start.idx || ctx->player.anim.sprite.idx == player_jump_end.idx || ctx->player.anim.sprite.idx == player_attack.idx) {
		loop = false;
	}
	UpdateAnim(ctx, &ctx->player.anim, loop);

	if (ctx->player.pos.y > 1000.0f) {
		ResetGame(ctx);
	}
}