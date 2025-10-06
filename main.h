#include <SDL.h>
#include <SDL_main.h>
#if 0
#include <SDL_ttf.h>
#endif

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

#if 0
void DrawCircle(SDL_Renderer* renderer, ivec2s center, int32_t radius);
void DrawCircleFilled(SDL_Renderer* renderer, ivec2s center, int32_t radius);
#endif

SDL_EnumerationResult EnumerateDirectoryCallback(void *userdata, const char *dirname, const char *fname);

size_t HashString(char* key, size_t len);

typedef struct Rect {
	vec2s min;
	vec2s max;
} Rect;

typedef struct SpriteLayer {
	char* name;
} SpriteLayer;

enum {
	SpriteCellFlags_Hitbox = FLAG(0),
	SpriteCellFlags_Origin = FLAG(1),
};
typedef uint32_t SpriteCellFlags;

typedef struct SpriteCell {
	size_t layer_idx;
	size_t frame_idx;
	ivec2s offset;
	ivec2s size;
	ssize_t z_idx;
	SDL_Texture* texture;
	SpriteCellFlags flags;
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
	ssize_t idx;
} Sprite;

bool SpritesEqual(Sprite a, Sprite b);

typedef struct Anim {
	Sprite sprite;
	size_t frame_idx;
	double dt_accumulator;
	int32_t timer; // As long as this timer is > 0, don't change the animation.
	bool ended;
} Anim;

void ResetAnim(Anim* anim);

enum {
	// All entities
	EntityFlags_Active = FLAG(0),

	// Player
	EntityFlags_JumpReleased = FLAG(1),

	// Enemy
	EntityFlags_Boar = FLAG(2),

	// Player, Enemy
	EntityFlags_TouchingFloor = FLAG(3),

	// Tile
	EntityFlags_Solid = FLAG(4),
};
typedef uint32_t EntityFlags;

typedef struct Entity {
	Anim anim;

	ivec2s src; // tile atlas pos
	union {
		vec2s pos; // entity pos
		ivec2s dst; // tile level pos
	};
	vec2s start_pos;
	vec2s vel;

	double coyote_time;
	int32_t dir;

	EntityFlags flags;
} Entity;

void EntityMoveX(Entity* entity, float acc);
void EntityMoveY(Entity* entity, float acc);

bool SetSpriteFromPath(Entity* entity, const char* path);
bool SetSprite(Entity* entity, Sprite sprite);

typedef struct Level {
	ivec2s size;
	SDL_Time modify_time;
	Entity player;
	Entity* enemies; size_t n_enemies;
	Entity* tiles; size_t n_tiles;
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
#if 0
	float display_content_scale;
	TTF_TextEngine* text_engine;
	TTF_Font* font_roboto_regular;
#endif

	SDL_Gamepad* gamepad;
	vec2s gamepad_left_stick;

	int32_t button_left; bool button_left_released;
	int32_t button_right;
	bool button_jump;
	bool button_jump_released;
	bool button_attack;

	bool left_mouse_pressed;
	vec2s mouse_pos;

	SDL_Time time;
	float refresh_rate;
	double dt_accumulator;
	
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
Level* GetCurrentLevel(Context* ctx);

SpriteDesc* GetSpriteDesc(Context* ctx, Sprite sprite);
ivec2s GetSpriteOrigin(Context* ctx, Sprite sprite);
bool GetSpriteHitbox(Context* ctx, Sprite sprite, size_t frame_idx, int32_t dir, Rect* hitbox);
void DrawSprite(Context* ctx, Sprite sprite, size_t frame_idx, vec2s pos, int32_t dir);
void DrawSpriteTile(Context* ctx, Sprite tileset, ivec2s src, vec2s dst);
ivec2s GetTilesetDimensions(Context* ctx, Sprite tileset);

void UpdateAnim(Context* ctx, Anim* anim, bool loop);
void DrawAnim(Context* ctx, Anim* anim, vec2s pos, int32_t dir);

Entity* GetPlayer(Context* ctx);
Entity* GetEnemies(Context* ctx, size_t* n_enemies);
Entity* GetTiles(Context* ctx, size_t* n_tiles);
Rect GetEntityHitbox(Context* ctx, Entity* entity);
void GetEntityHitboxes(Context* ctx, Entity* entity, Rect* h, Rect* lh, Rect* rh, Rect* uh, Rect* dh);
ivec2s GetEntityOrigin(Context* ctx, Entity* entity);
bool EntitiesIntersect(Context* ctx, Entity* a, Entity* b);
void DrawEntity(Context* ctx, Entity* entity);

void UpdatePlayer(Context* ctx);
void UpdateBoar(Context* ctx, Entity* boar);

bool EntityApplyFriction(Entity* entity, float fric, float max_vel);

ReplayFrame* GetReplayFrame(Context* ctx);
void SetReplayFrame(Context* ctx, size_t replay_frame_idx);