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

typedef struct Rect {
	ivec2s min;
	ivec2s max;
} Rect;

typedef struct SpriteLayer {
	char* name;
} SpriteLayer;

enum {
	SpriteCellFlags_Hitbox = FLAG(0),
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

typedef struct SpriteFrame {
	size_t dur;
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

typedef struct Sprite {
	ssize_t idx;
} Sprite;

int32_t CompareSpriteCells(const SpriteCell* a, const SpriteCell* b);

typedef struct Anim {
	Sprite sprite;
	size_t frame_idx;
	size_t frame_tick;
	bool ended;
} Anim;

#if 0
class Sprite {
private:
	ssize_t idx;
public:
	SpriteDesc* getSpriteDesc(Context* ctx);
	void Draw(Context* ctx, size_t frame_idx, ivec2s pos, int32_t dir);
	void DrawTile(Context* ctx, ivec2s tile, ivec2s ipos);
};

class Anim : public Sprite {
private:
	std::size_t frame_idx;
	std::size_t frame_tick;
	bool _ended;
public:
	void Draw(Context* ctx, ivec2s pos, int32_t dir);
	void Update(Context* ctx, bool loop);
	void Reset();
};
#endif

#define PLAYER_ACC 0.500000f
#define PLAYER_FRIC 0.300000f
#define PLAYER_MAX_VEL 3.500000f
#define PLAYER_JUMP 10.000000f
#define PLAYER_JUMP_REMAINDER 10

#define TILE_SIZE 16
#define GRAVITY 0.4f

enum {
	EntityFlags_Player = FLAG(0),
	EntityFlags_JumpReleased = FLAG(1),

	EntityFlags_Enemy = FLAG(2),
	EntityFlags_Boar = FLAG(3),

	EntityFlags_Tile = FLAG(4),
	EntityFlags_Solid = FLAG(5),
};
typedef uint32_t EntityFlags;

typedef struct Entity {
	ivec2s start_pos;
	ivec2s pos;
	vec2s pos_remainder;

	vec2s vel;
	int32_t dir;

	Anim anim;

	int32_t touching_floor;

	ivec2s src_pos;

	EntityFlags flags;
} Entity;

void ResetAnim(Anim* anim);

#define MAX_SPRITES 256

typedef struct Level {
	ivec2s size;
	SDL_Time modify_time;
	Entity* entities; size_t n_entities;
} Level;

bool IsSolid(Level* level, ivec2s grid_pos);

typedef struct Context {
	SDL_Window* window;

	SDL_Renderer* renderer;
	SDL_DisplayMode* display_mode;
	bool vsync;
	float display_content_scale;

	TTF_TextEngine* text_engine;
	TTF_Font* font_roboto_regular;

	SDL_Gamepad* gamepad;
	vec2s gamepad_left_stick;

	int32_t button_left;
	int32_t button_right;
	bool button_jump;
	bool button_jump_released;
	bool button_attack;

	bool left_mouse_pressed;
	vec2s mouse_pos;

	SDL_Time time;
	float dt;
	
	Level* levels; size_t n_levels;
	size_t level_idx;
	Entity* player; // Player is really stored in the entity array of the level. This just makes it faster to access.

	bool running;

	SpriteDesc sprites[MAX_SPRITES];

	size_t n_collisions;
	size_t n_sprites;

	size_t sprite_tests_failed;

	bool draw_selected_anim;
	Anim selected_anim;

	ivec2s selected_tile;
} Context;

Entity* GetEntities(Context* ctx, size_t* n_entities);

void UpdatePlayer(Context* ctx, Entity* player);
void UpdateBoar(Context* ctx, Entity* boar);

void SetSpriteFromPath(Entity* entity, const char* path);
bool SetSprite(Entity* entity, Sprite sprite);
SpriteDesc* GetSpriteDesc(Context* ctx, Sprite sprite);
void LoadSprite(SDL_Renderer* renderer, SDL_IOStream* fs, SpriteDesc* sd);
void DrawSprite(Context* ctx, Sprite sprite, size_t frame_idx, ivec2s pos, int32_t dir);
void DrawSpriteTile(Context* ctx, Sprite sprite, ivec2s tile, ivec2s ipos);
void DrawEntity(Context* ctx, Entity* entity);
void DrawAnim(Context* ctx, Anim* anim, ivec2s pos, int32_t dir);
void UpdateAnim(Context* ctx, Anim* anim, bool loop);
bool SpritesEqual(Sprite a, Sprite b);
bool GetSpriteHitbox(Context* ctx, Sprite sprite, size_t frame_idx, Rect* hitbox);
Rect GetEntityHitbox(Context* ctx, Entity* entity);
bool EntitiesIntersect(Context* ctx, Entity* a, Entity* b);

void ResetGame(Context* ctx);
void DrawCircle(SDL_Renderer* renderer, ivec2s center, int32_t radius);
void DrawCircleFilled(SDL_Renderer* renderer, ivec2s center, int32_t radius);

SDL_EnumerationResult EnumerateDirectoryCallback(void *userdata, const char *dirname, const char *fname);

size_t HashString(char* key, size_t len);

// NOTE: Redefine these as needed.
#define FULLSCREEN 0
#define DELTA_TIME 0