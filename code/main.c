#define ENABLE_PROFILING 1
#define FULLSCREEN 1

#define STMT(X) do {X} while (false)

#pragma warning(push, 0)
#include <SDL.h>
#include <SDL_main.h>

typedef int64_t ssize_t;

#include <cglm/struct.h>

#define XXH_STATIC_LINKING_ONLY /* access advanced declarations */
#define XXH_IMPLEMENTATION      /* access definitions */
#include <xxhash.h>

#include <infl.h>
#include <cJson/cJson.h>

#ifdef _DEBUG
#include <raddbg_markup.h>
#endif

#if ENABLE_PROFILING
#include <spall/spall.h>
#define SPALL_BUFFER_BEGIN_NAME(NAME) STMT( \
	SDL_Time time; \
	SDL_GetCurrentTime(&time); \
	spall_buffer_begin( \
		&ctx->spall_ctx, \
		&ctx->spall_buffer, \
		NAME, \
		sizeof(NAME) - 1, \
		time); \
)
#define SPALL_BUFFER_BEGIN() STMT( \
	SDL_Time time; \
	SDL_GetCurrentTime(&time); \
	spall_buffer_begin( \
		&ctx->spall_ctx, \
		&ctx->spall_buffer, \
		__FUNCTION__, \
		sizeof(__FUNCTION__) - 1, \
		time); \
)
#define SPALL_BUFFER_END() STMT( \
	SDL_Time time; \
	SDL_GetCurrentTime(&time); \
	spall_buffer_end( \
		&ctx->spall_ctx, \
		&ctx->spall_buffer, \
		time); \
)
#else
#define SPALL_BUFFER_BEGIN()
#endif

#pragma warning(pop)

#include "aseprite.h"

#define FORCEINLINE SDL_FORCE_INLINE
#define function static

#define UNUSED(X) (void)X

#define HAS_FLAG(FLAGS, FLAG) ((FLAGS) & (FLAG)) // TODO: Figure out why I can't have multiple flags set in the second argument.
#define FLAG(X) (1u << X##u)

#define GetSprite(path) ((Sprite){HashString(path, 0) & (MAX_SPRITES - 1)})

#define SDL_CHECK(E) STMT( \
	if (!(E)) { \
		SDL_LogMessage(SDL_LOG_CATEGORY_ASSERT, SDL_LOG_PRIORITY_CRITICAL, "%s(%d): %s", __FILE__, __LINE__, SDL_GetError()); \
		SDL_TriggerBreakpoint(); \
	} \
)

#define SDL_ReadStruct(S, STRUCT) SDL_ReadIO(S, (STRUCT), sizeof(*(STRUCT)))

#define SDL_ReadIOChecked(S, P, SZ) SDL_CHECK(SDL_ReadIO(S, P, SZ) == SZ)
#define SDL_ReadStructChecked(S, STRUCT) SDL_CHECK(SDL_ReadStruct(S, STRUCT) == sizeof(*(STRUCT)))

#define FULLSCREEN 1

#define GAME_WIDTH 960
#define GAME_HEIGHT 540

#define GAMEPAD_THRESHOLD 0.5f

#define PLAYER_ACC 0.02f
#define PLAYER_FRIC 0.006f
#define PLAYER_MAX_VEL 0.3f
#define PLAYER_JUMP 1.3f
#define PLAYER_JUMP_REMAINDER 0.2f

#define BOAR_ACC 0.01f
#define BOAR_FRIC 0.005f
#define BOAR_MAX_VEL 0.15f

#define TILE_SIZE 16
#define GRAVITY 0.009f

#define MAX_SPRITES 256

#define MIN_FPS 15

// 1/60/8
// So basically, if we are running at perfect 60 fps, then the physics will update 8 times per second.
#define dt 0.00208333333333333333333333333333333333333333333333

typedef struct SpriteLayer {
	char* name;
} SpriteLayer;

enum {
	SpriteCellType_Sprite,
	SpriteCellType_Hitbox,
	SpriteCellType_Hurtbox,
	SpriteCellType_Origin,
};
typedef uint32_t SpriteCellType;

typedef struct SpriteCell {
	ivec2s offset;
	ivec2s size;
	SDL_Texture* texture;
	uint32_t layer_idx;
	uint32_t frame_idx;
	int32_t z_idx;
	SpriteCellType type;
} SpriteCell;

typedef struct SpriteFrame {
	double dur;
	SpriteCell* cells; size_t n_cells;
} SpriteFrame;

typedef struct SpriteDesc {
	char* path;
	ivec2s size;
	SpriteLayer* layers; size_t n_layers;
	SpriteFrame* frames; size_t n_frames;
} SpriteDesc;

typedef struct Sprite {
	int32_t idx;
} Sprite;

typedef struct Anim {
	Sprite sprite;
	double dt_accumulator;
	uint32_t frame_idx;
	bool ended;
} Anim;

enum {
	EntityType_Player,
	EntityType_Boar,
};
typedef uint32_t EntityType;

enum {
	EntityState_Inactive,
	EntityState_Die,
	EntityState_Attack,
	EntityState_Fall,
	EntityState_Jump,
	EntityState_Free,
	
	// TODO
	EntityState_Hurt,
	
};
typedef uint32_t EntityState;

typedef struct Entity {
	Anim anim;

	// The position is the position of the origin.
	// The origin is defined by the current sprite.
	// Each sprite has its own origin relative to its canvas.
	ivec2s start_pos;
	ivec2s pos;
	vec2s pos_remainder;

	vec2s vel;
	
	int32_t dir;

	EntityType type;
	EntityState state;
} Entity;

typedef struct Tile {
	ivec2s src;
	ivec2s dst;
} Tile;

typedef struct TileLayer {
	Tile* tiles; // n_tiles = size.x*size.y/TILE_SIZE
} TileLayer;

typedef struct Level {
	ivec2s size;
	Entity player;
	Entity* enemies; size_t n_enemies;
	TileLayer* tile_layers; size_t n_tile_layers;
	bool* tiles; // n_tiles = size.x*size.y/TILE_SIZE
} Level;

typedef struct Rect {
	ivec2s min;
	ivec2s max;
} Rect;

typedef struct ReplayFrame {
	Entity player;
	Entity* enemies; size_t n_enemies;
} ReplayFrame;

typedef struct Arena {
	uint8_t* first;
	uint8_t* last;
	uint8_t* cur;
} Arena;

typedef struct Context {
#if ENABLE_PROFILING
	SpallProfile spall_ctx;
	SpallBuffer spall_buffer;
#endif

	Arena perm;
	Arena temp;

	SDL_Window* window;

	SDL_Renderer* renderer;
	bool vsync;

	SDL_Gamepad* gamepad;
	vec2s gamepad_left_stick;

	bool button_left;
	bool button_right;
	bool button_jump;
	bool button_jump_released;
	bool button_attack;

	bool left_mouse_pressed;
	vec2s mouse_pos;

	SDL_Time time;
	double dt_accumulator;
	double refresh_rate;
	
	Level* levels; size_t n_levels;
	size_t level_idx;

	bool running;

	SpriteDesc sprites[MAX_SPRITES];

	ReplayFrame* replay_frames; 
	size_t replay_frame_idx; 
	size_t replay_frame_idx_max;
	size_t c_replay_frames;
	bool paused;
} Context;

#include "util.c"

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

function bool SetSprite(Entity* entity, Sprite sprite) {
    bool sprite_changed = false;
    if (!SpritesEqual(entity->anim.sprite, sprite)) {
        sprite_changed = true;
        entity->anim.sprite = sprite;
        ResetAnim(&entity->anim);
    }
    return sprite_changed;
}

function void ResetGame(Context* ctx) {
	SPALL_BUFFER_BEGIN();

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
			SetSprite(player, player_idle);
		}
		for (size_t enemy_idx = 0; enemy_idx < level->n_enemies; ++enemy_idx) {
			Entity* enemy = &level->enemies[enemy_idx];
			enemy->state = EntityState_Free;
			enemy->pos = enemy->start_pos;
			enemy->vel = (vec2s){0.0f};
			enemy->dir = 1;
			ResetAnim(&enemy->anim);
			if (enemy->type == EntityType_Boar) {
				SetSprite(enemy, boar_idle);
			}
		}
	}

	SPALL_BUFFER_END();
}

function ivec2s GetSpriteOrigin(Context* ctx, Sprite sprite, int32_t dir) {
	SPALL_BUFFER_BEGIN();
	ivec2s res = {0};

	// Find origin
	SpriteDesc* sd = GetSpriteDesc(ctx, sprite);
	SpriteFrame* frame = &sd->frames[0];
	for (size_t cell_idx = 0; cell_idx < frame->n_cells; ++cell_idx) {
		SpriteCell* cell = &frame->cells[cell_idx];
		if (cell->type == SpriteCellType_Origin) {
			res = cell->offset;
			break;
		}
	}

	// Flip if necessary
	if (dir == -1) {
		res.x = sd->size.x - res.x;
	}

	SPALL_BUFFER_END();
	return res;
}

function void DrawSprite(Context* ctx, Sprite sprite, size_t frame, vec2s pos, int32_t dir) {
	SPALL_BUFFER_BEGIN();

	SpriteDesc* sd = GetSpriteDesc(ctx, sprite);
	SDL_assert(sd->frames && "invalid sprite");
	SpriteFrame* sf = &sd->frames[frame];
	ivec2s origin = GetSpriteOrigin(ctx, sprite, dir);
	for (size_t cell_idx = 0; cell_idx < sf->n_cells; ++cell_idx) {
		SpriteCell* cell = &sf->cells[cell_idx];
		if (cell->texture) {
			SDL_FRect srcrect = {
				0.0f,
				0.0f,
				(float)(cell->size.x),
				(float)(cell->size.y),
			};

			SDL_FRect dstrect = {
				pos.x + (float)(cell->offset.x*dir - origin.x),
				pos.y + (float)(cell->offset.y - origin.y),
				(float)(cell->size.x*dir),
				(float)(cell->size.y),
			};

			if (dir == -1) {
				dstrect.x += (float)sd->size.x;
			}

			SDL_CHECK(SDL_RenderTexture(ctx->renderer, cell->texture, &srcrect, &dstrect));
		}
	}

	SPALL_BUFFER_END();
}

function ivec2s GetTilesetDimensions(Context* ctx, Sprite tileset) {
	SpriteDesc* sd = GetSpriteDesc(ctx, tileset);
	SDL_assert(sd->n_layers == 1);
	SDL_assert(sd->n_frames == 1);
	SDL_assert(sd->frames[0].n_cells == 1);

	SDL_Texture* texture = sd->frames[0].cells[0].texture;

	return (ivec2s){texture->w/TILE_SIZE, texture->h/TILE_SIZE};
}

function bool GetSpriteHitbox(Context* ctx, Sprite sprite, size_t frame_idx, int32_t dir, Rect* hitbox) {
	SPALL_BUFFER_BEGIN();
	bool res = false;

	SDL_assert(hitbox);
	SDL_assert(dir == 1 || dir == -1);
	SpriteDesc* sd = GetSpriteDesc(ctx, sprite);
	SDL_assert(frame_idx < sd->n_frames);
	SpriteFrame* frame = &sd->frames[frame_idx];
	for (size_t cell_idx = 0; cell_idx < frame->n_cells; ++cell_idx) {
		SpriteCell* cell = &frame->cells[cell_idx];
		if (cell->type == SpriteCellType_Hitbox) {
			if (dir == 1) {
				hitbox->min.x = cell->offset.x;
				hitbox->max.x = cell->offset.x + cell->size.x;
				hitbox->min.y = cell->offset.y;
				hitbox->max.y = cell->offset.y + cell->size.y;
			} else {
				hitbox->min.x = -cell->offset.x - cell->size.x;
				hitbox->max.x = -cell->offset.x;
				hitbox->min.y = cell->offset.y;
				hitbox->max.y = cell->offset.y + cell->size.y;
				
				hitbox->min.x += sd->size.x;
				hitbox->max.x += sd->size.x;
			}

			// HACK
			--hitbox->max.x;
			--hitbox->max.y;

			ivec2s origin = GetSpriteOrigin(ctx, sprite, dir);
			hitbox->min = glms_ivec2_sub(hitbox->min, origin);
			hitbox->max = glms_ivec2_sub(hitbox->max, origin);

			res = true;
			break;
		}
	}

	SPALL_BUFFER_END();
	return res;
}

function void UpdateAnim(Context* ctx, Anim* anim, bool loop) {
	SPALL_BUFFER_BEGIN();

    SpriteDesc* sd = GetSpriteDesc(ctx, anim->sprite);
	double dur = sd->frames[anim->frame_idx].dur;
	size_t n_frames = sd->n_frames;

    if (loop || !anim->ended) {
        anim->dt_accumulator += dt;
        if (anim->dt_accumulator >= dur) {
            anim->dt_accumulator = 0.0;
            ++anim->frame_idx;
            if (anim->frame_idx >= n_frames) {
                if (loop) anim->frame_idx = 0;
                else {
                    --anim->frame_idx;
                    anim->ended = true;
                }
            }
        }
    }

    SPALL_BUFFER_END();
}

function SDL_EnumerationResult EnumerateSpriteDirectory(void *userdata, const char *dirname, const char *fname) {
	Context* ctx = userdata;
	SPALL_BUFFER_BEGIN();

	// dirname\fname\file

	char dir_path[1024];
	SDL_CHECK(SDL_snprintf(dir_path, 1024, "%s%s", dirname, fname) >= 0);

	int32_t n_files;
	char** files = SDL_GlobDirectory(dir_path, "*.aseprite", 0, &n_files); SDL_CHECK(files);
	if (n_files > 0) {
		size_t raw_chunk_alloc_size = 4096ULL * 32ULL;
		void* raw_chunk = ArenaAllocRaw(&ctx->temp, raw_chunk_alloc_size);

		size_t dst_buf_alloc_size = 4096ULL * 2048ULL;
		void* dst_buf = ArenaAllocRaw(&ctx->temp, dst_buf_alloc_size);

		for (size_t file_idx = 0; file_idx < (size_t)n_files; ++file_idx) {
			char* file = files[file_idx];

			char sprite_path[2048];
			SDL_CHECK(SDL_snprintf(sprite_path, sizeof(sprite_path), "%s\\%s", dir_path, file) >= 0);
			SDL_CHECK(SDL_GetPathInfo(sprite_path, NULL));

			Sprite sprite = GetSprite(sprite_path);
			SpriteDesc* sd = GetSpriteDesc(ctx, sprite);
			SDL_assert(!sd && "Collision");
			sd = &ctx->sprites[sprite.idx];

			{
				size_t buf_len = SDL_strlen(sprite_path) + 1;
				sd->path = ArenaAlloc(&ctx->perm, (uint64_t)buf_len, char);
				SDL_strlcpy(sd->path, sprite_path, buf_len);
			}
			
			SDL_IOStream* fs = SDL_IOFromFile(sprite_path, "r"); SDL_CHECK(fs);
			
			// LoadSprite
			{
				SPALL_BUFFER_BEGIN_NAME("LoadSprite");
				/* 
				This loops through the frames in three passes. These are:
				1. Get layer count and for each frame, get cell count.
				2. Get layer names.
				3. Everything else, including decompressing texture data and uploading it to the GPU.
				*/

				ASE_Header header;
				SDL_ReadStructChecked(fs, &header);

				SDL_assert(header.magic_number == 0xA5E0);
				SDL_assert(header.color_depth == 32);
				SDL_assert((header.pixel_w == 0 || header.pixel_w == 1) && (header.pixel_h == 0 || header.pixel_h == 1));
				
				sd->size.x = (int32_t)header.w;
				sd->size.y = (int32_t)header.h;
				SDL_assert(header.grid_x == 0);
				SDL_assert(header.grid_y == 0);
				SDL_assert(header.grid_w == 0 || header.grid_w == 16);
				SDL_assert(header.grid_h == 0 || header.grid_h == 16);
				sd->n_frames = (size_t)header.n_frames;
				sd->frames = ArenaAlloc(&ctx->perm, sd->n_frames, SpriteFrame);

				int64_t fs_save = SDL_TellIO(fs); SDL_CHECK(fs_save != -1);

				for (size_t frame_idx = 0; frame_idx < sd->n_frames; ++frame_idx) {
					ASE_Frame frame;
					SDL_ReadStructChecked(fs, &frame);
					SDL_assert(frame.magic_number == 0xF1FA);

					// Would mean this aseprite file is very old.
					SDL_assert(frame.n_chunks != 0);

					for (size_t chunk_idx = 0; chunk_idx < frame.n_chunks; ++chunk_idx) {
						ASE_ChunkHeader chunk_header;
						SDL_ReadStructChecked(fs, &chunk_header);
						if (chunk_header.size == sizeof(ASE_ChunkHeader)) continue;

						size_t raw_chunk_size = chunk_header.size - sizeof(ASE_ChunkHeader);
						SDL_assert(raw_chunk_alloc_size >= raw_chunk_size);
						SDL_ReadIOChecked(fs, raw_chunk, raw_chunk_size);

						switch (chunk_header.type) {
						case ASE_ChunkType_Layer: {
							++sd->n_layers;
						} break;
						case ASE_ChunkType_Cell: {
							++sd->frames[frame_idx].n_cells;
						} break;
						}
					}
				}
				sd->layers = ArenaAlloc(&ctx->perm, sd->n_layers, SpriteLayer); // We don't have to zero this out.
				
				for (size_t frame_idx = 0; frame_idx < sd->n_frames; ++frame_idx) {
					if (sd->frames[frame_idx].n_cells > 0) {
						sd->frames[frame_idx].cells = ArenaAlloc(&ctx->perm, sd->frames[frame_idx].n_cells, SpriteCell);
					}
				}

				size_t layer_idx;

				SDL_CHECK(SDL_SeekIO(fs, fs_save, SDL_IO_SEEK_SET) != -1);
				layer_idx = 0;
				for (size_t frame_idx = 0; frame_idx < sd->n_frames; ++frame_idx) {
					ASE_Frame frame;
					SDL_ReadStructChecked(fs, &frame);
					SDL_assert(frame.magic_number == 0xF1FA);

					// Would mean this aseprite file is very old.
					SDL_assert(frame.n_chunks != 0);

					for (size_t chunk_idx = 0; chunk_idx < frame.n_chunks; ++chunk_idx) {
						ASE_ChunkHeader chunk_header;
						SDL_ReadStructChecked(fs, &chunk_header);
						if (chunk_header.size == sizeof(ASE_ChunkHeader)) continue;

						size_t raw_chunk_size = chunk_header.size - sizeof(ASE_ChunkHeader);
						SDL_ReadIOChecked(fs, raw_chunk, raw_chunk_size);

						switch (chunk_header.type) {
						case ASE_ChunkType_Layer: {
							// TODO: tileset_idx and uuid
							ASE_LayerChunk* chunk = raw_chunk;
							SpriteLayer sprite_layer = {0};
							if (chunk->layer_name.len > 0) {
								sprite_layer.name = ArenaAlloc(&ctx->perm, chunk->layer_name.len + 1, char);
								SDL_strlcpy(sprite_layer.name, (const char*)(chunk+1), chunk->layer_name.len + 1);
							}
							sd->layers[layer_idx++] = sprite_layer;
						} break;
						}
					}
				}

				SDL_CHECK(SDL_SeekIO(fs, fs_save, SDL_IO_SEEK_SET) != -1);
				layer_idx = 0;
				for (size_t frame_idx = 0; frame_idx < sd->n_frames; ++frame_idx) {
					size_t cell_idx = 0;
					ASE_Frame frame;
					SDL_ReadStructChecked(fs, &frame);
					SDL_assert(frame.magic_number == 0xF1FA);

					// Would mean this aseprite file is very old.
					SDL_assert(frame.n_chunks != 0);

					sd->frames[frame_idx].dur = ((double)frame.frame_dur)/1000.0;

					for (size_t chunk_idx = 0; chunk_idx < frame.n_chunks; ++chunk_idx) {
						ASE_ChunkHeader chunk_header;
						SDL_ReadStructChecked(fs, &chunk_header);
						if (chunk_header.size == sizeof(ASE_ChunkHeader)) continue;

						size_t raw_chunk_size = chunk_header.size - sizeof(ASE_ChunkHeader);
						SDL_ReadIOChecked(fs, raw_chunk, raw_chunk_size);

						switch (chunk_header.type) {
						case ASE_ChunkType_OldPalette: {
						} break;
						case ASE_ChunkType_OldPalette2: {
						} break;
						case ASE_ChunkType_Cell: {
							ASE_CellChunk* chunk = raw_chunk;
							SpriteCell cell = {
								.layer_idx = (uint32_t)layer_idx,
								.frame_idx = (uint32_t)frame_idx,
								.offset.x = chunk->x,
								.offset.y = chunk->y,
								.z_idx = (ssize_t)chunk->z_idx,
							};
							SDL_assert(chunk->type == ASE_CellType_CompressedImage);
							switch (chunk->type) {
								case ASE_CellType_Raw: {
								} break;
								case ASE_CellType_Linked: {
								} break;
								case ASE_CellType_CompressedImage: {
									cell.size.x = chunk->compressed_image.w;
									cell.size.y = chunk->compressed_image.h;

									if (SDL_strcmp(sd->layers[chunk->layer_idx].name, "Hitbox") == 0) {
										cell.type = SpriteCellType_Hitbox;
									} else if (SDL_strcmp(sd->layers[chunk->layer_idx].name, "Origin") == 0) {
										cell.type = SpriteCellType_Origin;
									} else {
										// It's the zero-sized array at the end of ASE_CellChunk.
										size_t src_buf_size = raw_chunk_size - sizeof(ASE_CellChunk) - 2; 
										void* src_buf = (void*)((&chunk->compressed_image.h)+1);

										size_t dst_buf_size = sizeof(uint32_t)*cell.size.x*cell.size.y;
										SDL_assert(dst_buf_alloc_size >= dst_buf_size);
										SPALL_BUFFER_BEGIN_NAME("INFL_ZInflate");
										size_t res = INFL_ZInflate(dst_buf, dst_buf_size, src_buf, src_buf_size);
										SPALL_BUFFER_END();
										SDL_assert(res > 0);

										SDL_Surface* surf = SDL_CreateSurfaceFrom(cell.size.x, cell.size.y, SDL_PIXELFORMAT_RGBA32, dst_buf, sizeof(uint32_t)*cell.size.x); SDL_CHECK(surf);
										cell.texture = SDL_CreateTextureFromSurface(ctx->renderer, surf); SDL_CHECK(cell.texture);
										SDL_DestroySurface(surf);
									}
								} break;
								case ASE_CellType_CompressedTilemap: {
								} break;
							}
							sd->frames[frame_idx].cells[cell_idx++] = cell;
						} break;
						case ASE_ChunkType_CellExtra: {
							ASE_CellChunk* chunk = raw_chunk;
							UNUSED(chunk);
							ASE_CellExtraChunk* extra_chunk = (ASE_CellExtraChunk*)((uintptr_t)raw_chunk + (uintptr_t)chunk_header.size - sizeof(ASE_CellExtraChunk));
							UNUSED(extra_chunk);
						} break;
						case ASE_ChunkType_ColorProfile: {
							ASE_ColorProfileChunk* chunk = raw_chunk;
							switch (chunk->type) {
							case ASE_ColorProfileType_None: {

							} break;
							case ASE_ColorProfileType_SRGB: {

							} break;
							case ASE_ColorProfileType_EmbeddedICC: {

							} break;
							default: {
								SDL_assert(false);
							} break;
							}
						} break;
						case ASE_ChunkType_ExternalFiles: {
							ASE_ExternalFilesChunk* chunk = raw_chunk;
							UNUSED(chunk);
							SDL_Log("ASE_CHUNK_TYPE_EXTERNAL_FILES");
						} break;
						case ASE_ChunkType_DeprecatedMask: {
						} break;
						case ASE_ChunkType_Path: {
							SDL_Log("ASE_CHUNK_TYPE_PATH");
						} break;
						case ASE_ChunkType_Tags: {
							ASE_TagsChunk* chunk = raw_chunk;
							ASE_Tag* tags = (ASE_Tag*)(chunk+1);
							for (size_t tag_idx = 0; tag_idx < (size_t)chunk->n_tags; ++tag_idx) {
								ASE_Tag* tag = &tags[tag_idx]; UNUSED(tag);
							}
						} break;
						case ASE_ChunkType_Palette: {
							ASE_PaletteChunk* chunk = raw_chunk; // TODO: Do I have to do anything with this?
							UNUSED(chunk);
						} break;
						case ASE_ChunkType_UserData: {
							SDL_Log("ASE_CHUNK_TYPE_USER_DATA");
						} break;
						case ASE_ChunkType_Slice: {
							SDL_Log("ASE_CHUNK_TYPE_SLICE");
						} break;
						case ASE_ChunkType_Tileset: {
							SDL_Log("ASE_CHUNK_TYPE_TILESET");
						} break;
						}
					}
				}

				SPALL_BUFFER_END();
			}

			SDL_CloseIO(fs);
		}

		// Normally this should only be reset at the end of every frame, but in this case I know it will work.
		ArenaReset(&ctx->temp);

	} else if (n_files == 0) {
		SDL_CHECK(SDL_EnumerateDirectory(dir_path, EnumerateSpriteDirectory, ctx));
	}
	
	SDL_free(files);
	SPALL_BUFFER_END();
	return SDL_ENUM_CONTINUE;
}

function void DrawAnim(Context* ctx, Anim* anim, vec2s pos, int32_t dir) {
    DrawSprite(ctx, anim->sprite, anim->frame_idx, pos, dir);
}

function void DrawEntity(Context* ctx, Entity* entity) {
    if (entity->state != EntityState_Inactive) {
        DrawAnim(ctx, &entity->anim, vec2_from_ivec2(entity->pos), entity->dir);
    }
}

function Rect GetEntityHitbox(Context* ctx, Entity* entity) {
	SPALL_BUFFER_BEGIN();
	Rect hitbox = {0};
	SpriteDesc* sd = GetSpriteDesc(ctx, entity->anim.sprite);

	/* 	
	I'll admit this part of the function is kind of weird. I might end up changing it later.
	The way it works is: we start from the current frame and go backward.
	For each frame, check if there is a corresponding hitbox. If so, pick that one.
	If no hitbox is found and the frame index is 0, loop forward instead.
	*/
	{
		bool res; ssize_t frame_idx;
		for (res = false, frame_idx = entity->anim.frame_idx; !res && frame_idx >= 0; --frame_idx) {
			res = GetSpriteHitbox(ctx, entity->anim.sprite, (size_t)frame_idx, entity->dir, &hitbox); 
		}
		if (!res && entity->anim.frame_idx == 0) {
			for (frame_idx = 1; !res && frame_idx < (ssize_t)sd->n_frames; ++frame_idx) {
				res = GetSpriteHitbox(ctx, entity->anim.sprite, (size_t)frame_idx, entity->dir, &hitbox);
			}
		}
		SDL_assert(res);
	}

	hitbox.min = glms_ivec2_add(hitbox.min, entity->pos);
	hitbox.max = glms_ivec2_add(hitbox.max, entity->pos);

	SPALL_BUFFER_END();
	return hitbox;
}

function bool EntitiesIntersect(Context* ctx, Entity* a, Entity* b) {
    if (a->state == EntityState_Inactive || b->state == EntityState_Inactive) return false;
    Rect ha = GetEntityHitbox(ctx, a);
    Rect hb = GetEntityHitbox(ctx, b);
    return RectsIntersect(ha, hb);
}

// This returns a new EntityState instead of setting the 
// entity state directly because depending on the entity,
// certain states might not make sense.
function EntityState EntityMoveAndCollide(Context* ctx, Entity* entity, vec2s acc, float fric, float max_vel) {
	SPALL_BUFFER_BEGIN();
	Level* level = GetCurrentLevel(ctx);
	Rect prev_hitbox = GetEntityHitbox(ctx, entity);

	entity->vel = glms_vec2_add(entity->vel, acc);

	if (entity->state == EntityState_Free) {
		if (entity->vel.x < 0.0f) entity->vel.x = SDL_min(0.0f, entity->vel.x + fric);
		else if (entity->vel.x > 0.0f) entity->vel.x = SDL_max(0.0f, entity->vel.x - fric);
		entity->vel.x = SDL_clamp(entity->vel.x, -max_vel, max_vel);
	}

	bool horizontal_collision_happened = false;
	vec2s vel = entity->vel;
	if (entity->vel.x < 0.0f && prev_hitbox.min.x % TILE_SIZE == 0) {
		ivec2s grid_pos;
		grid_pos.x = (prev_hitbox.min.x-TILE_SIZE)/TILE_SIZE;
		for (grid_pos.y = prev_hitbox.min.y/TILE_SIZE; grid_pos.y <= prev_hitbox.max.y/TILE_SIZE; ++grid_pos.y) {
			if (IsSolid(level, grid_pos)) {
				vel.x = 0.0f;
				horizontal_collision_happened = true;
				break;
			}
		}
	} else if (entity->vel.x > 0.0f && (prev_hitbox.max.x+1) % TILE_SIZE == 0) {
		ivec2s grid_pos;
		grid_pos.x = (prev_hitbox.max.x+1)/TILE_SIZE;
		for (grid_pos.y = prev_hitbox.min.y/TILE_SIZE; grid_pos.y <= prev_hitbox.max.y/TILE_SIZE; ++grid_pos.y) {
			if (IsSolid(level, grid_pos)) {
				vel.x = 0.0f;
				horizontal_collision_happened = true;
				break;
			}
		}
	}
	bool touching_floor = false;
	bool vertical_collision_happened = false;
	if (entity->vel.y < 0.0f && prev_hitbox.min.y % TILE_SIZE == 0) {
		ivec2s grid_pos;
		grid_pos.y = (prev_hitbox.min.y-TILE_SIZE)/TILE_SIZE;
		for (grid_pos.x = prev_hitbox.min.x/TILE_SIZE; grid_pos.x <= prev_hitbox.max.x/TILE_SIZE; ++grid_pos.x) {
			if (IsSolid(level, grid_pos)) {
				vel.y = 0.0f;
				vertical_collision_happened = true;
				break;
			}
		}
	} else if (entity->vel.y > 0.0f && (prev_hitbox.max.y+1) % TILE_SIZE == 0) {
		ivec2s grid_pos;
		grid_pos.y = (prev_hitbox.max.y+1)/TILE_SIZE;
		for (grid_pos.x = prev_hitbox.min.x/TILE_SIZE; grid_pos.x <= prev_hitbox.max.x/TILE_SIZE; ++grid_pos.x) {
			if (IsSolid(level, grid_pos)) {
				vel.y = 0.0f;
				entity->vel.y = 0.0f;
				entity->state = EntityState_Free;
				touching_floor = true;
				vertical_collision_happened = true;
				break;
			}
		}
	}

    entity->pos_remainder = glms_vec2_add(entity->pos_remainder, vel);
    entity->pos = glms_ivec2_add(entity->pos, ivec2_from_vec2(glms_vec2_round(entity->pos_remainder)));
    entity->pos_remainder = glms_vec2_sub(entity->pos_remainder, glms_vec2_round(entity->pos_remainder));

    Rect hitbox = GetEntityHitbox(ctx, entity);

	ivec2s grid_pos;
	for (grid_pos.y = hitbox.min.y/TILE_SIZE; (!horizontal_collision_happened || !vertical_collision_happened) && grid_pos.y <= hitbox.max.y/TILE_SIZE; ++grid_pos.y) {
		for (grid_pos.x = hitbox.min.x/TILE_SIZE; (!horizontal_collision_happened || !vertical_collision_happened) && grid_pos.x <= hitbox.max.x/TILE_SIZE; ++grid_pos.x) {
			if (IsSolid(level, grid_pos)) {
				Rect tile_rect;
				tile_rect.min = glms_ivec2_scale(grid_pos, TILE_SIZE);
				tile_rect.max = glms_ivec2_adds(tile_rect.min, TILE_SIZE);
				if (RectsIntersect(hitbox, tile_rect)) {
					if (RectsIntersect(prev_hitbox, tile_rect)) continue;

					if (!horizontal_collision_happened) {
						Rect h = prev_hitbox;
						h.min.x = hitbox.min.x;
						h.max.x = hitbox.max.x;
						if (RectsIntersect(h, tile_rect)) {
							int32_t amount = 0;
							int32_t incr = (int32_t)glm_signf(entity->vel.x);
							while (RectsIntersect(h, tile_rect)) {
								h.min.x -= incr;
								h.max.x -= incr;
								amount += incr;
							}
							entity->pos.x -= amount;
							horizontal_collision_happened = true;
						}

					}
					if (!vertical_collision_happened) {
						Rect h = prev_hitbox;
						h.min.y = hitbox.min.y;
						h.max.y = hitbox.max.y;
						if (RectsIntersect(h, tile_rect)) {
							int32_t amount = 0;
							int32_t incr = (int32_t)glm_signf(entity->vel.y);
							while (RectsIntersect(h, tile_rect)) {
								h.min.y -= incr;
								h.max.y -= incr;
								amount += incr;
							}
							entity->pos.y -= amount;					
							vertical_collision_happened = true;
						}
					}
				}
			}
		}
	}

	EntityState res;
	if (!touching_floor && entity->vel.y > 0.0f) {
		res = EntityState_Fall;
	} else {
		res = entity->state;
	}

	SPALL_BUFFER_END();
	return res;
}

function void UpdatePlayer(Context* ctx) {
	SPALL_BUFFER_BEGIN();
	Entity* player = GetPlayer(ctx);
	Level* level = GetCurrentLevel(ctx);

	int32_t input_dir = 0;
	if (ctx->gamepad) {
		if (ctx->gamepad_left_stick.x == 0.0f) input_dir = 0;
		else if (ctx->gamepad_left_stick.x > 0.0f) input_dir = 1;
		else if (ctx->gamepad_left_stick.x < 0.0f) input_dir = -1;
	} else {
		input_dir = ctx->button_right - ctx->button_left;
	}

	switch (player->state) {
	case EntityState_Free: {
		if (ctx->button_attack) {
			player->state = EntityState_Attack;
		} else if (ctx->button_jump) {
			player->state = EntityState_Jump;
		}
	} break;
	case EntityState_Jump: {
		if (ctx->button_jump_released) {
			player->vel.y /= 2.0f;
			player->state = EntityState_Fall;
		}
	} break;
	default: {
		// TODO?
	} break;
	}

	if (player->pos.y > (float)level->size.y) {
		ResetGame(ctx);
	} else switch (player->state) {
	case EntityState_Inactive:
		break;
    case EntityState_Die: {
		SetSprite(player, player_die);
		bool loop = false;
		UpdateAnim(ctx, &player->anim, loop);
		if (player->anim.ended) {
			ResetGame(ctx);
		}
	} break;

    case EntityState_Attack: {
		SetSprite(player, player_attack);

		size_t n_enemies; Entity* enemies = GetEnemies(ctx, &n_enemies);
		for (size_t enemy_idx = 0; enemy_idx < n_enemies; ++enemy_idx) {
			Entity* enemy = &enemies[enemy_idx];
			if (EntitiesIntersect(ctx, player, enemy)) {
				switch (enemy->type) {
				case EntityType_Boar:
					enemy->state = EntityState_Hurt;
					break;
				default:
					break;
				}
			}
		}
		
		bool loop = false;
		UpdateAnim(ctx, &player->anim, loop);
		if (player->anim.ended) {
			player->state = EntityState_Free;
		}
	} break;
    	
    case EntityState_Fall: {
    	SetSprite(player, player_jump_end);

    	vec2s acc = {0.0f, GRAVITY};
    	player->state = EntityMoveAndCollide(ctx, player, acc, PLAYER_FRIC, PLAYER_MAX_VEL);

    	bool loop = false;
    	UpdateAnim(ctx, &player->anim, loop);
	} break;
    	
	case EntityState_Jump: {
		vec2s acc = {0.0f};
		if (SetSprite(player, player_jump_start)) {
			acc.y -= PLAYER_JUMP;
		}

    	acc.y += GRAVITY;
    	player->state = EntityMoveAndCollide(ctx, player, acc, PLAYER_FRIC, PLAYER_MAX_VEL);

		bool loop = false;
    	UpdateAnim(ctx, &player->anim, loop);
	} break;

	case EntityState_Free: {
		vec2s acc = {0.0f, 0.0f};

		acc.y += GRAVITY;

		if (!ctx->gamepad) {
			acc.x = (float)input_dir * PLAYER_ACC;
		} else {
			acc.x = ctx->gamepad_left_stick.x * PLAYER_ACC;
		}

		if (input_dir == 0 && player->vel.x == 0.0f) {
			SetSprite(player, player_idle);
		} else {
			SetSprite(player, player_run);
			if (player->vel.x != 0.0f) {
				player->dir = (int32_t)glm_signf(player->vel.x);
			} else if (input_dir != 0) {
				player->dir = input_dir;
			}
		}

		player->state = EntityMoveAndCollide(ctx, player, acc, PLAYER_FRIC, PLAYER_MAX_VEL);		

		bool loop = true;
		UpdateAnim(ctx, &player->anim, loop);
	} break;

	default: {
		// TODO?
	} break;
	}

	SPALL_BUFFER_END();
}

function void UpdateBoar(Context* ctx, Entity* boar) {
	SPALL_BUFFER_BEGIN();

	switch (boar->state) {
	case EntityState_Hurt: {
		SetSprite(boar, boar_hit);

		bool loop = false;
		UpdateAnim(ctx, &boar->anim, loop);

		if (boar->anim.ended) {
			boar->state = EntityState_Inactive;
		}
	} break;

	case EntityState_Fall: {
		SetSprite(boar, boar_idle); // TODO: Is there a boar fall sprite? Could I make one?

		vec2s acc = {0.0f, GRAVITY};
		boar->state = EntityMoveAndCollide(ctx, boar, acc, BOAR_FRIC, BOAR_MAX_VEL);

		bool loop = true;
		UpdateAnim(ctx, &boar->anim, loop);
	} break;

	case EntityState_Free: {
		SetSprite(boar, boar_idle);

		vec2s acc = {0.0f, GRAVITY};
		boar->state = EntityMoveAndCollide(ctx, boar, acc, BOAR_FRIC, BOAR_MAX_VEL);

		bool loop = true;
		UpdateAnim(ctx, &boar->anim, loop);
	} break;

	default: {
		// TODO?
	} break;
	}

	SPALL_BUFFER_END();
}

function void SetReplayFrame(Context* ctx, size_t replay_frame_idx) {
	SPALL_BUFFER_BEGIN();

	ctx->replay_frame_idx = replay_frame_idx;
	Level* level = GetCurrentLevel(ctx);
	ReplayFrame* replay_frame = &ctx->replay_frames[ctx->replay_frame_idx];
	level->player = replay_frame->player;
	SDL_memcpy(level->enemies, replay_frame->enemies, replay_frame->n_enemies * sizeof(Entity));
	level->n_enemies = replay_frame->n_enemies;

	SPALL_BUFFER_END();
}

function int32_t CompareSpriteCells(const SpriteCell* a, const SpriteCell* b) {
    ssize_t a_order = (ssize_t)a->layer_idx + a->z_idx;
    ssize_t b_order = (ssize_t)b->layer_idx + b->z_idx;
    if ((a_order < b_order) || ((a_order == b_order) && (a->z_idx < b->z_idx))) {
        return -1;
    } else if ((b_order < a_order) || ((b_order == a_order) && (b->z_idx < a->z_idx))) {
        return 1;
    }
    return 0;
}

int32_t main(int32_t argc, char* argv[]) {
	UNUSED(argc);
	UNUSED(argv);

	// SetSprites
	{
		// This is the only time that we set the sprite variables.
		// After that, they are effectively constants.

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

	// InitContext
	Context* ctx;
	{
		SDL_CHECK(SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD));

		uint64_t memory_size = 1024ULL * 1024ULL * 32ULL;

		uint8_t* memory = SDL_malloc(memory_size); SDL_CHECK(memory);
		Arena perm;
		perm.first = memory;
		perm.last = memory + memory_size/2;
		perm.cur = perm.first;
		Arena temp;
		temp.first = memory + memory_size/2 + 1;
		temp.last = memory + memory_size;
		temp.cur = temp.first;

		ctx = ArenaAlloc(&perm, 1, Context);
		ctx->perm = perm;
		ctx->temp = temp;
	}

#if ENABLE_PROFILING
	{
		bool ok;

		ok = spall_init_file("profile/profile.spall", 1, &ctx->spall_ctx); SDL_assert(ok);

		int32_t buffer_size = 1 * 1024 * 1024;
		uint8_t* buffer = SDL_malloc(buffer_size); SDL_CHECK(buffer);
		ctx->spall_buffer = (SpallBuffer){
			.length = buffer_size,
			.data = buffer,
		};
		ok = spall_buffer_init(&ctx->spall_ctx, &ctx->spall_buffer); SDL_assert(ok);
	}
#endif

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

		if (display_mode->refresh_rate_numerator == 0) {
			ctx->refresh_rate = 0.01666666666666666666666666666666666666666666666667;
		} else {
			ctx->refresh_rate = (double)display_mode->refresh_rate_numerator / (double)display_mode->refresh_rate_denominator;
		}

		SDL_CHECK(SDL_SetDefaultTextureScaleMode(ctx->renderer, SDL_SCALEMODE_PIXELART));
		SDL_CHECK(SDL_SetRenderScale(ctx->renderer, (float)(display_mode->w/GAME_WIDTH), (float)(display_mode->h/GAME_HEIGHT)));
		SDL_CHECK(SDL_SetRenderDrawBlendMode(ctx->renderer, SDL_BLENDMODE_BLEND));
	}
	
	// LoadSprites
	{
		SDL_CHECK(SDL_EnumerateDirectory("assets\\legacy_fantasy_high_forest", EnumerateSpriteDirectory, ctx));
	}

	// SortSprites
	{
		SPALL_BUFFER_BEGIN_NAME("SortSprites");

		for (size_t sprite_idx = 0; sprite_idx < MAX_SPRITES; ++sprite_idx) {
			SpriteDesc* sd = GetSpriteDesc(ctx, (Sprite){(int32_t)sprite_idx});
			if (sd) {
				for (size_t frame_idx = 0; frame_idx < sd->n_frames; ++frame_idx) {
					SpriteFrame* sf = &sd->frames[frame_idx];
					SDL_qsort(sf->cells, sf->n_cells, sizeof(SpriteCell), (SDL_CompareCallback)CompareSpriteCells);
				}
			}
		}

		SPALL_BUFFER_END();
	}

	// LoadLevels
	{
		SPALL_BUFFER_BEGIN_NAME("LoadLevels");

		cJSON* head;
		{
			size_t file_len;
			void* file_data = SDL_LoadFile("assets\\levels\\test.ldtk", &file_len); SDL_CHECK(file_data);
			head = cJSON_ParseWithLength((const char*)file_data, file_len);
			SDL_free(file_data);
		}
		SDL_assert(HAS_FLAG(head->type, cJSON_Object));

		cJSON* level_nodes = cJSON_GetObjectItem(head, "levels");
		cJSON* level_node; cJSON_ArrayForEach(level_node, level_nodes) {
			++ctx->n_levels;
		}
		ctx->levels = ArenaAlloc(&ctx->perm, ctx->n_levels, Level);
		size_t level_idx = 0;

		// LoadLevel
		cJSON_ArrayForEach(level_node, level_nodes) {
			Level level = {0};

			cJSON* w = cJSON_GetObjectItem(level_node, "pxWid");
			level.size.x = (int32_t)cJSON_GetNumberValue(w);

			cJSON* h = cJSON_GetObjectItem(level_node, "pxHei");
			level.size.y = (int32_t)cJSON_GetNumberValue(h);

			size_t n_tiles = (size_t)(level.size.x*level.size.y/TILE_SIZE);
			level.tiles = ArenaAlloc(&ctx->perm, n_tiles, bool);

			// TODO
			level.n_tile_layers = 3;
			level.tile_layers = ArenaAlloc(&ctx->perm, level.n_tile_layers, TileLayer);
			for (size_t tile_layer_idx = 0; tile_layer_idx < level.n_tile_layers; ++tile_layer_idx) {
				TileLayer* tile_layer = &level.tile_layers[tile_layer_idx];
				size_t n_tiles = (size_t)level.size.x*level.size.y/TILE_SIZE;
				tile_layer->tiles = ArenaAlloc(&ctx->perm, n_tiles, Tile);
				for (size_t i = 0; i < n_tiles; ++i) {
					tile_layer->tiles[i].src.x = -1;
					tile_layer->tiles[i].src.x = -1;
					tile_layer->tiles[i].dst.x = -1;
					tile_layer->tiles[i].dst.y = -1;
				}
			}

			const char* layer_player = "Player";
			const char* layer_enemies = "Enemies";
			const char* layer_tiles = "Tiles";
			const char* layer_props = "Props";
			const char* layer_grass = "Grass";

			cJSON* layer_instances = cJSON_GetObjectItem(level_node, "layerInstances");
			cJSON* layer_instance; cJSON_ArrayForEach(layer_instance, layer_instances) {
				cJSON* node_ident = cJSON_GetObjectItem(layer_instance, "__identifier");
				char* ident = cJSON_GetStringValue(node_ident); SDL_assert(ident);

				if (SDL_strcmp(ident, layer_enemies) == 0) {
					cJSON* entity_instances = cJSON_GetObjectItem(layer_instance, "entityInstances");
					cJSON* entity_instance; cJSON_ArrayForEach(entity_instance, entity_instances) {
						++level.n_enemies;
					}
				}
			}

			level.enemies = ArenaAlloc(&ctx->perm, level.n_enemies, Entity);
			Entity* enemy = level.enemies;
			cJSON_ArrayForEach(layer_instance, layer_instances) {
				cJSON* node_type = cJSON_GetObjectItem(layer_instance, "__type");
				char* type = cJSON_GetStringValue(node_type); SDL_assert(type);
				cJSON* node_ident = cJSON_GetObjectItem(layer_instance, "__identifier");
				char* ident = cJSON_GetStringValue(node_ident); SDL_assert(ident);
				if (SDL_strcmp(type, "Entities") == 0) {
					cJSON* entity_instances = cJSON_GetObjectItem(layer_instance, "entityInstances");

					if (SDL_strcmp(ident, layer_player) == 0) {
						cJSON* entity_instance = entity_instances->child;
						cJSON* world_x = cJSON_GetObjectItem(entity_instance, "__worldX");
						cJSON* world_y = cJSON_GetObjectItem(entity_instance, "__worldY");
						level.player.start_pos = (ivec2s){(int32_t)cJSON_GetNumberValue(world_x), (int32_t)cJSON_GetNumberValue(world_y)};
					} else if (SDL_strcmp(ident, layer_enemies) == 0) {
						cJSON* entity_instance; cJSON_ArrayForEach(entity_instance, entity_instances) {
							cJSON* identifier_node = cJSON_GetObjectItem(entity_instance, "__identifier");
							char* identifier = cJSON_GetStringValue(identifier_node);
							cJSON* world_x = cJSON_GetObjectItem(entity_instance, "__worldX");
							cJSON* world_y = cJSON_GetObjectItem(entity_instance, "__worldY");
							enemy->start_pos = (ivec2s){(int32_t)cJSON_GetNumberValue(world_x), (int32_t)cJSON_GetNumberValue(world_y)};
							if (SDL_strcmp(identifier, "Boar") == 0) {
								enemy->type = EntityType_Boar;
							} // else if (SDL_strcmp(identifier, "") == 0) {}
							++enemy;
						}
					}
				} else if (SDL_strcmp(type, "Tiles") == 0) {
					cJSON* grid_tiles = cJSON_GetObjectItem(layer_instance, "gridTiles");

					// TODO
					TileLayer* tile_layer = NULL;
	 				if (SDL_strcmp(ident, layer_tiles) == 0) {
						tile_layer = &level.tile_layers[0];
					} else if (SDL_strcmp(ident, layer_props) == 0) {
						tile_layer = &level.tile_layers[1];
					} else if (SDL_strcmp(ident, layer_grass) == 0) {
						tile_layer = &level.tile_layers[2];
					} else {
						SDL_assert(!"Invalid layer!");
					}

					cJSON* grid_tile; cJSON_ArrayForEach(grid_tile, grid_tiles) {
						cJSON* src_node = cJSON_GetObjectItem(grid_tile, "src");
						ivec2s src = {
							(int32_t)cJSON_GetNumberValue(src_node->child),
							(int32_t)cJSON_GetNumberValue(src_node->child->next),
						};

						cJSON* dst_node = cJSON_GetObjectItem(grid_tile, "px");
						ivec2s dst = {
							(int32_t)cJSON_GetNumberValue(dst_node->child),
							(int32_t)cJSON_GetNumberValue(dst_node->child->next),
						};

						cJSON* node_src_idx = cJSON_GetObjectItem(grid_tile, "t");
						size_t src_idx = (size_t)cJSON_GetNumberValue(node_src_idx);
						tile_layer->tiles[src_idx] = (Tile){src, dst};
					}
				}
 				
			}
			ctx->levels[level_idx++] = level;
		}

		cJSON* defs = cJSON_GetObjectItem(head, "defs");
		cJSON* tilesets = cJSON_GetObjectItem(defs, "tilesets");
		bool break_all = false;
		cJSON* tileset; cJSON_ArrayForEach(tileset, tilesets) {
			cJSON* enum_tags = cJSON_GetObjectItem(tileset, "enumTags");
			cJSON* enum_tag; cJSON_ArrayForEach(enum_tag, enum_tags) {
				cJSON* id_node = cJSON_GetObjectItem(enum_tag, "enumValueId");
				char* id = cJSON_GetStringValue(id_node);
				if (SDL_strcmp(id, "Collide") != 0) continue;
				
				cJSON* tile_ids = cJSON_GetObjectItem(enum_tag, "tileIds");
				cJSON* tile_id; cJSON_ArrayForEach(tile_id, tile_ids) {
					size_t src_idx = (int32_t)cJSON_GetNumberValue(tile_id);
					TileLayer* tile_layer = &ctx->levels[0].tile_layers[0]; // TODO
					ivec2s dst = tile_layer->tiles[src_idx].dst;
					if (dst.x == -1 || dst.y == -1) continue;
					Level* level = GetCurrentLevel(ctx); // TODO
					size_t tile_idx = (size_t)((dst.x + dst.y*level->size.x)/TILE_SIZE);
					level->tiles[tile_idx] = true;
				}
				break_all = true;
				break;
			}
			if (break_all) break;
		}
		break_all = false;

		cJSON_Delete(head);

		SPALL_BUFFER_END();
	}

	// InitReplayFrames
	{
		ctx->c_replay_frames = 1024;
		ctx->replay_frames = SDL_malloc(ctx->c_replay_frames * sizeof(ReplayFrame)); SDL_CHECK(ctx->replay_frames);
	}

	ResetGame(ctx);

	ctx->running = true;
	while (ctx->running) {
		size_t times_updated = 0;
		while (ctx->dt_accumulator > dt && times_updated < 8) {	// TODO
			++times_updated;		
			// GetInput
			{
				SPALL_BUFFER_BEGIN_NAME("GetInput");

				if (!ctx->gamepad) {
					int32_t joystick_count = 0;
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

				SPALL_BUFFER_END();
			}
			if (!ctx->paused) {
				// UpdateGame
				{
					SPALL_BUFFER_BEGIN_NAME("UpdateGame");

					UpdatePlayer(ctx);
					size_t n_enemies; Entity* enemies = GetEnemies(ctx, &n_enemies);
					for (size_t enemy_idx = 0; enemy_idx < n_enemies; ++enemy_idx) {
						Entity* enemy = &enemies[enemy_idx];
						if (enemy->type == EntityType_Boar) {
							UpdateBoar(ctx, enemy);
						}
					}

					SPALL_BUFFER_END();
				}
				
				// RecordReplayFrame
				{
					SPALL_BUFFER_BEGIN_NAME("RecordReplayFrame");

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

					SPALL_BUFFER_END();
				}
			}

			ArenaReset(&ctx->temp);

	 		ctx->dt_accumulator -= dt;
		}
		SDL_Log("Times updated: %llu", times_updated);

		// DrawBegin
		{
			SPALL_BUFFER_BEGIN_NAME("DrawBegin");

			SDL_CHECK(SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, 0));
			SDL_CHECK(SDL_RenderClear(ctx->renderer));

			SPALL_BUFFER_END();
		}

		// DrawEntities
		{
			SPALL_BUFFER_BEGIN_NAME("DrawEntities");

			size_t n_enemies; Entity* enemies = GetEnemies(ctx, &n_enemies);
			for (size_t enemy_idx = 0; enemy_idx < n_enemies; ++enemy_idx) {
				Entity* enemy = &enemies[enemy_idx];
				DrawEntity(ctx, enemy);
			}
			DrawEntity(ctx, GetPlayer(ctx));

			SPALL_BUFFER_END();
		}

		// DrawLevel
		{
			SPALL_BUFFER_BEGIN_NAME("DrawLevel");

			Level* level = GetCurrentLevel(ctx);

			SpriteDesc* sd = GetSpriteDesc(ctx, spr_tiles);
			SDL_assert(sd->n_layers == 1);
			SDL_assert(sd->n_frames == 1);
			SDL_assert(sd->frames[0].n_cells == 1);

			SDL_Texture* texture = sd->frames[0].cells[0].texture;

			size_t draw_calls = 0;

			for (size_t tile_layer_idx = 0; tile_layer_idx < level->n_tile_layers; ++tile_layer_idx) {
				size_t n_tiles;
				Tile* tiles = GetLayerTiles(level, tile_layer_idx, &n_tiles);
				for (size_t tile_idx = 0; tile_idx < n_tiles; ++tile_idx) {
					Tile tile = tiles[tile_idx];

					if (tile.src.x == -1) continue;					
					
					SDL_FRect srcrect = {
						(float)tile.src.x,
						(float)tile.src.y,
						(float)TILE_SIZE,
						(float)TILE_SIZE,
					};
					SDL_FRect dstrect = {
						(float)tile.dst.x,
						(float)tile.dst.y,
						(float)TILE_SIZE,
						(float)TILE_SIZE,
					};

					SDL_CHECK(SDL_RenderTexture(ctx->renderer, texture, &srcrect, &dstrect));

					++draw_calls;
				}
			}

			//SDL_Log("Draw calls: %llu", draw_calls);

			SPALL_BUFFER_END();
		}

		// DrawHitbox
		{
			SPALL_BUFFER_BEGIN_NAME("DrawHitbox");

			Entity* player = GetPlayer(ctx);
			Rect hitbox = GetEntityHitbox(ctx, player);
			SDL_CHECK(SDL_SetRenderDrawColor(ctx->renderer, 255, 0, 0, 128));
			SDL_FRect rect = {(float)hitbox.min.x, (float)hitbox.min.y, (float)(hitbox.max.x-hitbox.min.x+1), (float)(hitbox.max.y-hitbox.min.y+1)};
			SDL_CHECK(SDL_RenderFillRect(ctx->renderer, &rect));

			SPALL_BUFFER_END();
		}

		// DrawCollisions
		{
			SPALL_BUFFER_BEGIN_NAME("DrawCollisions");

			Level* level = &ctx->levels[0]; // TODO
			ivec2s dst = {0, 0};
			for (size_t i = 0; i < level->size.x*level->size.y/TILE_SIZE; ++i) {
				if (level->tiles[i]) {
					SDL_FRect rect = {
						(float)dst.x,
						(float)dst.y, 
						(float)TILE_SIZE, 
						(float)TILE_SIZE,
					};
					SDL_CHECK(SDL_RenderFillRect(ctx->renderer, &rect));
				}
				dst.x += TILE_SIZE;
				if (dst.x >= level->size.x) {
					dst.x = 0;
					dst.y += TILE_SIZE;
				}
			}

			SPALL_BUFFER_END();
		}

		// DrawEnd
		{
			SPALL_BUFFER_BEGIN_NAME("DrawEnd");

			SDL_RenderPresent(ctx->renderer);

			SPALL_BUFFER_END();
		}

		// UpdateTime
		{			
			SPALL_BUFFER_BEGIN_NAME("UpdateTime");

			SDL_Time current_time;
			SDL_CHECK(SDL_GetCurrentTime(&current_time));
			SDL_Time dt_int = current_time - ctx->time;
			const double NANOSECONDS_IN_SECOND = 1000000000.0;
			double dt_double = (double)dt_int / NANOSECONDS_IN_SECOND;

			ctx->dt_accumulator = SDL_min(ctx->dt_accumulator + dt_double, 1.0/((double)MIN_FPS)); // TODO
			
			ctx->time = current_time;

			SPALL_BUFFER_END();
		}	
	}
	
	// NOTE: If we don't do this, we might not get the last few events.
#if ENABLE_PROFILING
	spall_buffer_quit(&ctx->spall_ctx, &ctx->spall_buffer);
	spall_quit(&ctx->spall_ctx);
#endif

	// NOTE: If we don't do this, SDL might not reverse certain operations,
	// like changing the resolution of the monitor.
	SDL_Quit();

	return 0;
}