Tile GetTile(Level* level, ivec2s grid_pos) {
	return level->tiles[grid_pos.y*level->size.x + grid_pos.x];
}

void SetTile(Level* level, ivec2s grid_pos, Tile tile) {
	SDL_assert(grid_pos.x >= 0 && grid_pos.y >= 0 && grid_pos.x < level->size.x && grid_pos.y < level->size.y);
	level->tiles[grid_pos.y*level->size.x + grid_pos.x] = tile;
}

bool IsSolid(Level* level, ivec2s grid_pos) {
	Tile tile = GetTile(level, grid_pos);
	return tile.x != 0 && tile.y != 0;
}

void LoadLevel(Context* ctx) {
	ctx->level.size.x = 25;
	ctx->level.size.y = 25;
	ctx->level.tiles = SDL_calloc(ctx->level.size.x * ctx->level.size.y, sizeof(Tile)); SDL_CHECK(ctx->level.tiles);

	// size_t file_size;
	// char* file = (char*)SDL_LoadFile("level", &file_size); SDL_CHECK(file);
	// ctx->level.size.y = 1;
	// for (size_t file_i = 0, x = 0; file_i < file_size;) {
	// 	x += 1;
	// 	file_i += 2;
	// 	if (file[file_i] == '\r') {
	// 		ctx->level.size.x = SDL_max((int32_t)x, ctx->level.size.x);
	// 		x = 0;
	// 		ctx->level.size.y += 1;
	// 	}
	// }

	// ctx->level.tiles = SDL_calloc(ctx->level.size.x * ctx->level.size.y, sizeof(Tile)); SDL_CHECK(ctx->level.tiles);

	// size_t file_i = 0;
	// for (ivec2s tile = {0, 0}; tile.y < ctx->level.size.y; tile.y += 1) {
	// 	for (tile.x = 0; tile.x < ctx->level.size.x; tile.x += 1) {
	// 		if (file[file_i] == '\r') {
	// 			file_i += 2;
	// 			break;
	// 		}
	// 		switch (file[file_i]) {
	// 		case '1':
	// 			SetTile(&ctx->level, tile, TILE_TYPE_GROUND);
	// 			break;
	// 		}
	// 		file_i += 1;
	// 	}
	// }

	// SDL_PathInfo info;
	// SDL_CHECK(SDL_GetPathInfo("level", &info));
	// ctx->level.modify_time = info.modify_time;

	// SDL_free(file);
// #else
// 	ctx->level.tiles = SDL_calloc(ctx->level.size.x * ctx->level.size.y, sizeof(Tile)); SDL_CHECK(ctx->level.tiles);

// 	static Sprite spr_tiles;
// 	static bool initialized_sprites = false;
// 	if (!initialized_sprites) {
// 		initialized_sprites = true;
// 		spr_tiles = GetSprite("assets\\legacy_fantasy_high_forest\\Assets\\Tiles.aseprite");
// 	}
// 	SpriteDesc* sprdesc_tiles = GetSpriteDesc(ctx, spr_tiles);
// 	SDL_Surface* surf = sprdesc_tiles->frames[0].cells[0].surf;

// 	bool break_all = false;
// 	uint32_t zeros = {0};
// 	for (ivec2s tile = {0, 0}; tile.y < 25 && !break_all; ++tile.y) {
// 		for (tile.x = 0; tile.x < 25 && !break_all; ++tile.x) {
// 			uint32_t* pixels = (uint32_t*)surf->pixels;
// 			uint32_t* tile_start = pixels + 25*tile.y + tile.x;
// 			for (size_t row = 0; row < 16; ++row) {
// 				uint32_t* tile_cur = tile_start + 25*row;
// 				if (SDL_memcmp(tile_cur, zeros, sizeof(uint32_t)*16) == 0) {

// 				}
// 			}
// 		}
// 	}
// #endif
}