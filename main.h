#define FORCEINLINE SDL_FORCE_INLINE

#define KILOBYTE(X) ((X)*1024LL)
#define MEGABYTE(X) (KILOBYTE(X)*1024LL)
#define GIGABYTE(X) (MEGABYTE(X)*1024LL)
#define TERABYTE(X) (GIGABYTE(X)*1024LL)

#define STMT(X) do {X} while (false)

#define STRINGIFY(X) #X
#define CONCAT(A,B) A##B

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

#define PLAYER_FRAME_COUNT 4
#define PLAYER_FRAME_WIDTH 64
#define PLAYER_FRAME_TICK 20

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
	
	SDL_Surface* surf;
	void* surf_backing;
} SpriteCell;

typedef struct SpriteFrame {
	size_t dur;
	SpriteCell* cells; size_t n_cells;
} SpriteFrame;

typedef struct SpriteDesc {
	char* path;
	ivec2s size;
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

typedef struct Rect {
	ivec2s min;
	ivec2s max;
} Rect;

typedef struct Entity {
	vec2s pos;
	ivec2s size;
	vec2s vel;
	float dir;
	Anim anim;
	// ssize_t jump_frames;
	ssize_t touching_floor;
} Entity;

void ResetAnim(Anim* anim);

#define MAX_SPRITES 256

typedef struct Nuklear {
	struct nk_context ctx;
	struct nk_user_font font;
} Nuklear;

typedef uint8_t TileType;
enum {
	TILE_TYPE_EMPTY,
	TILE_TYPE_GROUND,
};

typedef struct Level {
	TileType* tiles;
	size_t w;
	size_t h;
	SDL_Time modify_time;
} Level;

TileType GetTile(Level* level, size_t tile_x, size_t tile_y);

void SetTile(Level* level, size_t tile_x, size_t tile_y, TileType tile);

typedef struct Context {
	SDL_Window* window;

	SDL_Renderer* renderer;
	SDL_DisplayMode* display_mode;
	bool vsync;
	float display_content_scale;

	TTF_TextEngine* text_engine;
	TTF_Font* font_roboto_regular;

	// SDL_Gamepad* gamepad;
	int32_t button_left;
	int32_t button_right;
	bool button_jump;
	bool button_attack;

	SDL_Time time;
	float dt;
	
	Entity player;
	Level level;

	Nuklear nk;

	bool running;

	SpriteDesc sprites[MAX_SPRITES];

	size_t n_collisions;
	size_t n_sprites;

	size_t sprite_tests_failed;

	bool draw_selected_anim;
	Anim selected_anim;
} Context;

void UpdatePlayer(Context* ctx);

void SetSpriteFromPath(Context* ctx, Entity* entity, const char* path);
bool SetSprite(Entity* entity, Sprite sprite);
SpriteDesc* GetSpriteDesc(Context* ctx, Sprite sprite);
void LoadSprite(SDL_Renderer* renderer, SDL_IOStream* fs, SpriteDesc* sd);
void DrawSprite(Context* ctx, Sprite sprite, size_t frame_idx, vec2s pos, float dir);
void DrawEntity(Context* ctx, Entity* entity);
void DrawAnim(Context* ctx, Anim* anim, vec2s pos, float dir);
void UpdateAnim(Context* ctx, Anim* anim, bool loop);

void ResetGame(Context* ctx);
void LoadLevel(Context* ctx);
void DrawCircle(SDL_Renderer* renderer, int32_t cx, int32_t cy, int32_t r);
void DrawCircleFilled(SDL_Renderer* renderer, int32_t cx, int32_t cy, int32_t r);

SDL_EnumerationResult EnumerateDirectoryCallback(void *userdata, const char *dirname, const char *fname);

void NK_HandleEvent(Context* ctx, SDL_Event* event);
void NK_Render(Context* ctx);
float NK_TextWidthCallback(nk_handle handle, float height, const char *text, int len);
void NK_UpdateUI(Context* ctx);
struct nk_image NK_GetImage(const SpriteDesc* sprite, size_t frame_idx);

size_t HashString(char* key, size_t len);

