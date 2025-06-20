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

#define new(T) SDL_aligned_alloc(_Alignof(T), sizeof(T))
#define new_arr(T, COUNT) SDL_aligned_alloc(_Alignof(T), sizeof(T) * COUNT)
#define delete(P) SDL_free(P)

#ifdef assert
#undef assert
#endif
#define assert(COND, MSG) SDL_assert(COND && MSG)

#define arraysize(A) SDL_arraysize(A)
#define strlen(STR) SDL_strlen(STR)
#define memset(DEST, CH, COUNT) SDL_memset(DEST, CH, COUNT)
#define memcmp(LHS, RHS, COUNT) SDL_memcmp(LHS, RHS, COUNT)

#define floorf(X) SDL_floorf(X)
#define ceilf(X) SDL_ceilf(X)
#define roundf(X) SDL_roundf(X)
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#define max(A, B) SDL_max(A, B)
#define min(A, B) SDL_min(A, B)
#define signf(X) glm_signf(X)
#define clamp(X, A, B) SDL_clamp(X, A, B)

typedef int64_t ssize_t;

typedef struct Entity {
	vec2s pos;
	vec2s vel;
	int32_t w, h;
	int can_jump;
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

TileType get_tile(Level* level, size_t tile_x, size_t tile_y);
void set_tile(Level* level, size_t tile_x, size_t tile_y, TileType tile);

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

	bool running;
} Context;

void reset_game(Context* ctx);
bool load_level(Context* ctx);
void draw_circle(SDL_Renderer* renderer, int32_t cx, int32_t cy, int32_t r);
void draw_circle_filled(SDL_Renderer* renderer, int32_t cx, int32_t cy, int32_t r);

bool nk_handle_event(Context* ctx, SDL_Event* event);
bool nk_render(Context* ctx);
float nk_cb_text_width(nk_handle handle, float height, const char *text, int len);