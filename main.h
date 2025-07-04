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

typedef int64_t ssize_t;

#define PLAYER_FRAME_COUNT 4
#define PLAYER_FRAME_WIDTH 64
#define PLAYER_FRAME_TICK 20

enum {
	ENTITY_FLAG_STATIC = FLAG(0),
	ENTITY_FLAG_SUB_PIXEL_PRECISION = FLAG(1),
};
typedef uint32_t EntityFlags;

typedef size_t Sprite;

#define GetSprite(path) ((Sprite)(HashString(path, 0) & (MAX_SPRITES - 1)))

typedef struct Entity {
	union {
		struct {
			vec2s pos;
			vec2s size;
			vec2s vel;
		};
		struct {
			ivec2s ipos;
			ivec2s isize;
			ivec2s ivel;
		};
	};
	int32_t frame;
	int32_t frame_tick;
	int32_t can_jump;
	Sprite sprite;
	EntityFlags flags;
} Entity;

void ResetAnim(Entity* entity);

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

typedef struct SpriteDesc {
	char* path;
	SDL_Texture* texture;
	uint32_t w;
	uint32_t h;
	uint32_t n_frames;
	bool initialized;
} SpriteDesc;

#define MAX_SPRITES 256

typedef struct Context {
	SDL_Window* window;

	SDL_Renderer* renderer;
	SDL_DisplayMode* display_mode;
	bool vsync;
	float display_content_scale;

	TTF_TextEngine* text_engine;
	TTF_Font* font_roboto_regular;

	SDL_Gamepad* gamepad;
	vec2s axis;

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

	bool show_ui;
} Context;

void SetSprite(Context* ctx, Entity* entity, const char* path);

SpriteDesc* GetSpriteDesc(Context* ctx, Sprite sprite);

void ResetGame(Context* ctx);
void LoadLevel(Context* ctx);
void DrawCircle(SDL_Renderer* renderer, int32_t cx, int32_t cy, int32_t r);
void DrawCircleFilled(SDL_Renderer* renderer, int32_t cx, int32_t cy, int32_t r);

void LoadSprite(Context* ctx, char* path, size_t path_len);
SDL_EnumerationResult EnumerateDirectoryCallback(void *userdata, const char *dirname, const char *fname);

void NK_HandleEvent(Context* ctx, SDL_Event* event);
void NK_Render(Context* ctx);
float NK_TextWidthCallback(nk_handle handle, float height, const char *text, int len);
void NK_UpdateUI(Context* ctx);
struct nk_image NK_GetImage(const SpriteDesc* sprite, size_t frame_idx);

size_t HashString(char* key, size_t len);

