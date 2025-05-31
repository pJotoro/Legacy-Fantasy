FORCEINLINE void glm_vec2_from_ivec2(ivec2 iv, vec2 v) {
	v[0] = (float)iv[0];
	v[1] = (float)iv[1];
}

FORCEINLINE void glm_ivec2_from_vec2(vec2 v, ivec2 iv) {
	iv[0] = (int)v[0];
	iv[1] = (int)v[1];
}

typedef struct Rect {
	vec2 pos;
	vec2 area;
} Rect;

FORCEINLINE Rect rect(vec2 pos, vec2 area) {
	Rect rect;
	glm_vec2_copy(pos, rect.pos);
	glm_vec2_copy(area, rect.area);
	return rect;
}

typedef ivec2 Tile;

FORCEINLINE Rect rect_from_tile(Tile tile) {
	vec2 pos;
	glm_vec2_from_ivec2(tile, pos);
	glm_vec2_scale(pos, TILE_SIZE, pos);
	vec2 size;
	glm_vec2_fill(size, TILE_SIZE);
	return rect(pos, size);
}

FORCEINLINE void tile_from_rect(Rect rect, Tile tile) {
	glm_vec2_floor(rect.pos, rect.pos);
	glm_ivec2_from_vec2(rect.pos, tile);
}

FORCEINLINE bool tile_is_valid(Tile tile) {
	return tile[0] >= 0 && tile[0] < LEVEL_WIDTH && tile[1] >= 0 && tile[1] < LEVEL_HEIGHT;
}

FORCEINLINE bool rect_is_valid(Rect rect) {
	return rect.pos[0] >= 0.0f && rect.pos[0]+rect.area[0] < (LEVEL_WIDTH+1)*TILE_SIZE && rect.pos[1] >= 0.0f && rect.pos[1]+rect.area[1] < (LEVEL_HEIGHT+1)*TILE_SIZE;
}

FORCEINLINE bool rects_intersect(Rect a, Rect b) {
	if (!rect_is_valid(a) || !rect_is_valid(b)) return false;
	return ((a.pos[0] < (b.pos[0] + b.area[0]) && (a.pos[0] + a.area[0]) > b.pos[0]) && (a.pos[1] < (b.pos[1] + b.area[1]) && (a.pos[1] + a.area[1]) > b.pos[1]));
}

