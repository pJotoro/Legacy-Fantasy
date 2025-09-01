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
