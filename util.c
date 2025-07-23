FORCEINLINE vec2s vec2_from_ivec2(ivec2s v) {
	return (vec2s){(float)v.x, (float)v.y};
}

FORCEINLINE ivec2s ivec2_from_vec2(vec2s v) {
	return (ivec2s){(int)v.x, (int)v.y};
}

typedef ivec2s Tile;

FORCEINLINE Rect RectFromTile(Tile tile) {
    Rect res;
    res.min = glms_ivec2_scale(tile, TILE_SIZE);
    res.max = glms_ivec2_adds(res.min, TILE_SIZE);
	return res;
}

FORCEINLINE bool TileIsValid(Level* level, Tile tile) {
	return tile.x >= 0 && tile.x < level->w && tile.y >= 0 && tile.y < level->h;
}

FORCEINLINE bool RectIsValid(Level* level, Rect rect) {
	return rect.min.x >= 0.0f && rect.min.x+rect.max.x < (level->w+1)*TILE_SIZE && rect.min.y >= 0.0f && rect.min.y+rect.max.y < (level->h+1)*TILE_SIZE;
}

FORCEINLINE bool RectsIntersect(Rect a, Rect b) {
	bool d0 = b.max.x < a.min.x;
    bool d1 = a.max.x < b.min.x;
    bool d2 = b.max.y < a.min.y;
    bool d3 = a.max.y < b.min.y;
    return !(d0 | d1 | d2 | d3);
}

// typedef struct CollisionRes
// {
//     float depth;
//     vec2s contact_point;

//     // always points from a to b
//     vec2s n;
// } CollisionRes;

// // NOTE: This function assumes that RectsIntersectBasic(a, b) has already succeeded.
// CollisionRes RectsIntersect(Rect a, Rect b)
// {
//     CollisionRes res;

//     vec2s mid_a = glms_vec2_scale(glms_vec2_add(a.min, a.max), 0.5f);
//     vec2s mid_b = glms_vec2_scale(glms_vec2_add(b.min, b.max), 0.5f);
//     vec2s e_a = glms_vec2_normalize(glms_vec2_scale(glms_vec2_sub(a.max, a.min), 0.5f));
//     vec2s e_b = glms_vec2_normalize(glms_vec2_scale(glms_vec2_sub(b.max, b.min), 0.5f));
//     vec2s d = glms_vec2_sub(mid_b, mid_a);

//     // calc overlap on x and y axes
//     float dx = e_a.x + e_b.x - SDL_fabsf(d.x);
//     float dy = e_a.y + e_b.y - SDL_fabsf(d.y);

//     // x axis overlap is smaller
//     if (dx < dy)
//     {
//         res.depth = dx;
//         if (d.x < 0)
//         {
//             res.n = (vec2s){-1.0f, 0};
//             res.contact_point = glms_vec2_sub(mid_a, (vec2s){e_a.x, 0});
//         }
//         else
//         {
//             res.n = (vec2s){1.0f, 0};
//             res.contact_point = glms_vec2_add(mid_a, (vec2s){e_a.x, 0});
//         }
//     }

//     // y axis overlap is smaller
//     else
//     {
//         res.depth = dy;
//         if (d.y < 0)
//         {
//             res.n = (vec2s){0, -1.0f};
//             res.contact_point = glms_vec2_sub(mid_a, (vec2s){0, e_a.y});
//         }
//         else
//         {
//             res.n = (vec2s){0, 1.0f};
//             res.contact_point = glms_vec2_add(mid_a, (vec2s){0, e_a.y});
//         }
//     }

//     return res;
// }

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
