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

#if 0
void DrawCircle(SDL_Renderer* renderer, ivec2s center, int32_t radius) {
	int32_t x = radius;
	int32_t y = 0;
    
    SDL_CHECK(SDL_RenderPoint(renderer, (float)(center.x + x), (float)(center.y + y)));
    SDL_CHECK(SDL_RenderPoint(renderer, (float)(center.x - x), (float)(center.y + y)));
    SDL_CHECK(SDL_RenderPoint(renderer, (float)(center.x + x), (float)(center.y - y)));
    SDL_CHECK(SDL_RenderPoint(renderer, (float)(center.x - x), (float)(center.y - y)));

    SDL_CHECK(SDL_RenderPoint(renderer, (float)(center.x + y), (float)(center.y + x)));
    SDL_CHECK(SDL_RenderPoint(renderer, (float)(center.x - y), (float)(center.y + x)));
    SDL_CHECK(SDL_RenderPoint(renderer, (float)(center.x + y), (float)(center.y - x)));
    SDL_CHECK(SDL_RenderPoint(renderer, (float)(center.x - y), (float)(center.y - x)));

    int32_t point = 1 - radius;
    while (x > y)
    { 
        ++y;
        
        if (point <= 0) {
			point = point + 2*y + 1;
        }
        else
        {
            --x;
            point = point + 2*y - 2*x + 1;
        }
        
        if (x < y) {
            break;
        }

        SDL_CHECK(SDL_RenderPoint(renderer, (float)(center.x + x), (float)(center.y + y)));
        SDL_CHECK(SDL_RenderPoint(renderer, (float)(center.x - x), (float)(center.y + y)));
        SDL_CHECK(SDL_RenderPoint(renderer, (float)(center.x + x), (float)(center.y - y)));
        SDL_CHECK(SDL_RenderPoint(renderer, (float)(center.x - x), (float)(center.y - y)));
        
        if (x != y)
        {
	        SDL_CHECK(SDL_RenderPoint(renderer, (float)(center.x + y), (float)(center.y + x)));
	        SDL_CHECK(SDL_RenderPoint(renderer, (float)(center.x - y), (float)(center.y + x)));
	        SDL_CHECK(SDL_RenderPoint(renderer, (float)(center.x + y), (float)(center.y - x)));
	        SDL_CHECK(SDL_RenderPoint(renderer, (float)(center.x - y), (float)(center.y - x)));   
        }
    }	
}

void DrawCircleFilled(SDL_Renderer* renderer, ivec2s center, int32_t radius) {
	int32_t x = radius;
	int32_t y = 0;
    
    SDL_CHECK(SDL_RenderLine(renderer, (float)center.x, (float)center.y, (float)(center.x + x), (float)(center.y + y)));
    SDL_CHECK(SDL_RenderLine(renderer, (float)center.x, (float)center.y, (float)(center.x - x), (float)(center.y + y)));
    SDL_CHECK(SDL_RenderLine(renderer, (float)center.x, (float)center.y, (float)(center.x + x), (float)(center.y - y)));
    SDL_CHECK(SDL_RenderLine(renderer, (float)center.x, (float)center.y, (float)(center.x - x), (float)(center.y - y)));

    SDL_CHECK(SDL_RenderLine(renderer, (float)center.x, (float)center.y, (float)(center.x + y), (float)(center.y + x)));
    SDL_CHECK(SDL_RenderLine(renderer, (float)center.x, (float)center.y, (float)(center.x - y), (float)(center.y + x)));
    SDL_CHECK(SDL_RenderLine(renderer, (float)center.x, (float)center.y, (float)(center.x + y), (float)(center.y - x)));
    SDL_CHECK(SDL_RenderLine(renderer, (float)center.x, (float)center.y, (float)(center.x - y), (float)(center.y - x)));

    int32_t point = 1 - radius;
    while (x > y) { 
        ++y;
        
        if (point <= 0) {
			point = point + 2*y + 1;
        } else {
            --x;
            point = point + 2*y - 2*x + 1;
        }
        
        if (x < y) {
            break;
        }

        SDL_CHECK(SDL_RenderLine(renderer, (float)center.x, (float)center.y, (float)(center.x + x), (float)(center.y + y)));
        SDL_CHECK(SDL_RenderLine(renderer, (float)center.x, (float)center.y, (float)(center.x - x), (float)(center.y + y)));
        SDL_CHECK(SDL_RenderLine(renderer, (float)center.x, (float)center.y, (float)(center.x + x), (float)(center.y - y)));
        SDL_CHECK(SDL_RenderLine(renderer, (float)center.x, (float)center.y, (float)(center.x - x), (float)(center.y - y)));
        
        if (x != y) {
	        SDL_CHECK(SDL_RenderLine(renderer, (float)center.x, (float)center.y, (float)(center.x + y), (float)(center.y + x)));
	        SDL_CHECK(SDL_RenderLine(renderer, (float)center.x, (float)center.y, (float)(center.x - y), (float)(center.y + x)));
	        SDL_CHECK(SDL_RenderLine(renderer, (float)center.x, (float)center.y, (float)(center.x + y), (float)(center.y - x)));
	        SDL_CHECK(SDL_RenderLine(renderer, (float)center.x, (float)center.y, (float)(center.x - y), (float)(center.y - x)));   
        }
    }
}
#endif

FORCEINLINE size_t HashString(char* key, size_t len) {
    if (len == 0) {
        len = SDL_strlen(key);
    }
    return XXH3_64bits((const void*)key, len);
}

// Why not just divide by 32768.0f? It's because there is one more negative value than positive value.
FORCEINLINE float NormInt16(int16_t i16) {
    int32_t i32 = (int32_t)i16;
    i32 += 32768;
    float res = ((float)i32 / 32768.0f) - 1.0f;
    return res;
}

FORCEINLINE Tile* GetLevelTiles(Level* level, size_t* n_tiles) {
    SDL_assert(n_tiles);
    ivec2s size = level->size;
    *n_tiles = (size_t)(size.x * size.y);
    return level->tiles;
}

FORCEINLINE Entity* GetPlayer(Context* ctx) {
    return &ctx->levels[ctx->level_idx].player;
}

FORCEINLINE Entity* GetEnemies(Context* ctx, size_t* n_enemies) {
    SDL_assert(n_enemies);
    *n_enemies = ctx->levels[ctx->level_idx].n_enemies;
    return ctx->levels[ctx->level_idx].enemies;
}

FORCEINLINE Tile* GetTiles(Context* ctx, size_t* n_tiles) {
    SDL_assert(n_tiles);
    ivec2s size = ctx->levels[ctx->level_idx].size;
    *n_tiles = (size_t)(size.x * size.y);
    return ctx->levels[ctx->level_idx].tiles;
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