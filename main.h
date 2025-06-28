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

#define SDL_ReadStruct(FS, STRUCT) SDL_ReadIO(FS, &STRUCT, sizeof(STRUCT))

typedef int64_t ssize_t;

#define PLAYER_FRAME_COUNT 4
#define PLAYER_FRAME_WIDTH 64
#define PLAYER_FRAME_TICK 20

enum {
	ENTITY_FLAG_STATIC = FLAG(0),
	ENTITY_FLAG_SUB_PIXEL_PRECISION = FLAG(1),
};
typedef uint32_t EntityFlags;

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
	EntityFlags flags;
} Entity;

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

	SDL_Gamepad* gamepad;
	vec2s axis;

	SDL_Time time;
	float dt;
	
	Entity player;
	Level level;

	SDL_Texture* txtr_player_idle;

	Nuklear nk;

	uint32_t seed;

	bool running;
} Context;

void ResetGame(Context* ctx);
void LoadLevel(Context* ctx);
void DrawCircle(SDL_Renderer* renderer, int32_t cx, int32_t cy, int32_t r);
void DrawCircleFilled(SDL_Renderer* renderer, int32_t cx, int32_t cy, int32_t r);

void NK_HandleEvent(Context* ctx, SDL_Event* event);
void NK_Render(Context* ctx);
float NK_TextWidthCallback(nk_handle handle, float height, const char *text, int len);

uint32_t HashString(char* key, int32_t len, uint32_t seed);

typedef struct SpriteNode {
	char* key; // The same as the path, except with assets/ cut from the beginning and .aseprite cut from the end. The path doesn't have to be used again since the file has already been loaded, so this doesn't incur any cost.
	uint32_t hash;

	uint32_t w;
	uint32_t h;
	uint32_t n_frames;
	struct SpriteNode* prev;
	struct SpriteNode* next;
} SpriteNode;