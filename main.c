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

#include "aseprite.h"

#include "main.h"

#include "variables.c"
#include "util.c"
#include "nuklear_util.c"

void ResetGame(Context* ctx) {
	ctx->dt = ctx->display_mode->refresh_rate;
	ctx->player = (Entity){.pos.x = TILE_SIZE*1.0f, .pos.y = TILE_SIZE*6.0f, .size.x = (float)(ctx->txtr_player_idle->w/PLAYER_FRAME_COUNT), .size.y = (float)ctx->txtr_player_idle->h};
}

void LoadSprite(Context* ctx, SDL_IOStream* fs) {
	(void)ctx;

	ASE_Header header;
	SDL_ReadStructChecked(fs, &header);
	SDL_assert(header.magic_number == 0xA5E0);

	const size_t RAW_CHUNK_MAX_SIZE = MEGABYTE(1);
	void* raw_chunk = SDL_malloc(RAW_CHUNK_MAX_SIZE); SDL_CHECK(raw_chunk); // TODO: use an area.

	for (size_t frame_idx = 0; frame_idx < (size_t)header.n_frames; frame_idx += 1) {
		ASE_Frame frame;
		SDL_ReadStructChecked(fs, &frame);
		SDL_assert(frame.magic_number == 0xF1FA);

		// Would mean this aseprite file is very old.
		SDL_assert(frame.n_chunks != 0);

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
				(void)chunk;
			} break;
			case ASE_CHUNK_TYPE_CELL_EXTRA: {
				ASE_CellChunk* chunk = raw_chunk;
				(void)chunk;
				ASE_CellChunkExtra* chunk_extra = (ASE_CellChunkExtra*)((uintptr_t)raw_chunk + (uintptr_t)chunk_header.size - sizeof(ASE_CellChunkExtra));
				(void)chunk_extra;
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
			} break;
			case ASE_CHUNK_TYPE_DEPRECATED_MASK: {
			} break;
			case ASE_CHUNK_TYPE_PATH: {
			} break;
			case ASE_CHUNK_TYPE_TAGS: {
			} break;
			case ASE_CHUNK_TYPE_PALETTE: {
			} break;
			case ASE_CHUNK_TYPE_USER_DATA: {
			} break;
			case ASE_CHUNK_TYPE_SLICE: {
			} break;
			case ASE_CHUNK_TYPE_TILESET: {
			} break;
			default: {
				SDL_assert(false);
			} break;
			}
		}
	}

	SDL_free(raw_chunk);
}

void DrawCircleFilled(SDL_Renderer* renderer, const int32_t cx, const int32_t cy, const int32_t r) {
	int32_t x = r;
	int32_t y = 0;
    
    SDL_CHECK(SDL_RenderLine(renderer, (float)cx, (float)cy, (float)(cx + x), (float)(cy + y)));
    SDL_CHECK(SDL_RenderLine(renderer, (float)cx, (float)cy, (float)(cx - x), (float)(cy + y)));
    SDL_CHECK(SDL_RenderLine(renderer, (float)cx, (float)cy, (float)(cx + x), (float)(cy - y)));
    SDL_CHECK(SDL_RenderLine(renderer, (float)cx, (float)cy, (float)(cx - x), (float)(cy - y)));

    SDL_CHECK(SDL_RenderLine(renderer, (float)cx, (float)cy, (float)(cx + y), (float)(cy + x)));
    SDL_CHECK(SDL_RenderLine(renderer, (float)cx, (float)cy, (float)(cx - y), (float)(cy + x)));
    SDL_CHECK(SDL_RenderLine(renderer, (float)cx, (float)cy, (float)(cx + y), (float)(cy - x)));
    SDL_CHECK(SDL_RenderLine(renderer, (float)cx, (float)cy, (float)(cx - y), (float)(cy - x)));

    int32_t point = 1 - r;
    while (x > y) { 
        y += 1;
        
        if (point <= 0) {
			point = point + 2*y + 1;
        } else {
            x -= 1;
            point = point + 2*y - 2*x + 1;
        }
        
        if (x < y) {
            break;
        }

        SDL_CHECK(SDL_RenderLine(renderer, (float)cx, (float)cy, (float)(cx + x), (float)(cy + y)));
        SDL_CHECK(SDL_RenderLine(renderer, (float)cx, (float)cy, (float)(cx - x), (float)(cy + y)));
        SDL_CHECK(SDL_RenderLine(renderer, (float)cx, (float)cy, (float)(cx + x), (float)(cy - y)));
        SDL_CHECK(SDL_RenderLine(renderer, (float)cx, (float)cy, (float)(cx - x), (float)(cy - y)));
        
        if (x != y) {
	        SDL_CHECK(SDL_RenderLine(renderer, (float)cx, (float)cy, (float)(cx + y), (float)(cy + x)));
	        SDL_CHECK(SDL_RenderLine(renderer, (float)cx, (float)cy, (float)(cx - y), (float)(cy + x)));
	        SDL_CHECK(SDL_RenderLine(renderer, (float)cx, (float)cy, (float)(cx + y), (float)(cy - x)));
	        SDL_CHECK(SDL_RenderLine(renderer, (float)cx, (float)cy, (float)(cx - y), (float)(cy - x)));   
        }
    }	
}

TileType GetTile(Level* level, size_t tile_x, size_t tile_y) {
	return level->tiles[tile_y*level->w + tile_x];
}

void SetTile(Level* level, size_t tile_x, size_t tile_y, TileType tile) {
	SDL_assert(tile_x < level->w && tile_y < level->h);
	level->tiles[tile_y*level->w + tile_x] = tile;
}

void LoadLevel(Context* ctx) {
	size_t file_size;
	char* file = (char*)SDL_LoadFile("level", &file_size); SDL_CHECK(file);
	ctx->level.h = 1;
	for (size_t file_i = 0, x = 0; file_i < file_size;) {
		x += 1;
		if (file[file_i] == '\r') {
			file_i += 2;
			ctx->level.w = SDL_max(x, ctx->level.w);
			x = 0;
			ctx->level.h += 1;
		} else {
			file_i += 1;
		}
	}

	ctx->level.tiles = SDL_malloc(sizeof(TileType) * ctx->level.w * ctx->level.h); SDL_CHECK(ctx->level.tiles);
	for (size_t i = 0; i < ctx->level.w*ctx->level.h; i += 1) {
		ctx->level.tiles[i] = TILE_TYPE_EMPTY;
	}
	// SDL_memset(ctx->level.tiles, (int32_t)(ctx->level.w * ctx->level.h), TILE_TYPE_EMPTY);

	for (size_t tile_y = 0, file_i = 0; tile_y < ctx->level.h; tile_y += 1) {
		for (size_t tile_x = 0; tile_x < ctx->level.w; tile_x += 1) {
			if (file[file_i] == '\r') {
				file_i += 2;
				break;
			}
			switch (file[file_i]) {
			case '1':
				SetTile(&ctx->level, tile_x, tile_y, TILE_TYPE_GROUND);
				break;
			}
			file_i += 1;
		}
	}

	SDL_free(file);
}

SDL_EnumerationResult EnumerateDirectoryCallback(void *userdata, const char *dirname, const char *fname) {
	Context* ctx = userdata;

	char path[1024];
	SDL_CHECK(SDL_snprintf(path, 1024, "%s%s", dirname, fname) >= 0);

	SDL_PathInfo path_info;
	SDL_CHECK(SDL_GetPathInfo(path, &path_info));
	if (path_info.type == SDL_PATHTYPE_DIRECTORY) {
		int32_t n_files;
		char** files = SDL_GlobDirectory(path, "*.aseprite", 0, &n_files);
		if (n_files == 0) {
			SDL_CHECK(SDL_EnumerateDirectory(path, EnumerateDirectoryCallback, ctx));
		} else {
			for (size_t file_idx = 0; file_idx < (size_t)n_files; file_idx += 1) {
				char aseprite_filepath[2048];
				SDL_CHECK(SDL_snprintf(aseprite_filepath, 2048, "%s\\%s", path, files[file_idx]) >= 0);

				uint32_t hash = HashString(aseprite_filepath, 0, ctx->seed);
				SDL_Log("%s => %u", aseprite_filepath, hash);

				SDL_IOStream* fs = SDL_IOFromFile(aseprite_filepath, "r"); SDL_CHECK(fs);
				LoadSprite(ctx, fs);
				SDL_CloseIO(fs);
			}

			SDL_free(files);
		}
	}
		
	return SDL_ENUM_CONTINUE;
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
		ctx->seed = (uint32_t)ctx->time;
	}

	// CreateWindowAndRenderer
	{
		SDL_DisplayID display = SDL_GetPrimaryDisplay();
		ctx->display_mode = (SDL_DisplayMode *)SDL_GetDesktopDisplayMode(display); SDL_CHECK(ctx->display_mode);
		SDL_WindowFlags flags = SDL_WINDOW_HIGH_PIXEL_DENSITY;
		int32_t w = ctx->display_mode->w / 2;
		int32_t h = ctx->display_mode->h / 2;

#ifndef _DEBUG
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
		SDL_SetStringProperty(props, TTF_PROP_FONT_CREATE_FILENAME_STRING, "assets/fonts/Roboto-Regular.ttf");
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
		bool ok = nk_init_fixed(&ctx->nk.ctx, SDL_malloc(MEGABYTE(64)), MEGABYTE(64), &ctx->nk.font); SDL_assert(ok);
	}

	// InitLevel
	{
		LoadLevel(ctx);

		SDL_PathInfo info;
		SDL_CHECK(SDL_GetPathInfo("level", &info));
		ctx->level.modify_time = info.modify_time;
	}

	// LoadTextures
	{
		ctx->txtr_player_idle = IMG_LoadTexture(ctx->renderer, "assets/legacy_fantasy_high_forest/Character/Idle/Idle-Sheet.png"); SDL_CHECK(ctx->txtr_player_idle);
	}

	SDL_CHECK(SDL_EnumerateDirectory("assets/legacy_fantasy_high_forest", EnumerateDirectoryCallback, ctx));

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

		// UpdateUI
		{
			if (nk_begin(&ctx->nk.ctx, "Show", nk_rect(10, 10, 500, 220),
			    NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE)) {
				{
					float height = 20.0f;
					int cols = 2;
				    nk_layout_row_dynamic(&ctx->nk.ctx, height, cols);
				}

				#define nk_varf(ctx, var, min, max, incr) STMT( \
					nk_labelf(ctx, NK_TEXT_LEFT, STRINGIFY(var: %f), var); \
					nk_slider_float(ctx, min, &var, max, incr); \
				)
				#define nk_vari(ctx, var, min, max, incr) STMT( \
					nk_labelf(ctx, NK_TEXT_LEFT, STRINGIFY(var: %d), var); \
					nk_slider_int(ctx, min, &var, max, incr); \
				)
				nk_varf(&ctx->nk.ctx, GRAVITY, 0.0f, 1.0f, 0.01f);
				nk_varf(&ctx->nk.ctx, PLAYER_ACC, 0.0f, 1.0f, 0.01f);
				nk_varf(&ctx->nk.ctx, PLAYER_FRIC, 0.0f, 1.0f, 0.01f);
				nk_varf(&ctx->nk.ctx, PLAYER_MAX_VEL, 0.0f, 10.0f, 0.1f);
				nk_varf(&ctx->nk.ctx, PLAYER_JUMP, 0.0f, 30.0f, 0.5f);
				nk_varf(&ctx->nk.ctx, PLAYER_BOUNCE, 0.0f, 1.0f, 0.01f);
				nk_vari(&ctx->nk.ctx, TILE_SIZE, 0, 256, 2);
				nk_vari(&ctx->nk.ctx, PLAYER_JUMP_PERIOD, 0, 10, 1);
			}
			nk_end(&ctx->nk.ctx);
		}

		// PlayerAnimation
		{
			ctx->player.frame_tick += 1;
			if (ctx->player.frame_tick > PLAYER_FRAME_TICK) {
				ctx->player.frame_tick = 0;
				ctx->player.frame += 1;
				if (ctx->player.frame >= PLAYER_FRAME_COUNT) {
					ctx->player.frame = 0;
				}
			}
		}

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

			if (ctx->player.pos.y > (float)((ctx->level.h+5)*ctx->player.size.y)) {
				ResetGame(ctx);
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
			
			// SDL_CHECK(SDL_SetRenderDrawColor(ctx->renderer, 0, 255, 0, 0));
			// SDL_FRect rect = { (float)(rw/2), (float)(rh/2), (float)TILE_SIZE, (float)TILE_SIZE };
			// SDL_CHECK(SDL_RenderFillRect(ctx->renderer, &rect));

			SDL_FRect src = { (float)(ctx->player.frame*PLAYER_FRAME_WIDTH), 0.0f, (float)PLAYER_FRAME_WIDTH, (float)ctx->txtr_player_idle->h };
			SDL_FRect dst = { (float)(rw/2), (float)(rh/2), (float)PLAYER_FRAME_WIDTH, (float)ctx->txtr_player_idle->h };
			SDL_CHECK(SDL_RenderTexture(ctx->renderer, ctx->txtr_player_idle, &src, &dst));
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

void DrawCircle(SDL_Renderer* renderer, const int32_t cx, const int32_t cy, const int32_t r) {
	int32_t x = r;
	int32_t y = 0;
    
    SDL_RenderPoint(renderer, (float)(cx + x), (float)(cy + y));
    SDL_RenderPoint(renderer, (float)(cx - x), (float)(cy + y));
    SDL_RenderPoint(renderer, (float)(cx + x), (float)(cy - y));
    SDL_RenderPoint(renderer, (float)(cx - x), (float)(cy - y));

    SDL_RenderPoint(renderer, (float)(cx + y), (float)(cy + x));
    SDL_RenderPoint(renderer, (float)(cx - y), (float)(cy + x));
    SDL_RenderPoint(renderer, (float)(cx + y), (float)(cy - x));
    SDL_RenderPoint(renderer, (float)(cx - y), (float)(cy - x));

    int32_t point = 1 - r;
    while (x > y)
    { 
        y += 1;
        
        if (point <= 0) {
			point = point + 2*y + 1;
        }
        else
        {
            x -= 1;
            point = point + 2*y - 2*x + 1;
        }
        
        if (x < y) {
            break;
        }

        SDL_RenderPoint(renderer, (float)(cx + x), (float)(cy + y));
        SDL_RenderPoint(renderer, (float)(cx - x), (float)(cy + y));
        SDL_RenderPoint(renderer, (float)(cx + x), (float)(cy - y));
        SDL_RenderPoint(renderer, (float)(cx - x), (float)(cy - y));
        
        if (x != y)
        {
	        SDL_RenderPoint(renderer, (float)(cx + y), (float)(cy + x));
	        SDL_RenderPoint(renderer, (float)(cx - y), (float)(cy + x));
	        SDL_RenderPoint(renderer, (float)(cx + y), (float)(cy - x));
	        SDL_RenderPoint(renderer, (float)(cx - y), (float)(cy - x));   
        }
    }	

}

uint32_t HashString(char* key, int32_t len, uint32_t seed) {
	if (len == 0) {
		len = (int32_t)SDL_strlen(key);
	}
	return nk_murmur_hash((const void*)key, len, seed);
}