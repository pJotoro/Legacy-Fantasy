#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_image.h>

#include <cglm/cglm.h>

#include "main.h"

FORCEINLINE void glm_vec2_from_ivec2(ivec2 iv, vec2 v) {
	v[0] = (float)iv[0];
	v[1] = (float)iv[1];
}

FORCEINLINE void glm_ivec2_from_vec2(vec2 v, ivec2 iv) {
	iv[0] = (int)v[0];
	iv[1] = (int)v[1];
}

FORCEINLINE void glm_ivec2_from_vec2_floor(vec2 v, ivec2 iv) {
	vec2 vf;
	glm_vec2_floor(v, vf);
	iv[0] = (int)vf[0];
	iv[1] = (int)vf[1];
}

FORCEINLINE void glm_ivec2_from_vec2_ceil(vec2 v, ivec2 iv) {
	vec2 vc;
	glm_vec2_ceil(v, vc);
	iv[0] = (int)vc[0];
	iv[1] = (int)vc[1];
}

FORCEINLINE void glm_ivec2_from_vec2_round(vec2 v, ivec2 iv) {
	vec2 vr;
	glm_vec2_round(v, vr);
	iv[0] = (int)vr[0];
	iv[1] = (int)vr[1];
}

#define GRAVITY 0.4f

#define PLAYER_ACC 0.3f
#define PLAYER_FRIC 0.1f
#define PLAYER_MAX_VEL 3.5f
#define PLAYER_JUMP 8.0f

#define TILE_SIZE 64.0f

typedef enum EntityState {
	ENTITY_STATE_STAND,
	ENTITY_STATE_WALK,
	ENTITY_STATE_JUMP,
	ENTITY_STATE_FALL,
} EntityState;

typedef struct Entity {
	vec2 pos;
	vec2 vel;
	EntityState state;
} Entity;

typedef struct Context {
	SDL_Window* window;
	SDL_Renderer* renderer;
	Entity player;
	SDL_Gamepad* gamepad;
	vec2 axis;
} Context;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
	SDL_AppResult res = SDL_APP_CONTINUE;

	SDL_CHECK(SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD));

	Context* ctx = new(1, Context); SDL_CHECK(ctx);
	memset(ctx, 0, sizeof(Context));
	*appstate = ctx;

	// create_window
	{
		SDL_DisplayID display = SDL_GetPrimaryDisplay();
		const SDL_DisplayMode* display_mode = SDL_GetDesktopDisplayMode(display); SDL_CHECK(display_mode);
		SDL_WindowFlags flags = SDL_WINDOW_HIGH_PIXEL_DENSITY;
		int w = display_mode->w / 2;
		int h = display_mode->h / 2;
#ifndef _DEBUG
		flags |= SDL_WINDOW_FULLSCREEN;
		w = display_mode->w;
		h = display_mode->h;
#endif
		ctx->window = SDL_CreateWindow("Platformer", w, h, flags); SDL_CHECK(ctx->window);
	}

	ctx->renderer = SDL_CreateRenderer(ctx->window, NULL); SDL_CHECK(ctx->renderer);

	return res; 
}

#define LEVEL_WIDTH 10
#define LEVEL_HEIGHT 6
static bool level[LEVEL_HEIGHT][LEVEL_WIDTH] = {
	{false, false, false, false, false, false, false, false, false, false},
	{false, false, true,  true,  true,  false, false, true,  true, false},
	{true, false,  false, true,  false, false, false, false, true, false},
	{true, false,  false, true,  false, false, false, false, true, false},
	{true,  true,  true,  true,  true,  false, true,  true,  true, false},
	{false, false, false, false, false, false, false, false, false, false},
};

typedef struct Rect {
	vec2 pos;
	vec2 area;
} Rect;

FORCEINLINE Rect rect(vec2 pos, vec2 area) {
	Rect rect;
	glm_vec2_copy(pos, rect.pos);
	glm_vec2_copy(area, rect.area);
	return rect;
}

typedef ivec2 Tile;

FORCEINLINE Rect rect_from_tile(Tile tile) {
	vec2 pos;
	glm_vec2_from_ivec2(tile, pos);
	glm_vec2_scale(pos, TILE_SIZE, pos);
	vec2 size;
	glm_vec2_fill(size, TILE_SIZE);
	return rect(pos, size);
}

FORCEINLINE void tile_from_rect(Rect rect, Tile tile) {
	glm_ivec2_from_vec2_floor(rect.pos, tile);
}

bool tile_is_valid(Tile tile) {
	return tile[0] >= 0 && tile[0] < LEVEL_WIDTH && tile[1] >= 0 && tile[1] < LEVEL_HEIGHT;
}

bool rect_is_valid(Rect rect) {
	return rect.pos[0] >= 0.0f && rect.pos[0]+rect.area[0] < LEVEL_WIDTH*TILE_SIZE && rect.pos[1] >= 0.0f && rect.pos[1]+rect.area[1] < LEVEL_HEIGHT*TILE_SIZE;
}

bool rects_intersect(Rect a, Rect b) {
	if (!rect_is_valid(a) || !rect_is_valid(b)) return false;
	return ((a.pos[0] < (b.pos[0] + b.area[0]) && (a.pos[0] + a.area[0]) > b.pos[0]) && (a.pos[1] < (b.pos[1] + b.area[1]) && (a.pos[1] + a.area[1]) > b.pos[1]));
}

SDL_AppResult SDL_AppIterate(void* appstate) {
	SDL_AppResult res = SDL_APP_CONTINUE;
	Context* ctx = appstate;

	// TODO: Get VSync to work and get delta time.
	SDL_Delay(16);

	if (!ctx->gamepad) {
		int joystick_count = 0;
		SDL_JoystickID* joysticks = SDL_GetGamepads(&joystick_count);
		if (joystick_count != 0) {
			ctx->gamepad = SDL_OpenGamepad(joysticks[0]);
		}
	}
	
	{
		float acc = ctx->axis[0] * PLAYER_ACC;
		ctx->player.vel[0] += acc;
		ctx->player.vel[0] = clamp(ctx->player.vel[0], -PLAYER_MAX_VEL, PLAYER_MAX_VEL);
	}

	if (ctx->player.vel[0] < 0.0f) ctx->player.vel[0] = min(0.0f, ctx->player.vel[0] + PLAYER_FRIC);
	else if (ctx->player.vel[0] > 0.0f) ctx->player.vel[0] = max(0.0f, ctx->player.vel[0] - PLAYER_FRIC);
	
	ctx->player.vel[1] += GRAVITY;

	// player_collision
	{
		glm_vec2_add(ctx->player.pos, ctx->player.vel, ctx->player.pos);
		Rect player_rect = rect(ctx->player.pos, (vec2){TILE_SIZE, TILE_SIZE});
		bool break_loop = false;
		for (ssize_t tile_y = 0; tile_y < LEVEL_HEIGHT && !break_loop; tile_y += 1) {
			for (ssize_t tile_x = 0; tile_x < LEVEL_WIDTH && !break_loop; tile_x += 1) {
				if (level[tile_y][tile_x]) {
					Tile tile = {(int)tile_x, (int)tile_y};
					Rect tile_rect = rect_from_tile(tile);
					if (rects_intersect(player_rect, tile_rect)) {
						vec2 nvel;
						glm_vec2_normalize_to(ctx->player.vel, nvel);
						do {
							glm_vec2_sub(player_rect.pos, nvel, player_rect.pos);
						} while (rects_intersect(player_rect, tile_rect));
						glm_vec2_copy(player_rect.pos, ctx->player.pos);
						glm_vec2_zero(ctx->player.vel);
						ctx->player.can_jump = true;
						break_loop = true;
					}
				}
			}
		}
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
					ctx->player.can_jump = false;
					ctx->player.vel[1] = -PLAYER_JUMP;
				}
			}
			break;
		case SDLK_DOWN:
			break;
		case SDLK_R:
			ctx->player = (Entity){.pos = {TILE_SIZE}};
			ctx->axis[0] = 0.0f;
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
				ctx->player.can_jump = false;
				ctx->player.vel[1] = -PLAYER_JUMP;
			}
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