TileType GetTile(Level* level, size_t tile_x, size_t tile_y) {
	return level->tiles[tile_y*level->w + tile_x];
}

void SetTile(Level* level, size_t tile_x, size_t tile_y, TileType tile) {
	SDL_assert(tile_x < level->w && tile_y < level->h);
	level->tiles[tile_y*level->w + tile_x] = tile;
}

void LoadLevel(Context* ctx) {
	size_t file_size;
	char* file = (char*)SDL_LoadFile("level", &file_size); SDL_CHECK(file);
	ctx->level.h = 1;
	for (size_t file_i = 0, x = 0; file_i < file_size;) {
		x += 1;
		if (file[file_i] == '\r') {
			file_i += 2;
			ctx->level.w = SDL_max(x, ctx->level.w);
			x = 0;
			ctx->level.h += 1;
		} else {
			file_i += 1;
		}
	}

	ctx->level.tiles = SDL_calloc(ctx->level.w * ctx->level.h, sizeof(TileType)); SDL_CHECK(ctx->level.tiles);

	for (size_t tile_y = 0, file_i = 0; tile_y < ctx->level.h; tile_y += 1) {
		for (size_t tile_x = 0; tile_x < ctx->level.w; tile_x += 1) {
			if (file[file_i] == '\r') {
				file_i += 2;
				break;
			}
			switch (file[file_i]) {
			case '1':
				SetTile(&ctx->level, tile_x, tile_y, TILE_TYPE_GROUND);
				break;
			}
			file_i += 1;
		}
	}

	SDL_free(file);
}