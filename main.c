#include <SDL.h>
#include <SDL_main.h>
#include <SDL_image.h>
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

#include "aseprite.h"

#include "main.h"

#include "variables.c"
#include "util.c"
#include "level.c"
#include "nuklear_util.c"

void ResetGame(Context* ctx) {
	ctx->dt = ctx->display_mode->refresh_rate;
	ctx->player = (Entity){
		.pos.x = TILE_SIZE*1.0f, 
		.pos.y = TILE_SIZE*6.0f,
	};
	SetSprite(ctx, &ctx->player, "assets\\legacy_fantasy_high_forest\\Character\\Idle\\Idle.aseprite");
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

	ResetGame(ctx);

	ctx->running = true;
	while (ctx->running) {
		nk_input_begin(&ctx->nk.ctx);

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			NK_HandleEvent(ctx, &event);

			switch (event.type) {
			case SDL_EVENT_KEY_DOWN:
				switch (event.key.key) {
				case SDLK_0:
					ctx->show_ui = !ctx->show_ui;
					break;
				case SDLK_SPACE:
					break;
				case SDLK_ESCAPE:
					ctx->running = false;
					break;
				case SDLK_LEFT:
					if (!ctx->gamepad) {
						ctx->axis.x = SDL_max(ctx->axis.x - 1.0f, -1.0f);
					}
					break;
				case SDLK_RIGHT:
					if (!ctx->gamepad) {
						ctx->axis.x = SDL_min(ctx->axis.x + 1.0f, +1.0f);
					}
					break;
				case SDLK_UP:
					if (!ctx->gamepad) {
						if (!event.key.repeat && ctx->player.can_jump) {
							ctx->player.can_jump = 0;
							ctx->player.vel.y = -PLAYER_JUMP;
						}
					}
					break;
				case SDLK_DOWN:
					break;
				case SDLK_R:
					ResetGame(ctx);
					break;
				}
				break;
			case SDL_EVENT_KEY_UP:
				switch (event.key.key) {
				case SDLK_LEFT:
					if (!ctx->gamepad) {
						ctx->axis.x = SDL_min(ctx->axis.x + 1.0f, +1.0f);
					}	
					break;
				case SDLK_RIGHT:
					if (!ctx->gamepad) {
						ctx->axis.x = SDL_max(ctx->axis.x - 1.0f, -1.0f);
					}
					break;
				case SDLK_UP:
					if (!ctx->gamepad) {
						ctx->player.vel.y = SDL_max(ctx->player.vel.y, ctx->player.vel.y*GRAVITY);
					}
					break;
				case SDLK_DOWN:

					break;
				}
				break;
			case SDL_EVENT_GAMEPAD_AXIS_MOTION:
				if (event.gaxis.axis == SDL_GAMEPAD_AXIS_LEFTX) {
					float value = (float)event.gaxis.value / 32767.0f;
					ctx->axis.x = value;
					// TODO: What do we do about the y axis?
				}

				break;
			case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
				if (event.gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH) {
					if (ctx->player.can_jump) {
						ctx->player.can_jump = 0;
						ctx->player.vel.y = -PLAYER_JUMP;
					}
				}
				break;
			case SDL_EVENT_GAMEPAD_BUTTON_UP:
				if (event.gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH) {
					ctx->player.vel.y = SDL_max(ctx->player.vel.y, ctx->player.vel.y/2.0f);
				}
				break;
			case SDL_EVENT_QUIT:
				ctx->running = false;
				break;
			}		
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

		if (!ctx->gamepad) {
			int joystick_count = 0;
			SDL_JoystickID* joysticks = SDL_GetGamepads(&joystick_count);
			if (joystick_count != 0) {
				ctx->gamepad = SDL_OpenGamepad(joysticks[0]);
			}
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

		//if (ctx->show_ui) {
			NK_UpdateUI(ctx);
		//}


		const float MOVE_SPEED_MIN = 1.0f;
		bool player_was_moving_x = SDL_fabsf(ctx->player.vel.x) >= MOVE_SPEED_MIN;
		bool player_was_moving_y = SDL_fabsf(ctx->player.vel.y) >= MOVE_SPEED_MIN; (void)player_was_moving_y;

		// PlayerMovement
		{
			if (ctx->player.can_jump) {
				float acc = ctx->axis.x * PLAYER_ACC;
				// ctx->player.vel.x += acc*ctx->dt;
				ctx->player.vel.x += acc;
				if (ctx->player.vel.x < 0.0f) ctx->player.vel.x = SDL_min(0.0f, ctx->player.vel.x + PLAYER_FRIC);
				else if (ctx->player.vel.x > 0.0f) ctx->player.vel.x = SDL_max(0.0f, ctx->player.vel.x - PLAYER_FRIC);
				ctx->player.vel.x = SDL_clamp(ctx->player.vel.x, -PLAYER_MAX_VEL, PLAYER_MAX_VEL);
			}
			// ctx->player.vel.y += GRAVITY*ctx->dt;
			ctx->player.vel.y += GRAVITY;
		}	

		// PlayerCollision
		{
			if (ctx->player.vel.x < 0.0f) {
				Rect side;
				side.pos.x = ctx->player.pos.x + ctx->player.vel.x;
				side.pos.y = ctx->player.pos.y + 1.0f;
				side.area.x = 1.0f;
				side.area.y = ctx->player.size.y - 2.0f;
				bool break_loop = false;
				for (size_t tile_y = 0; tile_y < ctx->level.h && !break_loop; tile_y += 1) {
					for (size_t tile_x = 0; tile_x < ctx->level.w && !break_loop; tile_x += 1) {
						if (GetTile(&ctx->level, tile_x, tile_y)) {
							Tile t;
							t.x = (int)tile_x;
							t.y = (int)tile_y;
							Rect tile = RectFromTile(t);
							if (RectsIntersect(&ctx->level, side, tile)) {
								ctx->player.pos.x = tile.pos.x + ctx->player.size.x;
								ctx->player.vel.x = -ctx->player.vel.y * PLAYER_BOUNCE;
								break_loop = true;
							}
						}
					}
				}
			} else if (ctx->player.vel.x > 0.0f) {
				Rect side;
				side.pos.x = (ctx->player.pos.x + ctx->player.size.x - 1.0f) + ctx->player.vel.x;
				side.pos.y = ctx->player.pos.y + 1.0f;
				side.area.x = 1.0f;
				side.area.y = ctx->player.size.y - 2.0f;
				bool break_loop = false;
				for (size_t tile_y = 0; tile_y < ctx->level.h && !break_loop; tile_y += 1) {
					for (size_t tile_x = 0; tile_x < ctx->level.w && !break_loop; tile_x += 1) {
						if (GetTile(&ctx->level, tile_x, tile_y)) {
							Rect tile = RectFromTile((Tile){(int)tile_x, (int)tile_y});
							if (RectsIntersect(&ctx->level, side, tile)) {
								ctx->player.pos.x = tile.pos.x - ctx->player.size.x;
								ctx->player.vel.x = -ctx->player.vel.y * PLAYER_BOUNCE;
								break_loop = true;
							}
						}
					}
				}
			}

			ctx->player.can_jump = SDL_max(0, ctx->player.can_jump - 1);
			if (ctx->player.vel.y < 0.0f) {
				Rect side;
				side.pos.x = ctx->player.pos.x + 1.0f; 
				side.pos.y = ctx->player.pos.y + ctx->player.vel.y;
				side.area.x = ctx->player.size.x - 2.0f;
				side.area.y = 1.0f;
				bool break_loop = false;
				for (size_t tile_y = 0; tile_y < ctx->level.h && !break_loop; tile_y += 1) {
					for (size_t tile_x = 0; tile_x < ctx->level.w && !break_loop; tile_x += 1) {
						if (GetTile(&ctx->level, tile_x, tile_y)) {
							Rect tile = RectFromTile((Tile){(int)tile_x, (int)tile_y});
							if (RectsIntersect(&ctx->level, side, tile)) {
								ctx->player.pos.y = tile.pos.y + ctx->player.size.y;
								ctx->player.vel.y = -ctx->player.vel.y * PLAYER_BOUNCE;
								break_loop = true;
							}
						}
					}
				}
			} else if (ctx->player.vel.y > 0.0f) {
				Rect side;
				side.pos.x = ctx->player.pos.x + 1.0f; 
				side.pos.y = (ctx->player.pos.y + ctx->player.size.y - 1.0f) + ctx->player.vel.y;
				side.area.x = ctx->player.size.x - 2.0f;
				side.area.y = 1.0f;
				bool break_loop = false;
				for (size_t tile_y = 0; tile_y < ctx->level.h && !break_loop; tile_y += 1) {
					for (size_t tile_x = 0; tile_x < ctx->level.w && !break_loop; tile_x += 1) {
						if (GetTile(&ctx->level, tile_x, tile_y)) {
							Rect tile = RectFromTile((Tile){(int)tile_x, (int)tile_y});
							if (RectsIntersect(&ctx->level, side, tile)) {
								ctx->player.pos.y = tile.pos.y - ctx->player.size.y;
								ctx->player.vel.y = -ctx->player.vel.y * PLAYER_BOUNCE;
								ctx->player.can_jump = PLAYER_JUMP_PERIOD;
								break_loop = true;
							}
						}
					}
				}
			}

			// vec2s vel_dt;
			// vel_dt = glms_vec2_scale(ctx->player.vel, ctx->dt);
			// ctx->player.pos = glms_vec2_add(ctx->player.pos, vel_dt);
			ctx->player.pos = glms_vec2_add(ctx->player.pos, ctx->player.vel);

			if (ctx->player.pos.y > (float)(ctx->level.h+500)) {
				ResetGame(ctx);
			}
		}

		bool player_is_moving_x = SDL_fabsf(ctx->player.vel.x) >= MOVE_SPEED_MIN;
		bool player_is_moving_y = SDL_fabsf(ctx->player.vel.y) >= MOVE_SPEED_MIN; (void)player_is_moving_y;
		if (!player_was_moving_x && player_is_moving_x) {
			ResetAnim(&ctx->player);
			SetSprite(ctx, &ctx->player, "assets\\legacy_fantasy_high_forest\\Character\\Run\\Run.aseprite");
		} else if (player_was_moving_x && !player_is_moving_x) {
			ResetAnim(&ctx->player);
			SetSprite(ctx, &ctx->player, "assets\\legacy_fantasy_high_forest\\Character\\Idle\\Idle.aseprite");			
		}

		// PlayerAnimation
		{
			SpriteDesc* s = GetSpriteDesc(ctx, ctx->player.sprite);

			ctx->player.frame_tick += 1;
			if (ctx->player.frame_tick >= (int32_t)s->frame_dur[ctx->player.frame]) {
				ctx->player.frame_tick = 0;
				ctx->player.frame += 1;
				if (ctx->player.frame >= s->n_frames) {
					ctx->player.frame = 0;
				}
			}
		}

		// RenderBegin
		{
			SDL_CHECK(SDL_SetRenderDrawColor(ctx->renderer, 100, 100, 100, 255));
			SDL_CHECK(SDL_RenderClear(ctx->renderer));
		}

		int32_t rw, rh;
		SDL_CHECK(SDL_GetRenderOutputSize(ctx->renderer, &rw, &rh));

		// RenderPlayer
		{
			SpriteDesc* s = GetSpriteDesc(ctx, ctx->player.sprite);
			SDL_FRect src = { (float)(ctx->player.frame*s->w), 0.0f, (float)s->w, (float)s->h };
			SDL_FRect dst = { (float)(rw/2), (float)(rh/2), (float)s->w, (float)s->h };
			SDL_CHECK(SDL_RenderTexture(ctx->renderer, s->texture, &src, &dst));
		}

		// RenderLevel
		{
			SDL_CHECK(SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 255, 0));
			for (size_t tile_y = 0; tile_y < ctx->level.h; tile_y += 1) {
				for (size_t tile_x = 0; tile_x < ctx->level.w; tile_x += 1) {
					if (GetTile(&ctx->level, tile_x, tile_y) == TILE_TYPE_GROUND) {
						SDL_FRect rect = { (float)(tile_x * TILE_SIZE) - ctx->player.pos.x + (float)(rw/2), (float)(tile_y * TILE_SIZE) - ctx->player.pos.y + (float)(rh/2), (float)TILE_SIZE, (float)TILE_SIZE };
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
		SDL_IOWriteVarF(fs, PLAYER_BOUNCE);
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

	const size_t RAW_CHUNK_MAX_SIZE = MEGABYTE(1);
	void* raw_chunk = SDL_malloc(RAW_CHUNK_MAX_SIZE); SDL_CHECK(raw_chunk);

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
			SpriteDesc* sprite_desc = &ctx->sprites[sprite];
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
			
			SDL_IOStream* fs;
			ASE_Header header;
			{
				fs = SDL_IOFromFile(sprite_path, "r"); SDL_CHECK(fs);

				SDL_ReadStructChecked(fs, &header);
				SDL_assert(header.magic_number == 0xA5E0);
				sprite_desc->n_frames = header.n_frames;
				sprite_desc->frame_dur = SDL_malloc(sizeof(uint16_t) * sprite_desc->n_frames); SDL_CHECK(sprite_desc->frame_dur);

				size_t sprite_path_len = SDL_strlen(sprite_path);
				
				const char end[] = ".aseprite"; size_t end_len = SDL_arraysize(end);
				if (sprite_desc->n_frames == 1) {
					size_t end_idx = sprite_path_len - end_len + 1;
					const char* png = "png";
					sprite_path[end_idx+1] = '\0';
					SDL_strlcat(sprite_path, png, SPRITE_PATH_SIZE);
				} else {
					size_t end_idx = sprite_path_len - end_len;
					const char* sheet = "-Sheet.png";
					sprite_path[end_idx+1] = '\0';
					SDL_strlcat(sprite_path, sheet, SPRITE_PATH_SIZE);				
				}

				sprite_desc->texture = IMG_LoadTexture(ctx->renderer, sprite_path); SDL_CHECK(sprite_desc->texture);
			}

			for (size_t frame_idx = 0; frame_idx < (size_t)header.n_frames; frame_idx += 1) {
				ASE_Frame frame;
				SDL_ReadStructChecked(fs, &frame);
				SDL_assert(frame.magic_number == 0xF1FA);

				// Would mean this aseprite file is very old.
				SDL_assert(frame.n_chunks != 0);

				sprite_desc->frame_dur[frame_idx] = (uint16_t)((float)frame.frame_dur / (1000.0f / 60.0f)); // TODO: delta time

				for (size_t chunk_idx = 0; chunk_idx < frame.n_chunks; chunk_idx += 1) {
					ASE_ChunkHeader chunk_header;
					SDL_ReadStructChecked(fs, &chunk_header);
					if (chunk_header.size == sizeof(ASE_ChunkHeader)) continue;

					SDL_assert(chunk_header.size - sizeof(ASE_ChunkHeader) <= RAW_CHUNK_MAX_SIZE);
					SDL_ReadIOChecked(fs, raw_chunk, chunk_header.size - sizeof(ASE_ChunkHeader));

					switch (chunk_header.type) {
					case ASE_CHUNK_TYPE_OLD_PALETTE: {
					} break;
					case ASE_CHUNK_TYPE_OLD_PALETTE2: {
					} break;
					case ASE_CHUNK_TYPE_LAYER: {
						// TODO: tileset_idx and uuid
						ASE_LayerChunk* chunk = raw_chunk;
						(void)chunk;
					} break;
					case ASE_CHUNK_TYPE_CELL: {
						ASE_CellChunk* chunk = raw_chunk;
						switch (chunk->type) {
						case ASE_CELL_TYPE_RAW: {
							sprite_desc->w = chunk->raw.w;
							sprite_desc->h = chunk->raw.h;
						} break;
						case ASE_CELL_TYPE_LINKED: {

						} break;
						case ASE_CELL_TYPE_COMPRESSED_IMAGE: {
							sprite_desc->w = chunk->compressed_image.w;
							sprite_desc->h = chunk->compressed_image.h;
						} break;
						case ASE_CELL_TYPE_COMPRESSED_TILEMAP: {
							sprite_desc->w = chunk->compressed_image.w;
							sprite_desc->h = chunk->compressed_image.h;					
						} break;
						}
					} break;
					case ASE_CHUNK_TYPE_CELL_EXTRA: {
						ASE_CellChunk* chunk = raw_chunk;
						(void)chunk;
						ASE_CellExtraChunk* extra_chunk = (ASE_CellExtraChunk*)((uintptr_t)raw_chunk + (uintptr_t)chunk_header.size - sizeof(ASE_CellExtraChunk));
						(void)extra_chunk;
					} break;
					case ASE_CHUNK_TYPE_COLOR_PROFILE: {
						ASE_ColorProfileChunk* chunk = raw_chunk;
						switch (chunk->type) {
						case ASE_COLOR_PROFILE_TYPE_NONE: {

						} break;
						case ASE_COLOR_PROFILE_TYPE_SRGB: {

						} break;
						case ASE_COLOR_PROFILE_TYPE_EMBEDDED_ICC: {

						} break;
						default: {
							SDL_assert(false);
						} break;
						}
					} break;
					case ASE_CHUNK_TYPE_EXTERNAL_FILES: {
						ASE_ExternalFilesChunk* chunk = raw_chunk;
						(void)chunk;
						SDL_Log("ASE_CHUNK_TYPE_EXTERNAL_FILES");
					} break;
					case ASE_CHUNK_TYPE_DEPRECATED_MASK: {
					} break;
					case ASE_CHUNK_TYPE_PATH: {
						SDL_Log("ASE_CHUNK_TYPE_PATH");
					} break;
					case ASE_CHUNK_TYPE_TAGS: {
						ASE_TagsChunk* chunk = raw_chunk;
						(void)chunk;
					} break;
					case ASE_CHUNK_TYPE_PALETTE: {
						ASE_PaletteChunk* chunk = raw_chunk;
						(void)chunk;
					} break;
					case ASE_CHUNK_TYPE_USER_DATA: {
						SDL_Log("ASE_CHUNK_TYPE_USER_DATA");
					} break;
					case ASE_CHUNK_TYPE_SLICE: {
						SDL_Log("ASE_CHUNK_TYPE_SLICE");
					} break;
					case ASE_CHUNK_TYPE_TILESET: {
						SDL_Log("ASE_CHUNK_TYPE_TILESET");
					} break;
					default: {
						SDL_assert(false);
					} break;
					}
				}
			}

			ctx->n_sprites += 1;

			SDL_CloseIO(fs);

		#if 1
			sprite_desc->w = (uint16_t)sprite_desc->texture->w / sprite_desc->n_frames;
			sprite_desc->h = (uint16_t)sprite_desc->texture->h;
		#else
			sprite_desc->w = header.grid_x + header.grid_w + sprite_desc->w;
			sprite_desc->h = header.grid_y + header.grid_h + sprite_desc->h;
			if (sprite_desc->h != (uint32_t)sprite_desc->texture->h) {
				char prefix[] = "assets\\legacy_fantasy_high_forest\\"; size_t prefix_len = SDL_arraysize(prefix);
				char* rel_path = sprite_path + prefix_len - 1;
				SDL_LogMessage(SDL_LOG_CATEGORY_ERROR, SDL_LOG_PRIORITY_CRITICAL, "%s: aseprite: w=%u, h=%u, SDL_Texture: w=%d, h=%d.", rel_path, sprite_desc->w, sprite_desc->h, sprite_desc->texture->w/(int32_t)sprite_desc->n_frames, sprite_desc->texture->h);
				ctx->sprite_tests_failed += 1;
			}
		#endif
		}
	} else if (n_files == 0) {
		SDL_CHECK(SDL_EnumerateDirectory(dir_path, EnumerateDirectoryCallback, ctx));
	}
	
	SDL_free(files);
	SDL_free(raw_chunk);
	return SDL_ENUM_CONTINUE;
}

void ResetAnim(Entity* entity) {
	entity->frame = 0;
	entity->frame_tick = 0;
}

SpriteDesc* GetSpriteDesc(Context* ctx, Sprite sprite) {
	return &ctx->sprites[sprite];
}

void SetSprite(Context* ctx, Entity* entity, const char* path) {
	entity->sprite = GetSprite((char*)path);
	SpriteDesc* sprite_desc = GetSpriteDesc(ctx, entity->sprite);
	entity->size.x = (float)sprite_desc->w;
	entity->size.y = (float)sprite_desc->h;
}