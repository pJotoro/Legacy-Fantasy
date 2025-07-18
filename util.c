FORCEINLINE vec2s vec2_from_ivec2(ivec2s v) {
	return (vec2s){(float)v.x, (float)v.y};
}

FORCEINLINE ivec2s ivec2_from_vec2(vec2s v) {
	return (ivec2s){(int)v.x, (int)v.y};
}

typedef ivec2s Tile;

FORCEINLINE Rect RectFromTile(Tile tile) {
	vec2s pos = vec2_from_ivec2(tile);
	pos = glms_vec2_scale(pos, (float)TILE_SIZE);
	vec2s size = (vec2s){(float)TILE_SIZE, (float)TILE_SIZE};
	return (Rect){pos, size};
}

FORCEINLINE void TileFromRect(Rect rect, Tile tile) {
	rect.min = glms_vec2_floor(rect.min);
	tile = ivec2_from_vec2(rect.min);
}

FORCEINLINE bool TileIsValid(Level* level, Tile tile) {
	return tile.x >= 0 && tile.x < level->w && tile.y >= 0 && tile.y < level->h;
}

FORCEINLINE bool RectIsValid(Level* level, Rect rect) {
	return rect.min.x >= 0.0f && rect.min.x+rect.max.x < (level->w+1)*TILE_SIZE && rect.min.y >= 0.0f && rect.min.y+rect.max.y < (level->h+1)*TILE_SIZE;
}

FORCEINLINE bool RectsIntersect(Level* level, Rect a, Rect b) {
	if (!RectIsValid(level, a) || !RectIsValid(level, b)) return false;
	bool d0 = b.max.x < a.min.x;
    bool d1 = a.max.x < b.min.x;
    bool d2 = b.max.y < a.min.y;
    bool d3 = a.max.y < b.min.y;
    return !(d0 | d1 | d2 | d3);
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
