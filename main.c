#define FULLSCREEN 1

#define GAME_WIDTH 960
#define GAME_HEIGHT 540

#define GAMEPAD_THRESHOLD 0.5f

#define PLAYER_ACC 0.015f
#define PLAYER_FRIC 0.005f
#define PLAYER_MAX_VEL 0.25f
#define PLAYER_JUMP 1.0f
#define PLAYER_JUMP_REMAINDER 0.2f

#define BOAR_ACC 0.01f
#define BOAR_FRIC 0.005f
#define BOAR_MAX_VEL 0.15f

#define TILE_SIZE 16
#define GRAVITY 0.005f

#define MAX_SPRITES 256

#include "main.h"

#include "util.c"

// 1/60/8
// So basically, if we are running at perfect 60 fps, then the physics will update 8 times per second.
#define dt 0.00208333333333333333

static Sprite player_idle;
static Sprite player_run;
static Sprite player_jump_start;
static Sprite player_jump_end;
static Sprite player_attack;
static Sprite player_die;

static Sprite boar_idle;
static Sprite boar_walk;
static Sprite boar_run;
static Sprite boar_attack;
static Sprite boar_hit;

static Sprite spr_tiles;

#include "level.c"
#include "sprite.c"
#include "entity.c"

void InitSprites(void) {
	player_idle = GetSprite("assets\\legacy_fantasy_high_forest\\Character\\Idle\\Idle.aseprite");
	player_run = GetSprite("assets\\legacy_fantasy_high_forest\\Character\\Run\\Run.aseprite");
	player_jump_start = GetSprite("assets\\legacy_fantasy_high_forest\\Character\\Jump-Start\\Jump-Start.aseprite");
	player_jump_end = GetSprite("assets\\legacy_fantasy_high_forest\\Character\\Jump-End\\Jump-End.aseprite");
	player_attack = GetSprite("assets\\legacy_fantasy_high_forest\\Character\\Attack-01\\Attack-01.aseprite");
	player_die = GetSprite("assets\\legacy_fantasy_high_forest\\Character\\Dead\\Dead.aseprite");

	boar_idle = GetSprite("assets\\legacy_fantasy_high_forest\\Mob\\Boar\\Idle\\Idle.aseprite");
	boar_walk = GetSprite("assets\\legacy_fantasy_high_forest\\Mob\\Boar\\Walk\\Walk-Base.aseprite");
	boar_run = GetSprite("assets\\legacy_fantasy_high_forest\\Mob\\Boar\\Run\\Run.aseprite");
	boar_attack = GetSprite("assets\\legacy_fantasy_high_forest\\Mob\\Boar\\Walk\\Walk-Base.aseprite");
	boar_hit = GetSprite("assets\\legacy_fantasy_high_forest\\Mob\\Boar\\Hit-Vanish\\Hit.aseprite");

	spr_tiles = GetSprite("assets\\legacy_fantasy_high_forest\\Assets\\Tiles.aseprite");
}

void ResetGame(Context* ctx) {
	ctx->level_idx = 0;
	for (size_t level_idx = 0; level_idx < ctx->n_levels; ++level_idx) {
		Level* level = &ctx->levels[level_idx];
		{
			Entity* player = &level->player;
			player->state = EntityState_Free;
			player->pos = player->start_pos;
			player->vel = (vec2s){0.0f};
			player->dir = 1;
			ResetAnim(&player->anim);
			SetSpriteFromPath(player, "assets\\legacy_fantasy_high_forest\\Character\\Idle\\Idle.aseprite");
		}
		for (size_t enemy_idx = 0; enemy_idx < level->n_enemies; ++enemy_idx) {
			Entity* enemy = &level->enemies[enemy_idx];
			enemy->state = EntityState_Free;
			enemy->pos = enemy->start_pos;
			enemy->vel = (vec2s){0.0f};
			enemy->dir = 1;
			ResetAnim(&enemy->anim);
			if (enemy->type == EntityType_Boar) {
				SetSpriteFromPath(enemy, "assets\\legacy_fantasy_high_forest\\Mob\\Boar\\Idle\\Idle.aseprite");
			}
		}
	}
}

int32_t main(int32_t argc, char* argv[]) {
	UNUSED(argc);
	UNUSED(argv);

	InitSprites();

	// InitContext
	SDL_CHECK(SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO));
	Context* ctx = SDL_calloc(1, sizeof(Context)); SDL_CHECK(ctx);
	SDL_CHECK(SDL_GetCurrentTime(&ctx->time));

	// CreateWindowAndRenderer
	{
		SDL_DisplayID display = SDL_GetPrimaryDisplay();
		const SDL_DisplayMode* display_mode = SDL_GetDesktopDisplayMode(display); SDL_CHECK(display_mode);
	#if !FULLSCREEN
		SDL_CHECK(SDL_CreateWindowAndRenderer("LegacyFantasy", display_mode->w/2, display_mode->h/2, SDL_WINDOW_HIGH_PIXEL_DENSITY, &ctx->window, &ctx->renderer));
	#else
		SDL_CHECK(SDL_CreateWindowAndRenderer("LegacyFantasy", display_mode->w, display_mode->h, SDL_WINDOW_HIGH_PIXEL_DENSITY|SDL_WINDOW_FULLSCREEN, &ctx->window, &ctx->renderer));
	#endif

		ctx->vsync = SDL_SetRenderVSync(ctx->renderer, SDL_RENDERER_VSYNC_ADAPTIVE);
		if (!ctx->vsync) {
			ctx->vsync = SDL_SetRenderVSync(ctx->renderer, 1); // fixed refresh rate
		}

		ctx->refresh_rate = display_mode->refresh_rate;

		SDL_CHECK(SDL_SetDefaultTextureScaleMode(ctx->renderer, SDL_SCALEMODE_PIXELART));
		SDL_CHECK(SDL_SetRenderScale(ctx->renderer, (float)(display_mode->w/GAME_WIDTH), (float)(display_mode->h/GAME_HEIGHT)));
		SDL_CHECK(SDL_SetRenderDrawBlendMode(ctx->renderer, SDL_BLENDMODE_BLEND));
	}

	// LoadSprites
	SDL_CHECK(SDL_EnumerateDirectory("assets\\legacy_fantasy_high_forest", EnumerateSpriteDirectory, ctx));

	// SortSprites
	for (size_t sprite_idx = 0; sprite_idx < MAX_SPRITES; ++sprite_idx) {
		SpriteDesc* sd = &ctx->sprites[sprite_idx];
		if (sd->path) {
			for (size_t frame_idx = 0; frame_idx < sd->n_frames; ++frame_idx) {
				SpriteFrame* sf = &sd->frames[frame_idx];
				SDL_qsort(sf->cells, sf->n_cells, sizeof(SpriteCell), (SDL_CompareCallback)CompareSpriteCells);
			}
		}
	}
	
	LoadLevels(ctx);

	// InitReplayFrames
	{
		ctx->c_replay_frames = 1024;
		ctx->replay_frames = SDL_malloc(ctx->c_replay_frames * sizeof(ReplayFrame)); SDL_CHECK(ctx->replay_frames);
	}

	ResetGame(ctx);

	ctx->running = true;
	while (ctx->running) {
		while (ctx->dt_accumulator > dt) {
			GetInput(ctx); // TODO: Should this be inside or outside the loop?
			if (!ctx->paused) {
				UpdateGame(ctx);
				RecordReplayFrame(ctx);
			}

	 		ctx->dt_accumulator -= dt;
		}

		// RenderBegin
		{
			SDL_CHECK(SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, 0));
			SDL_CHECK(SDL_RenderClear(ctx->renderer));
		}

		// RenderLevel
		{
			size_t n_enemies; Entity* enemies = GetEnemies(ctx, &n_enemies);
			for (size_t enemy_idx = 0; enemy_idx < n_enemies; ++enemy_idx) {
				Entity* enemy = &enemies[enemy_idx];
				DrawEntity(ctx, enemy);
			}
			DrawEntity(ctx, GetPlayer(ctx));
			size_t n_tiles; Tile* tiles = GetTiles(ctx, &n_tiles);
			Level* level = GetCurrentLevel(ctx);
			for (size_t tile_idx = 0; tile_idx < n_tiles; ++tile_idx) {
				Tile tile = tiles[tile_idx];
				if (tile.type != TileType_Empty) {
					ivec2s pos = {(int32_t)tile_idx % level->size.x, (int32_t)tile_idx / level->size.x};
					pos = glms_ivec2_scale(pos, TILE_SIZE);
					DrawSpriteTile(ctx, spr_tiles, tile, pos);
				}
			}
		}

		// RenderHitbox
		{
			Entity* player = GetPlayer(ctx);
			Rect hitbox = GetEntityHitbox(ctx, player);
			SDL_CHECK(SDL_SetRenderDrawColor(ctx->renderer, 255, 0, 0, 128));
			SDL_FRect rect = {(float)hitbox.min.x, (float)hitbox.min.y, (float)(hitbox.max.x-hitbox.min.x), (float)(hitbox.max.y-hitbox.min.y)};
			SDL_CHECK(SDL_RenderFillRect(ctx->renderer, &rect));
		}

		// RenderEnd
		{
			SDL_RenderPresent(ctx->renderer);

			if (!ctx->vsync) {
				SDL_Delay(16); // TODO
			}
		}

		// UpdateTime
		{			
			SDL_Time current_time;
			SDL_CHECK(SDL_GetCurrentTime(&current_time));
			SDL_Time dt_int = current_time - ctx->time;
			const double NANOSECONDS_IN_SECOND = 1000000000.0;
			double dt_double = (double)dt_int / NANOSECONDS_IN_SECOND;
			ctx->dt_accumulator = SDL_min(ctx->dt_accumulator + dt_double, 1.0f/30.0f);
			ctx->time = current_time;
		}	
	}
	
	SDL_Quit();
	return 0;
}

SDL_EnumerationResult EnumerateSpriteDirectory(void *userdata, const char *dirname, const char *fname) {
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
		SDL_CHECK(SDL_EnumerateDirectory(dir_path, EnumerateSpriteDirectory, ctx));
	}
	
	SDL_free(files);
	return SDL_ENUM_CONTINUE;
}

void GetInput(Context* ctx) {
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
				case SDLK_SPACE:
					ctx->paused = !ctx->paused;
					break;
				case SDLK_0:
					break;
				case SDLK_X:
					ctx->button_attack = true;
					break;
				case SDLK_LEFT:
					if (!event.key.repeat) {
						ctx->button_left = 1;
					}
					if (ctx->paused) {
						SetReplayFrame(ctx, SDL_max(ctx->replay_frame_idx - 1, 0));
					}
					break;
				case SDLK_RIGHT:
					if (!event.key.repeat) {
						ctx->button_right = 1;
					}
					if (ctx->paused) {
						SetReplayFrame(ctx, SDL_min(ctx->replay_frame_idx + 1, ctx->replay_frame_idx_max - 1));
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
				ctx->gamepad_left_stick.x = NormInt16(event.gaxis.value);
				break;
			case SDL_GAMEPAD_AXIS_LEFTY:
				ctx->gamepad_left_stick.y = NormInt16(event.gaxis.value);
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

	if (SDL_fabs(ctx->gamepad_left_stick.x) > GAMEPAD_THRESHOLD) {
		ctx->gamepad_left_stick.x = 0.0f;
	}
}

void UpdateGame(Context* ctx) {
	UpdatePlayer(ctx);
	size_t n_enemies; Entity* enemies = GetEnemies(ctx, &n_enemies);
	for (size_t enemy_idx = 0; enemy_idx < n_enemies; ++enemy_idx) {
		Entity* enemy = &enemies[enemy_idx];
		if (enemy->type == EntityType_Boar) {
			UpdateBoar(ctx, enemy);
		}
	}
}

ReplayFrame* GetReplayFrame(Context* ctx) {
	return &ctx->replay_frames[ctx->replay_frame_idx];
}

void SetReplayFrame(Context* ctx, size_t replay_frame_idx) {
	ctx->replay_frame_idx = replay_frame_idx;
	Level* level = GetCurrentLevel(ctx);
	ReplayFrame* replay_frame = GetReplayFrame(ctx);
	level->player = replay_frame->player;
	SDL_memcpy(level->enemies, replay_frame->enemies, replay_frame->n_enemies * sizeof(Entity));
	level->n_enemies = replay_frame->n_enemies;

}

void RecordReplayFrame(Context* ctx) {
	ReplayFrame replay_frame = {0};
	
	Level* level = GetCurrentLevel(ctx);
	replay_frame.player = level->player;
	replay_frame.enemies = SDL_malloc(level->n_enemies * sizeof(Entity)); SDL_CHECK(replay_frame.enemies);
	SDL_memcpy(replay_frame.enemies, level->enemies, level->n_enemies * sizeof(Entity));
	replay_frame.n_enemies = level->n_enemies;
		
	ctx->replay_frames[ctx->replay_frame_idx++] = replay_frame;
	if (ctx->replay_frame_idx >= ctx->c_replay_frames - 1) {
		ctx->c_replay_frames *= 8;
		ctx->replay_frames = SDL_realloc(ctx->replay_frames, ctx->c_replay_frames * sizeof(ReplayFrame)); SDL_CHECK(ctx->replay_frames);
	}
	ctx->replay_frame_idx_max = SDL_max(ctx->replay_frame_idx_max, ctx->replay_frame_idx);
}

ivec2s GetTileSpritePos(Context* ctx, Sprite tileset, Tile tile) {
	ivec2s dim = GetTilesetDimensions(ctx, tileset);
	return (ivec2s){tile.src_idx % dim.x, tile.src_idx / dim.x};
}