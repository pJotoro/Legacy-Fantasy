#include <SDL.h>
#include <SDL_main.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include <cglm/struct.h>

#define STBDS_NO_SHORT_NAMES
#include <stb_ds.h>

#include "nuklear_defines.h"
#pragma warning(push, 0)
#include <nuklear.h>
#pragma warning(pop)

#include "main.h"

#define LEVEL_WIDTH 15
#define LEVEL_HEIGHT 15
static int level[LEVEL_HEIGHT][LEVEL_WIDTH] = {
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0},
	{0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 0},
	{1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

#include "variables.c"
#include "util.c"
#include "nuklear_util.c"

void reset_game(Context* ctx) {
	ctx->dt = ctx->display_mode->refresh_rate;
	ctx->player = (Entity){.pos.x = TILE_SIZE*1.0f, .pos.y = TILE_SIZE*6.0f};
}

#ifdef SDL_CHECK
#undef SDL_CHECK
#endif
#ifdef CHECK
#undef CHECK
#endif
#define SDL_CHECK(E) STMT(if (!E) { SDL_Log("SDL: %s.", SDL_GetError()); res = -1; })
#define CHECK(E) STMT(if (!E) { res = -1; })

int main(int argc, char* argv[]) {
	int res = 0;

	SDL_CHECK(SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD));
	SDL_CHECK(TTF_Init());

	Context* ctx = new(1, Context); SDL_CHECK(ctx);
	memset(ctx, 0, sizeof(Context));

	// init_time
	{
		SDL_CHECK(SDL_GetCurrentTime(&ctx->time));
		stbds_rand_seed((size_t)ctx->time);
	}

	// create_window_and_renderer
	{
		SDL_DisplayID display = SDL_GetPrimaryDisplay();
		ctx->display_mode = (SDL_DisplayMode *)SDL_GetDesktopDisplayMode(display); SDL_CHECK(ctx->display_mode);
		SDL_WindowFlags flags = SDL_WINDOW_HIGH_PIXEL_DENSITY;
		int w = ctx->display_mode->w / 2;
		int h = ctx->display_mode->h / 2;

#ifndef _DEBUG
		flags |= SDL_WINDOW_FULLSCREEN;
		w = ctx->display_mode->w;
		h = ctx->display_mode->h;
#endif

		SDL_CHECK(SDL_CreateWindowAndRenderer("Platformer", w, h, flags, &ctx->window, &ctx->renderer));

		ctx->vsync = SDL_SetRenderVSync(ctx->renderer, SDL_RENDERER_VSYNC_ADAPTIVE);
		if (!ctx->vsync) {
			ctx->vsync = SDL_SetRenderVSync(ctx->renderer, 1); // fixed refresh rate
		}

		ctx->display_content_scale = SDL_GetDisplayContentScale(display);
	}

	// text_engine_init
	{
		ctx->text_engine = TTF_CreateRendererTextEngine(ctx->renderer); SDL_CHECK(ctx->text_engine);
	}

	// load_font
	{
		SDL_PropertiesID props = SDL_CreateProperties(); SDL_CHECK(props);
		SDL_SetStringProperty(props, TTF_PROP_FONT_CREATE_FILENAME_STRING, "fonts/Roboto-Regular.ttf");
		SDL_SetFloatProperty(props, TTF_PROP_FONT_CREATE_SIZE_FLOAT, 15.0f);
		SDL_SetNumberProperty(props, TTF_PROP_FONT_CREATE_HORIZONTAL_DPI_NUMBER, (int64_t)(72.0f*ctx->display_content_scale));
		SDL_SetNumberProperty(props, TTF_PROP_FONT_CREATE_VERTICAL_DPI_NUMBER, (int64_t)(72.0f*ctx->display_content_scale));
		ctx->font_roboto_regular = TTF_OpenFontWithProperties(props); SDL_CHECK(ctx->font_roboto_regular);
		SDL_DestroyProperties(props);
	}

	// nuklear_init
	{
		ctx->nk.font.userdata.ptr = ctx->font_roboto_regular;
		ctx->nk.font.height = (float)TTF_GetFontHeight(ctx->font_roboto_regular);
		ctx->nk.font.width = nk_cb_text_width;
		CHECK(nk_init_fixed(&ctx->nk.ctx, SDL_malloc(MEGABYTE(64)), MEGABYTE(64), &ctx->nk.font));
	}

	reset_game(ctx);

	ctx->running = true;
	while (ctx->running && res == 0) {
		nk_input_begin(&ctx->nk.ctx);

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			nk_handle_event(ctx, &event);

			switch (event.type) {
			case SDL_EVENT_KEY_DOWN:
				switch (event.key.key) {
				case SDLK_ESCAPE:
					ctx->running = false;
					break;
				case SDLK_LEFT:
					if (!ctx->gamepad) {
						ctx->axis.x = max(ctx->axis.x - 1.0f, -1.0f);
					}
					break;
				case SDLK_RIGHT:
					if (!ctx->gamepad) {
						ctx->axis.x = min(ctx->axis.x + 1.0f, +1.0f);
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
					reset_game(ctx);
					break;
				}
				break;
			case SDL_EVENT_KEY_UP:
				switch (event.key.key) {
				case SDLK_LEFT:
					if (!ctx->gamepad) {
						ctx->axis.x = min(ctx->axis.x + 1.0f, +1.0f);
					}	
					break;
				case SDLK_RIGHT:
					if (!ctx->gamepad) {
						ctx->axis.x = max(ctx->axis.x - 1.0f, -1.0f);
					}
					break;
				case SDLK_UP:
					if (!ctx->gamepad) {
						ctx->player.vel.y = max(ctx->player.vel.y, ctx->player.vel.y*GRAVITY);
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
					ctx->player.vel.y = max(ctx->player.vel.y, ctx->player.vel.y/2.0f);
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

		if (ctx->player.vel.y > 81.0f) {
			SDL_Log("Blah");
		}

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

		// player_movement 
		{
			if (ctx->player.can_jump) {
				float acc = ctx->axis.x * PLAYER_ACC;
				ctx->player.vel.x += acc*ctx->dt;
				if (ctx->player.vel.x < 0.0f) ctx->player.vel.x = min(0.0f, ctx->player.vel.x + PLAYER_FRIC);
				else if (ctx->player.vel.x > 0.0f) ctx->player.vel.x = max(0.0f, ctx->player.vel.x - PLAYER_FRIC);
				ctx->player.vel.x = clamp(ctx->player.vel.x, -PLAYER_MAX_VEL, PLAYER_MAX_VEL);
			}
			ctx->player.vel.y += GRAVITY*ctx->dt;
		}	

		// player_collision
		{
			if (ctx->player.vel.x < 0.0f) {
				Rect side;
				side.pos.x = ctx->player.pos.x + ctx->player.vel.x;
				side.pos.y = ctx->player.pos.y + 1.0f;
				side.area.x = 1.0f;
				side.area.y = TILE_SIZE - 2.0f;
				bool break_loop = false;
				for (ssize_t tile_y = 0; tile_y < LEVEL_HEIGHT && !break_loop; tile_y += 1) {
					for (ssize_t tile_x = 0; tile_x < LEVEL_WIDTH && !break_loop; tile_x += 1) {
						if (level[tile_y][tile_x]) {
							Tile t;
							t.x = (int)tile_x;
							t.y = (int)tile_y;
							Rect tile = rect_from_tile(t);
							if (rects_intersect(side, tile)) {
								ctx->player.pos.x = tile.pos.x + TILE_SIZE;
								ctx->player.vel.x = -ctx->player.vel.y * PLAYER_BOUNCE;
								break_loop = true;
							}
						}
					}
				}
			} else if (ctx->player.vel.x > 0.0f) {
				Rect side;
				side.pos.x = (ctx->player.pos.x + TILE_SIZE - 1.0f) + ctx->player.vel.x;
				side.pos.y = ctx->player.pos.y + 1.0f;
				side.area.x = 1.0f;
				side.area.y = TILE_SIZE - 2.0f;
				bool break_loop = false;
				for (ssize_t tile_y = 0; tile_y < LEVEL_HEIGHT && !break_loop; tile_y += 1) {
					for (ssize_t tile_x = 0; tile_x < LEVEL_WIDTH && !break_loop; tile_x += 1) {
						if (level[tile_y][tile_x]) {
							Rect tile = rect_from_tile((Tile){(int)tile_x, (int)tile_y});
							if (rects_intersect(side, tile)) {
								ctx->player.pos.x = tile.pos.x - TILE_SIZE;
								ctx->player.vel.x = -ctx->player.vel.y * PLAYER_BOUNCE;
								break_loop = true;
							}
						}
					}
				}
			}

			ctx->player.can_jump = max(0, ctx->player.can_jump - 1);
			if (ctx->player.vel.y < 0.0f) {
				Rect side;
				side.pos.x = ctx->player.pos.x + 1.0f; 
				side.pos.y = ctx->player.pos.y + ctx->player.vel.y;
				side.area.x = TILE_SIZE - 2.0f;
				side.area.y = 1.0f;
				bool break_loop = false;
				for (ssize_t tile_y = 0; tile_y < LEVEL_HEIGHT && !break_loop; tile_y += 1) {
					for (ssize_t tile_x = 0; tile_x < LEVEL_WIDTH && !break_loop; tile_x += 1) {
						if (level[tile_y][tile_x]) {
							Rect tile = rect_from_tile((Tile){(int)tile_x, (int)tile_y});
							if (rects_intersect(side, tile)) {
								ctx->player.pos.y = tile.pos.y + TILE_SIZE;
								ctx->player.vel.y = -ctx->player.vel.y * PLAYER_BOUNCE;
								break_loop = true;
							}
						}
					}
				}
			} else if (ctx->player.vel.y > 0.0f) {
				Rect side;
				side.pos.x = ctx->player.pos.x + 1.0f; 
				side.pos.y = (ctx->player.pos.y + TILE_SIZE - 1.0f) + ctx->player.vel.y;
				side.area.x = TILE_SIZE - 2.0f;
				side.area.y = 1.0f;
				bool break_loop = false;
				for (ssize_t tile_y = 0; tile_y < LEVEL_HEIGHT && !break_loop; tile_y += 1) {
					for (ssize_t tile_x = 0; tile_x < LEVEL_WIDTH && !break_loop; tile_x += 1) {
						if (level[tile_y][tile_x]) {
							Rect tile = rect_from_tile((Tile){(int)tile_x, (int)tile_y});
							if (rects_intersect(side, tile)) {
								ctx->player.pos.y = tile.pos.y - TILE_SIZE;
								ctx->player.vel.y = -ctx->player.vel.y * PLAYER_BOUNCE;
								ctx->player.can_jump = PLAYER_JUMP_PERIOD;
								break_loop = true;
							}
						}
					}
				}
			}

			vec2s vel_dt;
			vel_dt = glms_vec2_scale(ctx->player.vel, ctx->dt);
			ctx->player.pos = glms_vec2_add(ctx->player.pos, vel_dt);

			Rect player_rect = (Rect){ctx->player.pos, (vec2s){TILE_SIZE, TILE_SIZE}};
			if (!rect_is_valid(player_rect)) {
				reset_game(ctx);
			}
		}

		// update_ui
		{
			#define EASY 0
			#define HARD 1

			static int op = EASY;
			static float value = 0.6f;
			static int i =  20;

			if (nk_begin(&ctx->nk.ctx, "Show", nk_rect(50, 50, 220, 220),
			    NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_CLOSABLE|NK_WINDOW_SCALABLE)) {
			    // fixed widget pixel width
			    nk_layout_row_static(&ctx->nk.ctx, 30, 80, 1);
			    if (nk_button_label(&ctx->nk.ctx, "button")) {
			        // event handling
			    }
			 
			    // fixed widget window ratio width
			    nk_layout_row_dynamic(&ctx->nk.ctx, 30, 2);
			    if (nk_option_label(&ctx->nk.ctx, "easy", op == EASY)) op = EASY;
			    if (nk_option_label(&ctx->nk.ctx, "hard", op == HARD)) op = HARD;
			 
			    // custom widget pixel width
			    nk_layout_row_begin(&ctx->nk.ctx, NK_STATIC, 30, 2);
			    {
			        nk_layout_row_push(&ctx->nk.ctx, 50);
			        nk_label(&ctx->nk.ctx, "Volume:", NK_TEXT_LEFT);
			        nk_layout_row_push(&ctx->nk.ctx, 110);
			        nk_slider_float(&ctx->nk.ctx, 0, &value, 1.0f, 0.1f);
			    }
			    nk_layout_row_end(&ctx->nk.ctx);
			}
			nk_end(&ctx->nk.ctx);
		}

		// render_begin
		{
			SDL_CHECK(SDL_SetRenderDrawColor(ctx->renderer, 100, 100, 100, 255));
			SDL_CHECK(SDL_RenderClear(ctx->renderer));
		}

		// render_player
		{
			SDL_CHECK(SDL_SetRenderDrawColor(ctx->renderer, 0, 255, 0, 0));
			SDL_FRect rect = { ctx->player.pos.x, ctx->player.pos.y, TILE_SIZE, TILE_SIZE };
			SDL_CHECK(SDL_RenderFillRect(ctx->renderer, &rect));
		}

		// render_level
		{
			SDL_CHECK(SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 255, 0));
			for (ssize_t level_y = 0; level_y < LEVEL_HEIGHT; level_y += 1) {
				for (ssize_t level_x = 0; level_x < LEVEL_WIDTH; level_x += 1) {
					if (level[level_y][level_x]) {
						SDL_FRect rect = { (float)level_x * TILE_SIZE, (float)level_y * TILE_SIZE, TILE_SIZE, TILE_SIZE };
						SDL_CHECK(SDL_RenderFillRect(ctx->renderer, &rect));
					}
				}
			}
		}

		// render_end
		{
			CHECK(nk_render(ctx));

			SDL_CHECK(SDL_RenderPresent(ctx->renderer));

			nk_clear(&ctx->nk.ctx);

			if (!ctx->vsync) {
				// TODO
			}
		}


	}
	
	return res; 
}

void draw_circle_filled(SDL_Renderer* renderer, const int32_t cx, const int32_t cy, const int32_t r) {
	int32_t x = r;
	int32_t y = 0;
    
    SDL_RenderLine(renderer, cx, cy, cx + x, cy + y);
    SDL_RenderLine(renderer, cx, cy, cx - x, cy + y);
    SDL_RenderLine(renderer, cx, cy, cx + x, cy - y);
    SDL_RenderLine(renderer, cx, cy, cx - x, cy - y);

    SDL_RenderLine(renderer, cx, cy, cx + y, cy + x);
    SDL_RenderLine(renderer, cx, cy, cx - y, cy + x);
    SDL_RenderLine(renderer, cx, cy, cx + y, cy - x);
    SDL_RenderLine(renderer, cx, cy, cx - y, cy - x);

    int point = 1 - r;
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

        SDL_RenderLine(renderer, cx, cy, cx + x, cy + y);
        SDL_RenderLine(renderer, cx, cy, cx - x, cy + y);
        SDL_RenderLine(renderer, cx, cy, cx + x, cy - y);
        SDL_RenderLine(renderer, cx, cy, cx - x, cy - y);
        
        if (x != y)
        {
	        SDL_RenderLine(renderer, cx, cy, cx + y, cy + x);
	        SDL_RenderLine(renderer, cx, cy, cx - y, cy + x);
	        SDL_RenderLine(renderer, cx, cy, cx + y, cy - x);
	        SDL_RenderLine(renderer, cx, cy, cx - y, cy - x);   
        }
    }	

}

// void draw_circle(SDL_Renderer* renderer, int32_t center_x, int32_t center_y, int32_t radius)
// {
//    const int32_t diameter = (radius * 2);

//    int32_t x = (radius - 1);
//    int32_t y = 0;
//    int32_t tx = 1;
//    int32_t ty = 1;
//    int32_t error = (tx - diameter);

//    while (x >= y)
//    {
//       //  Each of the following renders an octant of the circle
//       SDL_RenderPoint(renderer, center_x + x, center_y - y);
//       SDL_RenderPoint(renderer, center_x + x, center_y + y);
//       SDL_RenderPoint(renderer, center_x - x, center_y - y);
//       SDL_RenderPoint(renderer, center_x - x, center_y + y);
//       SDL_RenderPoint(renderer, center_x + y, center_y - x);
//       SDL_RenderPoint(renderer, center_x + y, center_y + x);
//       SDL_RenderPoint(renderer, center_x - y, center_y - x);
//       SDL_RenderPoint(renderer, center_x - y, center_y + x);

//       if (error <= 0)
//       {
//          ++y;
//          error += ty;
//          ty += 2;
//       }

//       if (error > 0)
//       {
//          --x;
//          tx += 2;
//          error += (tx - diameter);
//       }
//    }
// }

// void draw_circle(SDL_Renderer* renderer, const int x, const int y, const int radius) {
// 	SDL_Rect viewport;
// 	SDL_GetRenderViewport(renderer, &viewport);
// 	int xc = (viewport.x + viewport.w) / 2;
// 	int yc = (viewport.y + viewport.h) / 2;
//     for(int i = 0; i < 360; i ++) {                                               
//     	int xx = x + radius * cosf(i);
//     	int yy = y + radius * sinf(i);
//     	SDL_RenderPoint(renderer, xx, yy); 
// 	}
// }
