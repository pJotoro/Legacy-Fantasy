#define SDL_MAIN_USE_CALLBACKS 1
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

#include "variables.c"

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

#include "util.c"

typedef struct Entity {
	vec2s pos;
	vec2s vel;
	int can_jump;
} Entity;

typedef struct Nuklear {
	struct nk_context ctx;
	struct nk_user_font font;
	SDL_FRect scissor;
} Nuklear;

typedef struct Context {
	SDL_Window* window;
	SDL_Renderer* renderer;
	bool vsync;
	Entity player;
	SDL_Gamepad* gamepad;
	SDL_DisplayMode* display_mode;
	vec2s axis;
	SDL_Time time;
	float dt;
	Nuklear nk;
	TTF_Font* font_roboto_regular;
	TTF_TextEngine* text_engine;
	float display_content_scale;
} Context;

void reset_game(Context* ctx) {
	ctx->dt = ctx->display_mode->refresh_rate;
	ctx->player = (Entity){.pos.x = TILE_SIZE*1.0f, .pos.y = TILE_SIZE*6.0f};
}

#define SDL_CHECK(E) STMT(if (!E) { SDL_Log("SDL: %s.", SDL_GetError()); res = SDL_APP_FAILURE; })
#define CHECK(E) STMT(if (!E) { res = SDL_APP_FAILURE; })

float nk_cb_text_width(nk_handle handle, float height, const char *text, int len) {
	TTF_Font* font = handle.ptr;
	const int MAX_WIDTH = 256;
	int measured_width;
	size_t measured_length;
	float res = 0.0f;
	if (TTF_MeasureString(font, text, (size_t)len, MAX_WIDTH, &measured_width, &measured_length)) {
		res = (float)measured_width;
	}
	return res;
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
	SDL_AppResult res = SDL_APP_CONTINUE;

	SDL_CHECK(SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD));
	SDL_CHECK(TTF_Init());

	Context* ctx = new(1, Context); SDL_CHECK(ctx);
	memset(ctx, 0, sizeof(Context));
	*appstate = ctx;

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
	
	return res; 
}

/*
TODO:
- Delta time.
- Buy Legacy-Fantasy asset and render something in it.
- Make CMake hide full paths.
- Make CMake give more warnings and consider more warnings errors.
- Make Sublime Text understand C errors.
*/

bool nk_render(Context* ctx) {
	SDL_AppResult res = SDL_APP_SUCCESS;

	const struct nk_command* cmd = NULL;
	nk_foreach(cmd, &ctx->nk.ctx) {
		switch (cmd->type) {
	        case NK_COMMAND_NOP: {
	        	SDL_Log("NOP");
	        } break;
		    case NK_COMMAND_SCISSOR: {
	        	const struct nk_command_scissor* c = (const struct nk_command_scissor*)cmd;
	        	const SDL_Rect clip = {c->x, c->y, c->w, c->h};
	        	SDL_CHECK(SDL_SetRenderClipRect(ctx->renderer, &clip));
		    } break;
		    case NK_COMMAND_LINE: {
	        	SDL_Log("LINE");
		    } break;
		    case NK_COMMAND_CURVE: {
	        	SDL_Log("CURVE");
		    } break;
		    case NK_COMMAND_RECT: {
	        	const struct nk_command_rect* c = (const struct nk_command_rect*)cmd;
	        	SDL_FRect rect = {(float)c->x, (float)c->y, (float)c->w, (float)c->h};
	        	SDL_CHECK(SDL_SetRenderDrawColor(ctx->renderer, c->color.r, c->color.g, c->color.b, c->color.a));
	        	SDL_CHECK(SDL_RenderRect(ctx->renderer, &rect));
		    } break;
		    case NK_COMMAND_RECT_FILLED: {
	        	const struct nk_command_rect_filled* c = (const struct nk_command_rect_filled*)cmd;
	        	SDL_FRect rect = {(float)c->x, (float)c->y, (float)c->w, (float)c->h};
	        	SDL_CHECK(SDL_SetRenderDrawColor(ctx->renderer, c->color.r, c->color.g, c->color.b, c->color.a));
	        	SDL_CHECK(SDL_RenderFillRect(ctx->renderer, &rect));
		    } break;
		    case NK_COMMAND_RECT_MULTI_COLOR: {
	        	SDL_Log("RECT_MULTI_COLOR");
		    } break;
		    case NK_COMMAND_CIRCLE: {
		    	// TODO: Actually draw a circle.
	        	const struct nk_command_circle* c = (const struct nk_command_circle*)cmd;
	        	SDL_FRect rect = {(float)c->x, (float)c->y, (float)c->w, (float)c->h};
	        	SDL_CHECK(SDL_SetRenderDrawColor(ctx->renderer, c->color.r, c->color.g, c->color.b, c->color.a));
	        	SDL_CHECK(SDL_RenderRect(ctx->renderer, &rect));
		    } break;
		    case NK_COMMAND_CIRCLE_FILLED: {
		    	// TODO: Actually draw a circle.
	        	const struct nk_command_circle_filled* c = (const struct nk_command_circle_filled*)cmd;
	        	SDL_FRect rect = {(float)c->x, (float)c->y, (float)c->w, (float)c->h};
	        	SDL_CHECK(SDL_SetRenderDrawColor(ctx->renderer, c->color.r, c->color.g, c->color.b, c->color.a));
	        	SDL_CHECK(SDL_RenderRect(ctx->renderer, &rect));
		    } break;
		    case NK_COMMAND_ARC: {
	        	SDL_Log("ARC");
		    } break;
		    case NK_COMMAND_ARC_FILLED: {
	        	SDL_Log("ARC_FILLED");
		    } break;
		    case NK_COMMAND_TRIANGLE: {
	        	SDL_Log("TRIANGLE");
		    } break;
		    case NK_COMMAND_TRIANGLE_FILLED: {
	        	SDL_Log("TRIANGLE_FILLED");
		    } break;
		    case NK_COMMAND_POLYGON: {
	        	SDL_Log("POLYGON");
		    } break;
		    case NK_COMMAND_POLYGON_FILLED: {
	        	SDL_Log("POLYGON_FILLED");
		    } break;
		    case NK_COMMAND_POLYLINE: {
	        	SDL_Log("POLYLINE");
		    } break;
		    case NK_COMMAND_TEXT: {
	        	const struct nk_command_text* c = (const struct nk_command_text*)cmd;
	        	TTF_Text* text = TTF_CreateText(ctx->text_engine, ctx->font_roboto_regular, c->string, (size_t)c->length); SDL_CHECK(text);
	        	// TODO: text color
	        	SDL_CHECK(TTF_DrawRendererText(text, (float)c->x, (float)c->y));
	        	TTF_DestroyText(text);
		    } break;
		    case NK_COMMAND_IMAGE: {
	        	SDL_Log("IMAGE");
		    } break;
		    case NK_COMMAND_CUSTOM: {
	        	SDL_Log("CUSTOM");
		    } break;
		}
	}

	if (res == SDL_APP_FAILURE) return false;
	else return true;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
	SDL_AppResult res = SDL_APP_CONTINUE;
	Context* ctx = appstate;

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
		    NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_CLOSABLE)) {
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

	CHECK(nk_render(ctx));

	// render_end
	{
		SDL_CHECK(SDL_RenderPresent(ctx->renderer));

		nk_clear(&ctx->nk.ctx);

		if (!ctx->vsync) {
			// TODO
		}
	}

	return res; 
}

int nk_handle_event(Context* ctx, SDL_Event* event);

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
	SDL_AppResult res = SDL_APP_CONTINUE;
	Context* ctx = appstate;

	nk_handle_event(ctx, event);

	switch (event->type) {
	case SDL_EVENT_KEY_DOWN:
		switch (event->key.key) {
		case SDLK_ESCAPE:
			res = SDL_APP_SUCCESS;
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
				if (!event->key.repeat && ctx->player.can_jump) {
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
		switch (event->key.key) {
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
		if (event->gaxis.axis == SDL_GAMEPAD_AXIS_LEFTX) {
			float value = (float)event->gaxis.value / 32767.0f;
			ctx->axis.x = value;
			// TODO: What do we do about the y axis?
		}

		break;
	case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
		if (event->gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH) {
			if (ctx->player.can_jump) {
				ctx->player.can_jump = 0;
				ctx->player.vel.y = -PLAYER_JUMP;
			}
		}
		break;
	case SDL_EVENT_GAMEPAD_BUTTON_UP:
		if (event->gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH) {
			ctx->player.vel.y = max(ctx->player.vel.y, ctx->player.vel.y/2.0f);
		}
		break;
	case SDL_EVENT_QUIT:
		res = SDL_APP_SUCCESS;
		break;
	}

	return res; 
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {}

int
nk_handle_event(Context* ctx, SDL_Event* event)
{
    int ctrl_down = SDL_GetModState() & (SDL_KMOD_LCTRL | SDL_KMOD_RCTRL);

    switch(event->type)
    {
        case SDL_EVENT_KEY_UP: /* KEYUP & KEYDOWN share same routine */
        case SDL_EVENT_KEY_DOWN:
            {
                int down = event->type == SDL_EVENT_KEY_DOWN;
                switch(event->key.key)
                {
                    case SDLK_RSHIFT: /* RSHIFT & LSHIFT share same routine */
                    case SDLK_LSHIFT:    nk_input_key(&ctx->nk.ctx, NK_KEY_SHIFT, down); break;
                    case SDLK_DELETE:    nk_input_key(&ctx->nk.ctx, NK_KEY_DEL, down); break;

                    case SDLK_KP_ENTER:
                    case SDLK_RETURN:    nk_input_key(&ctx->nk.ctx, NK_KEY_ENTER, down); break;

                    case SDLK_TAB:       nk_input_key(&ctx->nk.ctx, NK_KEY_TAB, down); break;
                    case SDLK_BACKSPACE: nk_input_key(&ctx->nk.ctx, NK_KEY_BACKSPACE, down); break;
                    case SDLK_HOME:      nk_input_key(&ctx->nk.ctx, NK_KEY_TEXT_START, down);
                                         nk_input_key(&ctx->nk.ctx, NK_KEY_SCROLL_START, down); break;
                    case SDLK_END:       nk_input_key(&ctx->nk.ctx, NK_KEY_TEXT_END, down);
                                         nk_input_key(&ctx->nk.ctx, NK_KEY_SCROLL_END, down); break;
                    case SDLK_PAGEDOWN:  nk_input_key(&ctx->nk.ctx, NK_KEY_SCROLL_DOWN, down); break;
                    case SDLK_PAGEUP:    nk_input_key(&ctx->nk.ctx, NK_KEY_SCROLL_UP, down); break;
                    case SDLK_Z:         nk_input_key(&ctx->nk.ctx, NK_KEY_TEXT_UNDO, down && ctrl_down); break;
                    case SDLK_R:         nk_input_key(&ctx->nk.ctx, NK_KEY_TEXT_REDO, down && ctrl_down); break;
                    case SDLK_C:         nk_input_key(&ctx->nk.ctx, NK_KEY_COPY, down && ctrl_down); break;
                    case SDLK_V:         nk_input_key(&ctx->nk.ctx, NK_KEY_PASTE, down && ctrl_down); break;
                    case SDLK_X:         nk_input_key(&ctx->nk.ctx, NK_KEY_CUT, down && ctrl_down); break;
                    case SDLK_B:         nk_input_key(&ctx->nk.ctx, NK_KEY_TEXT_LINE_START, down && ctrl_down); break;
                    case SDLK_E:         nk_input_key(&ctx->nk.ctx, NK_KEY_TEXT_LINE_END, down && ctrl_down); break;
                    case SDLK_UP:        nk_input_key(&ctx->nk.ctx, NK_KEY_UP, down); break;
                    case SDLK_DOWN:      nk_input_key(&ctx->nk.ctx, NK_KEY_DOWN, down); break;
                    case SDLK_A:
                        if (ctrl_down)
                            nk_input_key(&ctx->nk.ctx,NK_KEY_TEXT_SELECT_ALL, down);
                        break;
                    case SDLK_LEFT:
                        if (ctrl_down)
                            nk_input_key(&ctx->nk.ctx, NK_KEY_TEXT_WORD_LEFT, down);
                        else nk_input_key(&ctx->nk.ctx, NK_KEY_LEFT, down);
                        break;
                    case SDLK_RIGHT:
                        if (ctrl_down)
                            nk_input_key(&ctx->nk.ctx, NK_KEY_TEXT_WORD_RIGHT, down);
                        else nk_input_key(&ctx->nk.ctx, NK_KEY_RIGHT, down);
                        break;
                }
            }
            return 1;

        case SDL_EVENT_MOUSE_BUTTON_UP: /* MOUSEBUTTONUP & MOUSEBUTTONDOWN share same routine */
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            {
                int down = event->type == SDL_EVENT_MOUSE_BUTTON_DOWN;
                const int x = event->button.x, y = event->button.y;
                switch(event->button.button)
                {
                    case SDL_BUTTON_LEFT:
                        if (event->button.clicks > 1)
                            nk_input_button(&ctx->nk.ctx, NK_BUTTON_DOUBLE, x, y, down);
                        nk_input_button(&ctx->nk.ctx, NK_BUTTON_LEFT, x, y, down); break;
                    case SDL_BUTTON_MIDDLE: nk_input_button(&ctx->nk.ctx, NK_BUTTON_MIDDLE, x, y, down); break;
                    case SDL_BUTTON_RIGHT:  nk_input_button(&ctx->nk.ctx, NK_BUTTON_RIGHT, x, y, down); break;
                }
            }
            return 1;

        case SDL_EVENT_MOUSE_MOTION:
            if (&ctx->nk.ctx.input.mouse.grabbed) {
                int x = (int)ctx->nk.ctx.input.mouse.prev.x, y = (int)ctx->nk.ctx.input.mouse.prev.y;
                nk_input_motion(&ctx->nk.ctx, x + event->motion.xrel, y + event->motion.yrel);
            }
            else nk_input_motion(&ctx->nk.ctx, event->motion.x, event->motion.y);
            return 1;

        case SDL_EVENT_TEXT_INPUT:
            {
                nk_glyph glyph;
                memcpy(glyph, event->text.text, NK_UTF_SIZE);
                nk_input_glyph(&ctx->nk.ctx, glyph);
            }
            return 1;

        case SDL_EVENT_MOUSE_WHEEL:
            nk_input_scroll(&ctx->nk.ctx,nk_vec2(event->wheel.x, event->wheel.y));
            return 1;
    }
    return 0;
}

