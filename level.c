TileType GetTile(Level* level, ivec2s tile) {
	return level->tiles[tile.y*level->size.x + tile.x];
}

void SetTile(Level* level, ivec2s tile, TileType tile_type) {
	SDL_assert(tile.x < level->size.x && tile.y < level->size.y);
	level->tiles[tile.y*level->size.x + tile.x] = tile_type;
}

void LoadLevel(Context* ctx) {
	size_t file_size;
	char* file = (char*)SDL_LoadFile("level", &file_size); SDL_CHECK(file);
	ctx->level.size.y = 1;
	for (size_t file_i = 0, x = 0; file_i < file_size;) {
		x += 1;
		if (file[file_i] == '\r') {
			file_i += 2;
			ctx->level.size.x = SDL_max((int32_t)x, ctx->level.size.x);
			x = 0;
			ctx->level.size.y += 1;
		} else {
			file_i += 1;
		}
	}

	ctx->level.tiles = SDL_calloc(ctx->level.size.x * ctx->level.size.y, sizeof(TileType)); SDL_CHECK(ctx->level.tiles);

	size_t file_i = 0;
	for (ivec2s tile = {0, 0}; tile.y < ctx->level.size.y; tile.y += 1) {
		for (tile.x = 0; tile.x < ctx->level.size.x; tile.x += 1) {
			if (file[file_i] == '\r') {
				file_i += 2;
				break;
			}
			switch (file[file_i]) {
			case '1':
				SetTile(&ctx->level, tile, TILE_TYPE_GROUND);
				break;
			}
			file_i += 1;
		}
	}

	SDL_PathInfo info;
	SDL_CHECK(SDL_GetPathInfo("level", &info));
	ctx->level.modify_time = info.modify_time;

	SDL_free(file);
}