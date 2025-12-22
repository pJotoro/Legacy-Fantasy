#pragma warning(push, 0)
#include <SDL.h>
#include <SDL_main.h>
#include <SDL_vulkan.h>

#include <cglm/struct.h>

#define XXH_STATIC_LINKING_ONLY /* access advanced declarations */
#define XXH_IMPLEMENTATION      /* access definitions */
#include <xxhash.h>

#include <infl.h>

#include <cJson/cJson.h>

#include <Volk/volk.h>

#ifndef _DEBUG
#define RADDBG_MARKUP_STUBS
#endif
#define RADDBG_MARKUP_IMPLEMENTATION
#include <raddbg_markup.h>

#if TOGGLE_PROFILING
#include <spall/spall.h>
#define SPALL_BUFFER_BEGIN_NAME(NAME) STMT( \
	SDL_Time time; \
	SDL_GetCurrentTime(&time); \
	spall_buffer_begin( \
		&ctx->spall_ctx, \
		&ctx->spall_buffer, \
		NAME, \
		sizeof(NAME) - 1, \
		time); \
)
#define SPALL_BUFFER_BEGIN() STMT( \
	SDL_Time time; \
	SDL_GetCurrentTime(&time); \
	spall_buffer_begin( \
		&ctx->spall_ctx, \
		&ctx->spall_buffer, \
		__FUNCTION__, \
		sizeof(__FUNCTION__) - 1, \
		time); \
)
#define SPALL_BUFFER_END() STMT( \
	SDL_Time time; \
	SDL_GetCurrentTime(&time); \
	spall_buffer_end( \
		&ctx->spall_ctx, \
		&ctx->spall_buffer, \
		time); \
)
#else
#define SPALL_BUFFER_BEGIN()
#define SPALL_BUFFER_END()
#define SPALL_BUFFER_BEGIN_NAME(NAME)
#endif
#pragma warning(pop)

typedef int64_t ssize_t;

#define STMT(X) do {X} while (false)

#define UNUSED(X) (void)X

#define HAS_FLAG(FLAGS, FLAG) ((FLAGS) & (FLAG)) // TODO: Figure out why I can't have multiple flags set in the second argument.
#define FLAG(X) (1u << X##u)

#define GetSprite(path) ((Sprite){HashString(path, 0) & (MAX_SPRITES - 1)})

#ifdef _DEBUG
#define SDL_CHECK(E) STMT( \
	if (!(E)) { \
		SDL_LogMessage(SDL_LOG_CATEGORY_ASSERT, SDL_LOG_PRIORITY_CRITICAL, "%s(%d): %s", __FILE__, __LINE__, SDL_GetError()); \
		SDL_TriggerBreakpoint(); \
	} \
)

#define VK_CHECK(E) STMT( SDL_assert((E) == VK_SUCCESS); )
#else
#define SDL_CHECK(E) E
#define VK_CHECK(E) E
#endif

#define SDL_ReadStruct(S, STRUCT) SDL_ReadIO(S, (STRUCT), sizeof(*(STRUCT)))

#define SDL_ReadIOChecked(S, P, SZ) SDL_CHECK(SDL_ReadIO(S, P, SZ) == SZ)
#define SDL_ReadStructChecked(S, STRUCT) SDL_CHECK(SDL_ReadStruct(S, STRUCT) == sizeof(*(STRUCT)))