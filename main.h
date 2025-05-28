#pragma once

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