#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_image.h>

#include <cglm/cglm.h>

#include "main.h"

static float GRAVITY;

static float PLAYER_ACC;
static float PLAYER_FRIC;
static float PLAYER_MAX_VEL;
static float PLAYER_JUMP;
static float PLAYER_BOUNCE;

#define PLAYER_JUMP_PERIOD 3

static float TILE_SIZE;

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
	vec2 pos;
	vec2 vel;
	int can_jump;
} Entity;

typedef struct Context {
	SDL_Window* window;
	SDL_Renderer* renderer;
	bool vsync;
	Entity player;
	SDL_Gamepad* gamepad;
	SDL_DisplayMode* display_mode;
	vec2 axis;
	SDL_Time time;
	float dt;
} Context;

void reset_game(Context* ctx) {
	ctx->dt = ctx->display_mode->refresh_rate;
	bool ok = SDL_GetCurrentTime(&ctx->time); SDL_assert(ok && SDL_GetError());
	ctx->player = (Entity){.pos[0] = TILE_SIZE*1.0f, .pos[1] = TILE_SIZE*6.0f};
}

#define SDL_CHECK(E) STMT(if (!E) { SDL_Log("SDL: %s.", SDL_GetError()); res = SDL_APP_FAILURE; })

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
	SDL_AppResult res = SDL_APP_CONTINUE;

	SDL_CHECK(SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD));

	Context* ctx = new(1, Context); SDL_CHECK(ctx);
	memset(ctx, 0, sizeof(Context));
	*appstate = ctx;

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

		TILE_SIZE = 32.0f;

		GRAVITY = 24.0f;

		PLAYER_ACC = 30.0f;
		PLAYER_FRIC = 12.0f;
		PLAYER_MAX_VEL = 180.0f;
		PLAYER_JUMP = 720.0f;
		PLAYER_BOUNCE = 0.1f;
	}

	reset_game(ctx);

	return res; 
}

/*
TODO:
- Delta time.
- Buy Legacy-Fantasy asset and render something in it.

TODO (distant future):
- Include dependencies as Git submodules. Modify cmake build system to build those dependencies too.
*/

SDL_AppResult SDL_AppIterate(void* appstate) {
	SDL_AppResult res = SDL_APP_CONTINUE;
	Context* ctx = appstate;

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

	if (ctx->player.vel[1] > 81.0f) {
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
	
	if (ctx->player.can_jump) {
		float acc = ctx->axis[0] * PLAYER_ACC;
		ctx->player.vel[0] += acc*ctx->dt;
		if (ctx->player.vel[0] < 0.0f) ctx->player.vel[0] = min(0.0f, ctx->player.vel[0] + PLAYER_FRIC);
		else if (ctx->player.vel[0] > 0.0f) ctx->player.vel[0] = max(0.0f, ctx->player.vel[0] - PLAYER_FRIC);
		ctx->player.vel[0] = clamp(ctx->player.vel[0], -PLAYER_MAX_VEL, PLAYER_MAX_VEL);
	}
	
	ctx->player.vel[1] += GRAVITY*ctx->dt;

	// player_collision
	{
		if (ctx->player.vel[0] < 0.0f) {
			Rect side;
			side.pos[0] = ctx->player.pos[0] + ctx->player.vel[0];
			side.pos[1] = ctx->player.pos[1] + 1.0f;
			side.area[0] = 1.0f;
			side.area[1] = TILE_SIZE - 2.0f;
			bool break_loop = false;
			for (ssize_t tile_y = 0; tile_y < LEVEL_HEIGHT && !break_loop; tile_y += 1) {
				for (ssize_t tile_x = 0; tile_x < LEVEL_WIDTH && !break_loop; tile_x += 1) {
					if (level[tile_y][tile_x]) {
						Rect tile = rect_from_tile((Tile){(int)tile_x, (int)tile_y});
						if (rects_intersect(side, tile)) {
							ctx->player.pos[0] = tile.pos[0] + TILE_SIZE;
							ctx->player.vel[0] = -ctx->player.vel[1] * PLAYER_BOUNCE;
							break_loop = true;
						}
					}
				}
			}
		} else if (ctx->player.vel[0] > 0.0f) {
			Rect side;
			side.pos[0] = (ctx->player.pos[0] + TILE_SIZE - 1.0f) + ctx->player.vel[0];
			side.pos[1] = ctx->player.pos[1] + 1.0f;
			side.area[0] = 1.0f;
			side.area[1] = TILE_SIZE - 2.0f;
			bool break_loop = false;
			for (ssize_t tile_y = 0; tile_y < LEVEL_HEIGHT && !break_loop; tile_y += 1) {
				for (ssize_t tile_x = 0; tile_x < LEVEL_WIDTH && !break_loop; tile_x += 1) {
					if (level[tile_y][tile_x]) {
						Rect tile = rect_from_tile((Tile){(int)tile_x, (int)tile_y});
						if (rects_intersect(side, tile)) {
							ctx->player.pos[0] = tile.pos[0] - TILE_SIZE;
							ctx->player.vel[0] = -ctx->player.vel[1] * PLAYER_BOUNCE;
							break_loop = true;
						}
					}
				}
			}
		}

		ctx->player.can_jump = max(0, ctx->player.can_jump - 1);
		if (ctx->player.vel[1] < 0.0f) {
			Rect side;
			side.pos[0] = ctx->player.pos[0] + 1.0f; 
			side.pos[1] = ctx->player.pos[1] + ctx->player.vel[1];
			side.area[0] = TILE_SIZE - 2.0f;
			side.area[1] = 1.0f;
			bool break_loop = false;
			for (ssize_t tile_y = 0; tile_y < LEVEL_HEIGHT && !break_loop; tile_y += 1) {
				for (ssize_t tile_x = 0; tile_x < LEVEL_WIDTH && !break_loop; tile_x += 1) {
					if (level[tile_y][tile_x]) {
						Rect tile = rect_from_tile((Tile){(int)tile_x, (int)tile_y});
						if (rects_intersect(side, tile)) {
							ctx->player.pos[1] = tile.pos[1] + TILE_SIZE;
							ctx->player.vel[1] = -ctx->player.vel[1] * PLAYER_BOUNCE;
							break_loop = true;
						}
					}
				}
			}
		} else if (ctx->player.vel[1] > 0.0f) {
			Rect side;
			side.pos[0] = ctx->player.pos[0] + 1.0f; 
			side.pos[1] = (ctx->player.pos[1] + TILE_SIZE - 1.0f) + ctx->player.vel[1];
			side.area[0] = TILE_SIZE - 2.0f;
			side.area[1] = 1.0f;
			bool break_loop = false;
			for (ssize_t tile_y = 0; tile_y < LEVEL_HEIGHT && !break_loop; tile_y += 1) {
				for (ssize_t tile_x = 0; tile_x < LEVEL_WIDTH && !break_loop; tile_x += 1) {
					if (level[tile_y][tile_x]) {
						Rect tile = rect_from_tile((Tile){(int)tile_x, (int)tile_y});
						if (rects_intersect(side, tile)) {
							ctx->player.pos[1] = tile.pos[1] - TILE_SIZE;
							ctx->player.vel[1] = -ctx->player.vel[1] * PLAYER_BOUNCE;
							ctx->player.can_jump = PLAYER_JUMP_PERIOD;
							break_loop = true;
						}
					}
				}
			}
		}

		vec2 vel_dt;
		glm_vec2_scale(ctx->player.vel, ctx->dt, vel_dt);
		glm_vec2_add(ctx->player.pos, vel_dt, ctx->player.pos);

		Rect player_rect = rect(ctx->player.pos, (vec2){TILE_SIZE, TILE_SIZE});
		if (!rect_is_valid(player_rect)) reset_game(ctx);
	}

	// render_begin
	{
		SDL_CHECK(SDL_RenderClear(ctx->renderer));
	}

	// render_player
	{
		SDL_CHECK(SDL_SetRenderDrawColor(ctx->renderer, 0, 255, 0, 0));
		SDL_FRect rect = { ctx->player.pos[0], ctx->player.pos[1], TILE_SIZE, TILE_SIZE };
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
		SDL_CHECK(SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, 0));
		SDL_CHECK(SDL_RenderPresent(ctx->renderer));

		if (!ctx->vsync) {
			// TODO
		}
	}

	return res; 
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
	SDL_AppResult res = SDL_APP_CONTINUE;
	Context* ctx = appstate;

	switch (event->type) {
	case SDL_EVENT_KEY_DOWN:
		switch (event->key.key) {
		case SDLK_ESCAPE:
			res = SDL_APP_SUCCESS;
			break;
		case SDLK_LEFT:
			if (!ctx->gamepad) {
				ctx->axis[0] = max(ctx->axis[0] - 1.0f, -1.0f);
			}
			break;
		case SDLK_RIGHT:
			if (!ctx->gamepad) {
				ctx->axis[0] = min(ctx->axis[0] + 1.0f, +1.0f);
			}
			break;
		case SDLK_UP:
			if (!ctx->gamepad) {
				if (!event->key.repeat && ctx->player.can_jump) {
					ctx->player.can_jump = 0;
					ctx->player.vel[1] = -PLAYER_JUMP;
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
				ctx->axis[0] = min(ctx->axis[0] + 1.0f, +1.0f);
			}	
			break;
		case SDLK_RIGHT:
			if (!ctx->gamepad) {
				ctx->axis[0] = max(ctx->axis[0] - 1.0f, -1.0f);
			}
			break;
		case SDLK_UP:
			if (!ctx->gamepad) {
				ctx->player.vel[1] = max(ctx->player.vel[1], ctx->player.vel[1]*GRAVITY);
			}
			break;
		case SDLK_DOWN:

			break;
		}
		break;
	case SDL_EVENT_GAMEPAD_AXIS_MOTION:
		if (event->gaxis.axis == SDL_GAMEPAD_AXIS_LEFTX) {
			float value = (float)event->gaxis.value / 32767.0f;
			ctx->axis[0] = value;
			// TODO: What do we do about the y axis?
		}

		break;
	case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
		if (event->gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH) {
			if (ctx->player.can_jump) {
				ctx->player.can_jump = 0;
				ctx->player.vel[1] = -PLAYER_JUMP;
			}
		}
		break;
	case SDL_EVENT_GAMEPAD_BUTTON_UP:
		if (event->gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH) {
			ctx->player.vel[1] = max(ctx->player.vel[1], ctx->player.vel[1]/2.0f);
		}
		break;
	case SDL_EVENT_QUIT:
		res = SDL_APP_SUCCESS;
		break;
	}

	return res; 
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
	SDL_Quit();
}