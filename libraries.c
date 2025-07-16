#pragma warning(push, 0)
#pragma warning(disable:4701)

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_stdinc.h>

#define NK_IMPLEMENTATION
#include "nuklear_defines.h"
#define NK_MEMSET SDL_memset
#define NK_MEMCPY SDL_memcpy
#define NK_SIN SDL_sinf
#define NK_COS SDL_cosf
#define NK_STRTOD SDL_strtod
#define NK_VSNPRINTF(s,n,f,a) SDL_vsnprintf(s,n,f,a)
#include <nuklear.h>

#define RADDBG_MARKUP_IMPLEMENTATION
#include <raddbg_markup.h>

#define INFL_IMPLEMENTATION
#include "infl.h"

#pragma warning(pop)