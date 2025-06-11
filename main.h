#define FORCEINLINE SDL_FORCE_INLINE

#define KILOBYTE(X) ((X)*1024LL)
#define MEGABYTE(X) (KILOBYTE(X)*1024LL)
#define GIGABYTE(X) (MEGABYTE(X)*1024LL)
#define TERABYTE(X) (GIGABYTE(X)*1024LL)

#define STMT(X) do {X} while (false)

#define HAS_FLAG(FLAGS, FLAG) ((FLAGS) & (FLAG))
#define FLAG(X) (1u << X##u)

#define new(COUNT, T) SDL_aligned_alloc(_Alignof(T), sizeof(T) * COUNT)
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
	int can_jump;
} Entity;

typedef struct Nuklear {
	struct nk_context ctx;
	struct nk_user_font font;
} Nuklear;

typedef struct VarF {
	char* key;
	double value;
} VarF;

typedef struct VarI {
	char* key;
	int32_t value;
} VarI;

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
	
	Nuklear nk;

	// VarF* float_vars;
	// VarI* int_vars;
	
	bool running;
} Context;

void reset_game(Context* ctx);
void draw_circle(SDL_Renderer* renderer, int32_t cx, int32_t cy, int32_t r);
void draw_circle_filled(SDL_Renderer* renderer, int32_t cx, int32_t cy, int32_t r);

bool nk_handle_event(Context* ctx, SDL_Event* event);
bool nk_render(Context* ctx);
float nk_cb_text_width(nk_handle handle, float height, const char *text, int len);