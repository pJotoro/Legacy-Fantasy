#include <SDL3/SDL_stdinc.h>

#define STB_DS_IMPLEMENTATION
#define STBDS_NO_SHORT_NAMES
#define STBDS_SIPHASH_2_4
#define STBDS_REALLOC(context,ptr,size) SDL_realloc(ptr,size)
#define STBDS_FREE(context,ptr)         SDL_free(ptr)
#include <stb_ds.h>