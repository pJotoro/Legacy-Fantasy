/*
This file is basically a catch-all for convenience/wrapper functions that I almost never touch.
*/

FORCEINLINE vec2s vec2_from_ivec2(ivec2s v) {
	return (vec2s){(float)v.x, (float)v.y};
}

FORCEINLINE ivec2s ivec2_from_vec2(vec2s v) {
	return (ivec2s){(int32_t)SDL_floorf(v.x), (int32_t)SDL_floorf(v.y)};
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

size_t HashString(char* key, size_t len) {
    if (len == 0) {
        len = SDL_strlen(key);
    }
    return XXH3_64bits((const void*)key, len);
}

Entity* GetPlayer(Context* ctx) {
    return &ctx->levels[ctx->level_idx].player;
}

Entity* GetEnemies(Context* ctx, size_t* n_enemies) {
    SDL_assert(n_enemies);
    *n_enemies = ctx->levels[ctx->level_idx].n_enemies;
    return ctx->levels[ctx->level_idx].enemies;
}

Entity* GetTiles(Context* ctx, size_t* n_tiles) {
    SDL_assert(n_tiles);
    *n_tiles = ctx->levels[ctx->level_idx].n_tiles;
    return ctx->levels[ctx->level_idx].tiles;
}

Level* GetCurrentLevel(Context* ctx) {
    return &ctx->levels[ctx->level_idx];
}

void DrawEntity(Context* ctx, Entity* entity) {
    if (HAS_FLAG(entity->flags, EntityFlags_Active)) {
        DrawSprite(ctx, entity->anim.sprite, entity->anim.frame_idx, entity->pos, entity->dir);
    }
}

void DrawAnim(Context* ctx, Anim* anim, vec2s pos, int32_t dir) {
    DrawSprite(ctx, anim->sprite, anim->frame_idx, pos, dir);
}

SpriteDesc* GetSpriteDesc(Context* ctx, Sprite sprite) {
    SDL_assert(sprite.idx >= 0 && sprite.idx < MAX_SPRITES);
    return &ctx->sprites[sprite.idx];
}

bool SpritesEqual(Sprite a, Sprite b) {
    return a.idx == b.idx;
}