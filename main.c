#define FULLSCREEN 1

#define GAME_WIDTH 960
#define GAME_HEIGHT 540

#define PLAYER_ACC 100.0f
#define PLAYER_FRIC 20.0f
#define PLAYER_MAX_VEL 500.0f
#define PLAYER_JUMP 10000.0f
#define PLAYER_JUMP_REMAINDER 10

#define BOAR_ACC 0.2f
#define BOAR_FRIC 0.1f
#define BOAR_MAX_VEL 1.6f

#define TILE_SIZE 16
#define GRAVITY 300.6f

#define MAX_SPRITES 256

#include "main.h"

#include "util.c"
#include "level.c"
#include "sprite.c"

// 1/60/8
// So basically, if we are running at perfect 60 fps, then the physics will update 8 times per second.
#define dt 0.00208333333333333333f
#define dt_double 0.00208333333333333333

// I know global variables are bad, but sometimes they are just so convenient.

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
}

void ResetGame(Context* ctx) {
	ctx->level_idx = 0;
	for (size_t level_idx = 0; level_idx < ctx->n_levels; ++level_idx) {
		Level* level = &ctx->levels[level_idx];
		{
			Entity* player = &level->player;
			player->flags |= EntityFlags_Active;
			player->pos = player->start_pos;
			player->vel = (vec2s){0.0f, 0.0f};
			player->dir = 1;
			player->touching_floor = 0;
			player->health = 5;
			ResetAnim(&player->anim);
			SetSpriteFromPath(player, "assets\\legacy_fantasy_high_forest\\Character\\Idle\\Idle.aseprite");
		}
		for (size_t enemy_idx = 0; enemy_idx < level->n_enemies; ++enemy_idx) {
			Entity* enemy = &level->enemies[enemy_idx];
			enemy->flags |= EntityFlags_Active;
			enemy->pos = enemy->start_pos;
			enemy->vel = (vec2s){0.0f, 0.0f};
			enemy->dir = 1;
			enemy->touching_floor = false;
			enemy->health = 1;
			ResetAnim(&enemy->anim);
			if (HAS_FLAG(enemy->flags, EntityFlags_Boar)) {
				SetSpriteFromPath(enemy, "assets\\legacy_fantasy_high_forest\\Mob\\Boar\\Idle\\Idle.aseprite");
			}
		}
	}
}

int32_t main(int32_t argc, char* argv[]) {
	UNUSED(argc);
	UNUSED(argv);

	SDL_CHECK(SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO));
#if 0
	SDL_CHECK(TTF_Init());
#endif

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
			for (size_t tile_idx = 0; tile_idx < level->n_tiles; ++tile_idx) {
				Entity* tile = &level->tiles[tile_idx];
				ivec2s src = tile->src;
				for (size_t tiles_collide_idx = 0; tiles_collide_idx < n_tiles_collide; ++tiles_collide_idx) {
					int32_t i = tiles_collide[tiles_collide_idx];
					int32_t j = (src.x + src.y*25)/TILE_SIZE; // TODO: Replace 25 with tileset width.
					if (i == j) {
						tile->flags |= EntityFlags_Solid;
						break;
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

#if 0
		ctx->display_content_scale = SDL_GetDisplayContentScale(display);
#endif
		ctx->refresh_rate = display_mode->refresh_rate;

		SDL_CHECK(SDL_SetDefaultTextureScaleMode(ctx->renderer, SDL_SCALEMODE_PIXELART));
		SDL_CHECK(SDL_SetRenderScale(ctx->renderer, (float)(display_mode->w/GAME_WIDTH), (float)(display_mode->h/GAME_HEIGHT)));

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

	ctx->c_replay_frames = 1024;
	ctx->replay_frames = SDL_malloc(ctx->c_replay_frames * sizeof(ReplayFrame)); SDL_CHECK(ctx->replay_frames);

	ctx->running = true;
	while (ctx->running) {
		while (ctx->dt_accumulator > dt_double) {
			UpdateGame(ctx);
			ctx->dt_accumulator -= dt_double;
		}

		// RenderBegin
		{
			SDL_CHECK(SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, 0));
			SDL_CHECK(SDL_RenderClear(ctx->renderer));
		}

		// RenderLevel
		{
			static Sprite spr_tiles;
			static bool initialized_sprites = false;
			if (!initialized_sprites) {
				initialized_sprites = true;
				spr_tiles = GetSprite("assets\\legacy_fantasy_high_forest\\Assets\\Tiles.aseprite");
			}

			size_t n_enemies; Entity* enemies = GetEnemies(ctx, &n_enemies);
			for (size_t enemy_idx = 0; enemy_idx < n_enemies; ++enemy_idx) {
				Entity* enemy = &enemies[enemy_idx];
				DrawEntity(ctx, enemy);
			}
			DrawEntity(ctx, GetPlayer(ctx));
			size_t n_tiles; Entity* tiles = GetTiles(ctx, &n_tiles);
			for (size_t tile_idx = 0; tile_idx < n_tiles; ++tile_idx) {
				Entity* tile = &tiles[tile_idx];
				DrawSpriteTile(ctx, spr_tiles, tile->src, tile->pos);
			}
		}

		// RenderEnd
		{
			SDL_RenderPresent(ctx->renderer);

			if (!ctx->vsync) {
				// TODO
			}
		}

		// ReplayFrame replay_frame = {0};
		// replay_frame.player = *GetPlayer(ctx);

		{			
			SDL_Time current_time;
			SDL_CHECK(SDL_GetCurrentTime(&current_time));
			SDL_Time dt_int = current_time - ctx->time;
			const double NANOSECONDS_IN_SECOND = 1000000000.0;
			double _dt_double = (double)dt_int / NANOSECONDS_IN_SECOND;
			ctx->dt_accumulator += _dt_double;
			ctx->time = current_time;
		}

		// if (!ctx->paused)
 		// {
	 	// 	ctx->replay_frames[ctx->n_replay_frames++] = replay_frame;
		// 	if (ctx->n_replay_frames >= ctx->c_replay_frames) {
		// 		ctx->c_replay_frames *= 8;
		// 		ctx->replay_frames = SDL_realloc(ctx->replay_frames, ctx->c_replay_frames * sizeof(ReplayFrame)); SDL_CHECK(ctx->replay_frames);
		// 	}
		// 	ctx->replay_frame_idx = ctx->n_replay_frames - 1;
 		// }		
	}

	for (size_t i = 0; i < ctx->n_replay_frames; ++i) {
		// ReplayFrame* r = &ctx->replay_frames[i];

	}
	
	SDL_Quit();
	return 0;
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

void UpdateGame(Context* ctx) {
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
						ctx->replay_frame_idx = SDL_max(ctx->replay_frame_idx - 1, 0);
					}
					break;
				case SDLK_RIGHT:
					if (!event.key.repeat) {
						ctx->button_right = 1;
					}
					if (ctx->paused) {
						ctx->replay_frame_idx = SDL_min(ctx->replay_frame_idx + 1, (ssize_t)ctx->n_replay_frames - 1);
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

	if (!ctx->vsync) {
		SDL_Delay(16); // TODO
	}

	if (!ctx->paused) {
		UpdatePlayer(ctx);
		size_t n_enemies; Entity* enemies = GetEnemies(ctx, &n_enemies);
		for (size_t enemy_idx = 0; enemy_idx < n_enemies; ++enemy_idx) {
			Entity* enemy = &enemies[enemy_idx];
			if (HAS_FLAG(enemy->flags, EntityFlags_Boar)) {
				UpdateBoar(ctx, enemy);
			}
		}
	} else {
		Entity* player = GetPlayer(ctx);
		*player = ctx->replay_frames[ctx->replay_frame_idx].player;
	}
}