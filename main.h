#define FORCEINLINE SDL_FORCE_INLINE

#define KILOBYTE(X) ((X)*1024LL)
#define MEGABYTE(X) (KILOBYTE(X)*1024LL)
#define GIGABYTE(X) (MEGABYTE(X)*1024LL)
#define TERABYTE(X) (GIGABYTE(X)*1024LL)

#define STMT(X) do {X} while (false)

#define STRINGIFY(X) #X
#define CONCAT(A,B) A##B

#define UNUSED(X) (void)X

#define HAS_FLAG(FLAGS, FLAG) ((FLAGS) & (FLAG))
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

typedef struct SpriteCell {
	size_t layer_idx;
	size_t frame_idx;
	ivec2s offset;
	ivec2s size;
	ssize_t z_idx;
	SDL_Texture* texture;	
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

int32_t CompareSpriteCells(SpriteCell* a, SpriteCell* b);

typedef struct Anim {
	Sprite sprite;
	size_t frame_idx;
	size_t frame_tick;
	bool ended;
} Anim;

#define PLAYER_JUMP_REMAINDER 10

enum {
	EntityFlags_Player = FLAG(0),
	EntityFlags_Enemy = FLAG(1),
	EntityFlags_Boar = FLAG(2),
	EntityFlags_Tile = FLAG(3),
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
	EntityFlags flags;
	bool jump_released;
} Entity;

void ResetAnim(Anim* anim);

#define MAX_SPRITES 256

enum {
	TileFlags_Solid = FLAG(0),
};
typedef uint32_t TileFlags;

typedef struct Tile {
	ivec2s src; // position in sprite
	ivec2s dst; // position in layer
	TileFlags flags;
} Tile;

enum {
	LevelLayerType_Tiles,
	LevelLayerType_Entities,
};
typedef uint32_t LevelLayerType;

typedef struct LevelLayer {
	LevelLayerType type;
	void* objects; size_t n_objects;
} LevelLayer;

typedef struct Level {
	ivec2s size;
	SDL_Time modify_time;
	LevelLayer* layers; size_t n_layers;
} Level;

Tile GetTile(Level* level, ivec2s grid_pos);
void SetTile(Level* level, ivec2s grid_pos, Tile tile);

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

	bool running;

	SpriteDesc sprites[MAX_SPRITES];

	size_t n_collisions;
	size_t n_sprites;

	size_t sprite_tests_failed;

	bool draw_selected_anim;
	Anim selected_anim;

	ivec2s selected_tile;
} Context;

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

void ResetGame(Context* ctx);
void DrawCircle(SDL_Renderer* renderer, ivec2s center, int32_t radius);
void DrawCircleFilled(SDL_Renderer* renderer, ivec2s center, int32_t radius);

SDL_EnumerationResult EnumerateDirectoryCallback(void *userdata, const char *dirname, const char *fname);

#if 0
void NK_HandleEvent(Context* ctx, SDL_Event* event);
void NK_Render(Context* ctx);
float NK_TextWidthCallback(nk_handle handle, float height, const char *text, int32_t len);
void NK_UpdateUI(Context* ctx);
struct nk_image NK_GetImage(const SpriteDesc* sprite, size_t frame_idx);
#endif

size_t HashString(char* key, size_t len);

