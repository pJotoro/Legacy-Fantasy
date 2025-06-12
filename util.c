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

FORCEINLINE Rect rect_from_tile(Tile tile) {
	vec2s pos = vec2_from_ivec2(tile);
	pos = glms_vec2_scale(pos, (float)TILE_SIZE);
	vec2s size = (vec2s){(float)TILE_SIZE, (float)TILE_SIZE};
	return (Rect){pos, size};
}

FORCEINLINE void tile_from_rect(Rect rect, Tile tile) {
	rect.pos = glms_vec2_floor(rect.pos);
	tile = ivec2_from_vec2(rect.pos);
}

FORCEINLINE bool tile_is_valid(Level* level, Tile tile) {
	return tile.x >= 0 && tile.x < level->w && tile.y >= 0 && tile.y < level->h;
}

FORCEINLINE bool rect_is_valid(Level* level, Rect rect) {
	return rect.pos.x >= 0.0f && rect.pos.x+rect.area.x < (level->w+1)*TILE_SIZE && rect.pos.y >= 0.0f && rect.pos.y+rect.area.y < (level->h+1)*TILE_SIZE;
}

FORCEINLINE bool rects_intersect(Level* level, Rect a, Rect b) {
	if (!rect_is_valid(level, a) || !rect_is_valid(level, b)) return false;
	return ((a.pos.x < (b.pos.x + b.area.x) && (a.pos.x + a.area.x) > b.pos.x) && (a.pos.y < (b.pos.y + b.area.y) && (a.pos.y + a.area.y) > b.pos.y));
}

