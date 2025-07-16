FORCEINLINE vec2s vec2_from_ivec2(ivec2s v) {
	return (vec2s){(float)v.x, (float)v.y};
}

FORCEINLINE ivec2s ivec2_from_vec2(vec2s v) {
	return (ivec2s){(int)v.x, (int)v.y};
}

typedef struct Rect {
	vec2s pos;
	vec2s area;
} Rect;

typedef ivec2s Tile;

FORCEINLINE Rect RectFromTile(Tile tile) {
	vec2s pos = vec2_from_ivec2(tile);
	pos = glms_vec2_scale(pos, (float)TILE_SIZE);
	vec2s size = (vec2s){(float)TILE_SIZE, (float)TILE_SIZE};
	return (Rect){pos, size};
}

FORCEINLINE void TileFromRect(Rect rect, Tile tile) {
	rect.pos = glms_vec2_floor(rect.pos);
	tile = ivec2_from_vec2(rect.pos);
}

FORCEINLINE bool TileIsValid(Level* level, Tile tile) {
	return tile.x >= 0 && tile.x < level->w && tile.y >= 0 && tile.y < level->h;
}

FORCEINLINE bool RectIsValid(Level* level, Rect rect) {
	return rect.pos.x >= 0.0f && rect.pos.x+rect.area.x < (level->w+1)*TILE_SIZE && rect.pos.y >= 0.0f && rect.pos.y+rect.area.y < (level->h+1)*TILE_SIZE;
}

FORCEINLINE bool RectsIntersect(Level* level, Rect a, Rect b) {
	if (!RectIsValid(level, a) || !RectIsValid(level, b)) return false;
	return ((a.pos.x < (b.pos.x + b.area.x) && (a.pos.x + a.area.x) > b.pos.x) && (a.pos.y < (b.pos.y + b.area.y) && (a.pos.y + a.area.y) > b.pos.y));
}

void DrawCircle(SDL_Renderer* renderer, const int32_t cx, const int32_t cy, const int32_t r) {
	int32_t x = r;
	int32_t y = 0;
    
    SDL_CHECK(SDL_RenderPoint(renderer, (float)(cx + x), (float)(cy + y)));
    SDL_CHECK(SDL_RenderPoint(renderer, (float)(cx - x), (float)(cy + y)));
    SDL_CHECK(SDL_RenderPoint(renderer, (float)(cx + x), (float)(cy - y)));
    SDL_CHECK(SDL_RenderPoint(renderer, (float)(cx - x), (float)(cy - y)));

    SDL_CHECK(SDL_RenderPoint(renderer, (float)(cx + y), (float)(cy + x)));
    SDL_CHECK(SDL_RenderPoint(renderer, (float)(cx - y), (float)(cy + x)));
    SDL_CHECK(SDL_RenderPoint(renderer, (float)(cx + y), (float)(cy - x)));
    SDL_CHECK(SDL_RenderPoint(renderer, (float)(cx - y), (float)(cy - x)));

    int32_t point = 1 - r;
    while (x > y)
    { 
        y += 1;
        
        if (point <= 0) {
			point = point + 2*y + 1;
        }
        else
        {
            x -= 1;
            point = point + 2*y - 2*x + 1;
        }
        
        if (x < y) {
            break;
        }

        SDL_CHECK(SDL_RenderPoint(renderer, (float)(cx + x), (float)(cy + y)));
        SDL_CHECK(SDL_RenderPoint(renderer, (float)(cx - x), (float)(cy + y)));
        SDL_CHECK(SDL_RenderPoint(renderer, (float)(cx + x), (float)(cy - y)));
        SDL_CHECK(SDL_RenderPoint(renderer, (float)(cx - x), (float)(cy - y)));
        
        if (x != y)
        {
	        SDL_CHECK(SDL_RenderPoint(renderer, (float)(cx + y), (float)(cy + x)));
	        SDL_CHECK(SDL_RenderPoint(renderer, (float)(cx - y), (float)(cy + x)));
	        SDL_CHECK(SDL_RenderPoint(renderer, (float)(cx + y), (float)(cy - x)));
	        SDL_CHECK(SDL_RenderPoint(renderer, (float)(cx - y), (float)(cy - x)));   
        }
    }	
}

void DrawCircleFilled(SDL_Renderer* renderer, const int32_t cx, const int32_t cy, const int32_t r) {
	int32_t x = r;
	int32_t y = 0;
    
    SDL_CHECK(SDL_RenderLine(renderer, (float)cx, (float)cy, (float)(cx + x), (float)(cy + y)));
    SDL_CHECK(SDL_RenderLine(renderer, (float)cx, (float)cy, (float)(cx - x), (float)(cy + y)));
    SDL_CHECK(SDL_RenderLine(renderer, (float)cx, (float)cy, (float)(cx + x), (float)(cy - y)));
    SDL_CHECK(SDL_RenderLine(renderer, (float)cx, (float)cy, (float)(cx - x), (float)(cy - y)));

    SDL_CHECK(SDL_RenderLine(renderer, (float)cx, (float)cy, (float)(cx + y), (float)(cy + x)));
    SDL_CHECK(SDL_RenderLine(renderer, (float)cx, (float)cy, (float)(cx - y), (float)(cy + x)));
    SDL_CHECK(SDL_RenderLine(renderer, (float)cx, (float)cy, (float)(cx + y), (float)(cy - x)));
    SDL_CHECK(SDL_RenderLine(renderer, (float)cx, (float)cy, (float)(cx - y), (float)(cy - x)));

    int32_t point = 1 - r;
    while (x > y) { 
        y += 1;
        
        if (point <= 0) {
			point = point + 2*y + 1;
        } else {
            x -= 1;
            point = point + 2*y - 2*x + 1;
        }
        
        if (x < y) {
            break;
        }

        SDL_CHECK(SDL_RenderLine(renderer, (float)cx, (float)cy, (float)(cx + x), (float)(cy + y)));
        SDL_CHECK(SDL_RenderLine(renderer, (float)cx, (float)cy, (float)(cx - x), (float)(cy + y)));
        SDL_CHECK(SDL_RenderLine(renderer, (float)cx, (float)cy, (float)(cx + x), (float)(cy - y)));
        SDL_CHECK(SDL_RenderLine(renderer, (float)cx, (float)cy, (float)(cx - x), (float)(cy - y)));
        
        if (x != y) {
	        SDL_CHECK(SDL_RenderLine(renderer, (float)cx, (float)cy, (float)(cx + y), (float)(cy + x)));
	        SDL_CHECK(SDL_RenderLine(renderer, (float)cx, (float)cy, (float)(cx - y), (float)(cy + x)));
	        SDL_CHECK(SDL_RenderLine(renderer, (float)cx, (float)cy, (float)(cx + y), (float)(cy - x)));
	        SDL_CHECK(SDL_RenderLine(renderer, (float)cx, (float)cy, (float)(cx - y), (float)(cy - x)));   
        }
    }	
}

void UpdateAnim(Context* ctx, Anim* anim) {
    SpriteDesc* sd = GetSpriteDesc(ctx, anim->sprite);
    anim->frame_tick += 1;
    if (anim->frame_tick >= sd->frames[anim->frame_idx].dur) {
        anim->frame_tick = 0;
        anim->frame_idx += 1;
        if (anim->frame_idx >= sd->n_frames) {
            anim->frame_idx = 0;
        }
    }
}
