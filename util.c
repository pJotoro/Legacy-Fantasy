FORCEINLINE vec2s vec2_from_ivec2(ivec2s v) {
	return (vec2s){(float)v.x, (float)v.y};
}

FORCEINLINE ivec2s ivec2_from_vec2(vec2s v) {
	return (ivec2s){(int32_t)v.x, (int32_t)v.y};
}

FORCEINLINE vec2s glms_vec2_round(vec2s v) {
    v.x = SDL_roundf(v.x);
    v.y = SDL_roundf(v.y);
    return v;
}

FORCEINLINE bool RectsIntersect(Rect a, Rect b) {
	bool d0 = b.max.x < a.min.x;
    bool d1 = a.max.x < b.min.x;
    bool d2 = b.max.y < a.min.y;
    bool d3 = a.max.y < b.min.y;
    return !(d0 | d1 | d2 | d3);
}

FORCEINLINE size_t HashString(char* key, size_t len) {
    if (len == 0) {
        len = SDL_strlen(key);
    }
    return XXH3_64bits((const void*)key, len);
}

// Why not just divide by 32768.0f? It's because there is one more negative value than positive value.
FORCEINLINE float NormInt16(int16_t i16) {
    int32_t i32 = (int32_t)i16;
    i32 += 32768;                               // 0 <= i32 <= 65535
    i32 *= 2;                                   // 0 <= i32 <= 131070
    float res = ((float)i32 / 65535.0f) - 1.0f; // -1.0f <= res <= 1.0f
    return res;
}

#if 0
FORCEINLINE Tile* GetLevelTiles(Level* level, size_t* n_tiles) {
    SDL_assert(n_tiles);
    ivec2s size = level->size;
    *n_tiles = (size_t)(size.x * size.y);
    return level->tiles;
}
#endif

FORCEINLINE Entity* GetPlayer(Context* ctx) {
    return &ctx->levels[ctx->level_idx].player;
}

FORCEINLINE Entity* GetEnemies(Context* ctx, size_t* n_enemies) {
    SDL_assert(n_enemies);
    *n_enemies = ctx->levels[ctx->level_idx].n_enemies;
    return ctx->levels[ctx->level_idx].enemies;
}

FORCEINLINE Level* GetCurrentLevel(Context* ctx) {
    return &ctx->levels[ctx->level_idx];
}

FORCEINLINE SpriteDesc* GetSpriteDesc(Context* ctx, Sprite sprite) {
    SDL_assert(sprite.idx >= 0 && sprite.idx < MAX_SPRITES);
    return &ctx->sprites[sprite.idx];
}

FORCEINLINE bool SpritesEqual(Sprite a, Sprite b) {
    return a.idx == b.idx;
}

FORCEINLINE void ResetAnim(Anim* anim) {
    anim->frame_idx = 0;
    anim->dt_accumulator = 0.0;
    anim->ended = false;
}

FORCEINLINE ssize_t GetTileIdx(Level* level, ivec2s pos) {
	return (ssize_t)((pos.x + pos.y*level->size.x)/TILE_SIZE);
}

FORCEINLINE TileLayer* GetTileLayer(Level* level, size_t tile_layer_idx) {
	return &level->tile_layers[tile_layer_idx];
}

FORCEINLINE void* ArenaAllocRaw(Arena* arena, uint64_t size) {
    void* res = (void*)arena->cur;
    SDL_assert(size > 0);
    size = (size - (size%1024ULL)) + 1024ULL;
    arena->cur += size;
    SDL_assert((uint64_t)arena->cur < (uint64_t)arena->last);
    SDL_memset(res, (int32_t)size, 0); // TODO
    return res;
}

#define ArenaAlloc(ARENA, COUNT, TYPE) (TYPE*)ArenaAllocRaw(ARENA, COUNT*sizeof(TYPE))

FORCEINLINE void ArenaReset(Arena* arena) {
    arena->cur = arena->first;
}

// TODO: Switch away from using bool to using a bit mask.

FORCEINLINE bool IsSolid(Level* level, ivec2s pos) {
    SDL_assert(pos.x >= 0 && pos.x < level->size.x && pos.y >= 0 && pos.y < level->size.y);
    size_t idx = (size_t)(pos.x + pos.y*level->size.x);
    return level->tiles[idx];
}