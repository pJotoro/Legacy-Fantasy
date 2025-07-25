#include <SDL.h>
#include <SDL_main.h>
#include <SDL_ttf.h>

#include <cglm/struct.h>

#include "nuklear_defines.h"
#pragma warning(push, 0)
#include <nuklear.h>
#pragma warning(pop)

#include <raddbg_markup.h>

#define XXH_STATIC_LINKING_ONLY /* access advanced declarations */
#define XXH_IMPLEMENTATION      /* access definitions */
#include <xxhash.h>

#include "infl.h"

typedef int64_t ssize_t;

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
		.pos.x = TILE_SIZE*9, 
		.pos.y = TILE_SIZE*8,
		.dir = 1.0f,
		.touching_floor = 10,
	};
	SetSpriteFromPath(ctx, &ctx->player, "assets\\legacy_fantasy_high_forest\\Character\\Idle\\Idle.aseprite");
}

int32_t main(int32_t argc, char* argv[]) {
	(void)argc;
	(void)argv;

	SDL_CHECK(SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD));
	SDL_CHECK(TTF_Init());

	Context* ctx = SDL_calloc(1, sizeof(Context)); SDL_CHECK(ctx);

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

#if 1
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
		ctx->nk.font.userdata.ptr = ctx->font_roboto_regular;
		ctx->nk.font.height = (float)TTF_GetFontHeight(ctx->font_roboto_regular);
		ctx->nk.font.width = NK_TextWidthCallback;
		const size_t MEM_SIZE = MEGABYTE(64);
		void* mem = SDL_malloc(MEM_SIZE); SDL_CHECK(mem);
		bool ok = nk_init_fixed(&ctx->nk.ctx, mem, MEM_SIZE, &ctx->nk.font); SDL_assert(ok);
	}

	LoadLevel(ctx);

	SDL_CHECK(SDL_EnumerateDirectory("assets\\legacy_fantasy_high_forest", EnumerateDirectoryCallback, ctx));
	if (ctx->sprite_tests_failed > 0) {
		SDL_Log("Sprite tests failed: %llu", ctx->sprite_tests_failed);
	}
	// for (size_t sprite_idx = 0; sprite_idx < MAX_SPRITES; sprite_idx += 1) {
	// 	SpriteDesc* sd = &ctx->sprites[sprite_idx];
	// 	if (!sd->path) continue;

	// 	for (size_t frame_idx = 0; frame_idx < sd->n_frames; frame_idx += 1) {

	// 	}
	// }

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

		nk_input_begin(&ctx->nk.ctx);

		ctx->button_jump = false;
		ctx->button_jump_released = false;
		ctx->button_attack = false;

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			NK_HandleEvent(ctx, &event);
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

		nk_input_end(&ctx->nk.ctx);

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

		// UpdateLevel
		{
			SDL_PathInfo info;
			SDL_CHECK(SDL_GetPathInfo("level", &info));
			if (info.modify_time != ctx->level.modify_time) {
				LoadLevel(ctx);
				ctx->level.modify_time = info.modify_time;
			}
		}

		UpdatePlayer(ctx);
		
		if (ctx->draw_selected_anim) {
			UpdateAnim(ctx, &ctx->selected_anim, true);
		}

		// RenderBegin
		{
			SDL_CHECK(SDL_SetRenderDrawColor(ctx->renderer, 100, 100, 100, 255));
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
			SDL_CHECK(SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 255, 0));
			for (ivec2s tile = {0, 0}; tile.y < ctx->level.size.y; tile.y += 1) {
				for (tile.x = 0; tile.x < ctx->level.size.x; tile.x += 1) {
					if (GetTile(&ctx->level, tile) == TILE_TYPE_GROUND) {
						SDL_FRect rect = { (float)(tile.x*TILE_SIZE - ctx->player.pos.x + render_area.x/2), (float)(tile.y*TILE_SIZE - ctx->player.pos.y + render_area.y/2), (float)TILE_SIZE, (float)TILE_SIZE };
						SDL_CHECK(SDL_RenderFillRect(ctx->renderer, &rect));
					}
				}
			}
		}

		// RenderSelectedTexture
		{
			// SDL_FRect dst = { 0.0f, 0.0f, (float)ctx->sprites[ctx->sprite_idx].w, (float)ctx->sprites[ctx->sprite_idx].h };
			// SDL_CHECK(SDL_RenderTexture(ctx->renderer, ctx->sprites[ctx->sprite_idx].texture, NULL, &dst));
		}

		if (ctx->draw_selected_anim) {
			DrawAnim(ctx, &ctx->selected_anim, (ivec2s){300, 300}, 1.0f);
		}

		#if 0
		{
			static Sprite tiles;
			static bool initialized_tiles = false;
			if (!initialized_tiles) {
				initialized_tiles = true;
				tiles = GetSprite("assets\\legacy_fantasy_high_forest\\Assets\\Tiles.aseprite");
			}
			for (ivec2s tile = {0, 0}; tile.y < 25; ++tile.y) {
				for (tile.x = 0; tile.x < 25; ++tile.x) {
					// if (!printed) {
					// 	
					// 	SDL_Log("{%f, %f}", current_pos.x, current_pos.y);
					// }
					DrawSpriteTile(ctx, tiles, tile, (vec2s){(float)tile.x*16.0f, (float)tile.y*16.0f});
				}
			}
		}
		#endif

		NK_UpdateUI(ctx);

		// RenderEnd
		{
			NK_Render(ctx);

			SDL_RenderPresent(ctx->renderer);

			nk_clear(&ctx->nk.ctx);

			if (!ctx->vsync) {
				// TODO
			}
		}
	}

	// WriteVariables
	{
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
		if (ctx->player.vel.x < 0.0f) {
			Rect side;
			side.min.x = ctx->player.pos.x + (int32_t)SDL_floorf(ctx->player.vel.x);
			side.min.y = ctx->player.pos.y + 1;
			side.max.x = side.min.x + 1;
			side.max.y = side.min.y + ctx->player.size.y - 2;
			Rect tile;
			if (RectIntersectsLevel(&ctx->level, side, &tile)) {
				int32_t old_pos = ctx->player.pos.x;
				ctx->player.pos.x = tile.max.x;
				if (ctx->player.pos.x > old_pos) {
					ctx->player.pos.x = old_pos;
				}
				conserved_vel.x = ctx->player.vel.x;
				ctx->player.vel.x = 0.0f;
			}
		} else if (ctx->player.vel.x > 0.0f) {
			Rect side;
			side.min.x = (ctx->player.pos.x + ctx->player.size.x - 1) + (int32_t)SDL_floorf(ctx->player.vel.x);
			side.min.y = ctx->player.pos.y + 1;
			side.max.x = side.min.x + 1;
			side.max.y = side.min.y + ctx->player.size.y - 2;
			Rect tile;
			if (RectIntersectsLevel(&ctx->level, side, &tile)) {
				int32_t old_pos = ctx->player.pos.x;
				ctx->player.pos.x = tile.min.x - ctx->player.size.x;
				if (ctx->player.pos.x < old_pos) {
					ctx->player.pos.x = old_pos;
				}
				conserved_vel.x = ctx->player.vel.x;
				ctx->player.vel.x = 0.0f;
			}
		}

		if (ctx->player.vel.y < 0.0f) {
			Rect side;
			side.min.x = ctx->player.pos.x + 1; 
			side.min.y = ctx->player.pos.y + (int32_t)SDL_floorf(ctx->player.vel.y);
			side.max.x = side.min.x + ctx->player.size.x - 2;
			side.max.y = side.min.y + 1;
			Rect tile;
			if (RectIntersectsLevel(&ctx->level, side, &tile)) {
				ctx->player.pos.y = tile.min.y + ctx->player.size.y;
				ctx->player.vel.y = 0.0f;
			}
		} else if (ctx->player.vel.y > 0.0f) {
			Rect side;
			side.min.x = ctx->player.pos.x + 1; 
			side.min.y = (ctx->player.pos.y + ctx->player.size.y - 1) + (int32_t)SDL_floorf(ctx->player.vel.y);
			side.max.x = side.min.x + ctx->player.size.x - 2;
			side.max.y = side.min.y + 1;
			Rect tile;
			if (RectIntersectsLevel(&ctx->level, side, &tile)) {
				ctx->player.pos.y = tile.min.y - ctx->player.size.y;
				ctx->player.vel.y = 0.0f;
				ctx->player.touching_floor = 10;
				ctx->player.jump_released = false;
			}
		}

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

	bool loop = true;
	if (ctx->player.anim.sprite.idx == player_jump_start.idx || ctx->player.anim.sprite.idx == player_jump_end.idx || ctx->player.anim.sprite.idx == player_attack.idx) {
		loop = false;
	}
	UpdateAnim(ctx, &ctx->player.anim, loop);

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

	if (ctx->player.pos.y > (float)(ctx->level.size.y+500)) {
		ResetGame(ctx);
	}
}