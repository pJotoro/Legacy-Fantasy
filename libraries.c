#pragma warning(push, 0)
#pragma warning(disable:4701)

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_stdinc.h>

#define RADDBG_MARKUP_IMPLEMENTATION
#include <raddbg_markup.h>

#define INFL_IMPLEMENTATION
#include "infl.h"

typedef int64_t ssize_t;
#define DBL_EPSILON 2.2204460492503131e-016
#include "json.c"

#pragma warning(pop)