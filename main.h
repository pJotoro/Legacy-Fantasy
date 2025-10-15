#include <SDL.h>
#include <SDL_main.h>

typedef int64_t ssize_t;

#include <cglm/struct.h>

#define XXH_STATIC_LINKING_ONLY /* access advanced declarations */
#define XXH_IMPLEMENTATION      /* access definitions */
#include <xxhash.h>

#include "infl.h"

#define DBL_EPSILON 2.2204460492503131e-016
#include "json.h"

#include <raddbg_markup.h>

#include "aseprite.h"

#define FORCEINLINE SDL_FORCE_INLINE

#define KILOBYTE(X) ((X)*1024LL)
#define MEGABYTE(X) (KILOBYTE(X)*1024LL)
#define GIGABYTE(X) (MEGABYTE(X)*1024LL)
#define TERABYTE(X) (GIGABYTE(X)*1024LL)

#define STMT(X) do {X} while (false)

#define STRINGIFY(X) #X
#define CONCAT(A,B) A##B

#define UNUSED(X) (void)X

#define HAS_FLAG(FLAGS, FLAG) ((FLAGS) & (FLAG)) // TODO: Figure out why I can't have multiple flags set in the second argument.
#define FLAG(X) (1u << X##u)

#define SDL_CHECK(E) STMT( \
	if (!(E)) { \
		SDL_LogMessage(SDL_LOG_CATEGORY_ASSERT, SDL_LOG_PRIORITY_CRITICAL, "%s(%d): %s", __FILE__, __LINE__, SDL_GetError()); \
		SDL_TriggerBreakpoint(); \
	} \
)

#define SDL_ReadStruct(S, STRUCT) SDL_ReadIO(S, (STRUCT), sizeof(*(STRUCT)))

#define SDL_ReadIOChecked(S, P, SZ) SDL_CHECK(SDL_ReadIO(S, P, SZ) == SZ)
#define SDL_ReadStructChecked(S, STRUCT) SDL_CHECK(SDL_ReadStruct(S, STRUCT) == sizeof(*(STRUCT)))

#define GetSprite(path) ((Sprite){HashString(path, 0) & (MAX_SPRITES - 1)})

SDL_EnumerationResult EnumerateSpriteDirectory(void *userdata, const char *dirname, const char *fname);

size_t HashString(char* key, size_t len);

typedef struct Rect {
	ivec2s min;
	ivec2s max;
} Rect;

enum {
	TileType_Empty, // no tile
	TileType_Decor, // cannot collide with
	TileType_Level, // can collide with
};
typedef uint16_t TileType;

typedef struct Tile {
	uint16_t src_idx;
	TileType type;
} Tile;

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

int32_t CompareSpriteCells(const SpriteCell* a, const SpriteCell* b);

typedef struct SpriteFrame {
	double dur;
	SpriteCell* cells; size_t n_cells;
} SpriteFrame;

typedef struct SpriteDesc {
	char* path;
	ivec2s size;
	ivec2s grid_offset;
	ivec2s grid_size;
	Rect hitbox;
	SpriteLayer* layers; size_t n_layers;
	SpriteFrame* frames; size_t n_frames;
} SpriteDesc;

void LoadSprite(SDL_Renderer* renderer, SDL_IOStream* fs, SpriteDesc* sd);

typedef struct Sprite {
	int32_t idx;
} Sprite;

bool SpritesEqual(Sprite a, Sprite b);

typedef struct Anim {
	Sprite sprite;
	double dt_accumulator;
	uint32_t frame_idx;
	int32_t timer; // As long as this timer is > 0, don't change the animation.
	bool ended;
} Anim;

void ResetAnim(Anim* anim);

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
	ivec2s prev_pos;
	ivec2s pos;
	vec2s pos_remainder;

	vec2s vel;
	
	int32_t dir;

	EntityType type;
	EntityState state;
} Entity;

typedef struct Level {
	ivec2s size;
	Entity player;
	Entity* enemies; size_t n_enemies;
	Tile* tiles; // n_tiles is inferred from the size
} Level;

bool IsSolid(Level* level, ivec2s grid_pos);

typedef struct ReplayFrame {
	Entity player;
	Entity* enemies; size_t n_enemies;
	// TODO
} ReplayFrame;

typedef struct Context {
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
	float refresh_rate;
	
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

void ResetGame(Context* ctx);
void GetInput(Context* ctx);
void UpdateGame(Context* ctx);
void RecordReplayFrame(Context* ctx);

SpriteDesc* GetSpriteDesc(Context* ctx, Sprite sprite);
ivec2s GetSpriteOrigin(Context* ctx, Sprite sprite, int32_t dir);
bool GetSpriteHitbox(Context* ctx, Sprite sprite, size_t frame_idx, int32_t dir, Rect* hitbox);
void DrawSprite(Context* ctx, Sprite sprite, size_t frame_idx, vec2s pos, int32_t dir);
void DrawSpriteTile(Context* ctx, Sprite tileset, Tile tile, ivec2s pos);
ivec2s GetTilesetDimensions(Context* ctx, Sprite tileset);

void UpdateAnim(Context* ctx, Anim* anim, bool loop);
void DrawAnim(Context* ctx, Anim* anim, vec2s pos, int32_t dir);

Level* GetCurrentLevel(Context* ctx);
Entity* GetPlayer(Context* ctx);
Entity* GetEnemies(Context* ctx, size_t* n_enemies);
Tile* GetTiles(Context* ctx, size_t* n_tiles);
Rect GetEntityHitbox(Context* ctx, Entity* entity);
void GetEntityHitboxes(Context* ctx, Entity* entity, Rect* h, Rect* lh, Rect* rh, Rect* uh, Rect* dh);
bool EntitiesIntersect(Context* ctx, Entity* a, Entity* b);
void DrawEntity(Context* ctx, Entity* entity);

void UpdatePlayer(Context* ctx);
void UpdateBoar(Context* ctx, Entity* boar);

ReplayFrame* GetReplayFrame(Context* ctx);
void SetReplayFrame(Context* ctx, size_t replay_frame_idx);

FORCEINLINE float NormInt16(int16_t i16);

enum {
	IntersectType_None,
	IntersectType_Left,
	IntersectType_Right,
	IntersectType_Up,
	IntersectType_Down,
	IntersectType_LeftUp,
	IntersectType_LeftDown,
	IntersectType_RightUp,
	IntersectType_RightDown,
};
typedef uint32_t IntersectType;

typedef struct IntersectResult {
	/*
	0 1 2
	3   4
	5 6 7
	*/
	IntersectType collisions[8];
} IntersectResult;

IntersectResult RectIntersectsLevel(Level* level, Rect rect);

void MoveEntity(Entity* entity, vec2s acc, float fric, float max_vel);

Tile* GetLevelTiles(Level* level, size_t* n_tiles);

ivec2s GetTileSpritePos(Context* ctx, Sprite tileset, Tile tile);
