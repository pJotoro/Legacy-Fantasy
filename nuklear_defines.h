#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_VARARGS // TODO: Get rid of this. SDL provides good functions for this. I could just write my own wrappers over those.
#define NK_INCLUDE_STANDARD_BOOL
#define NK_ASSERT(expr) SDL_assert(expr)