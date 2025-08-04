// Tile GetTile(Level* level, ivec2s grid_pos) {
// 	return level->tiles[grid_pos.y*level->size.x + grid_pos.x];
// }

// void SetTile(Level* level, ivec2s grid_pos, Tile tile) {
// 	// SDL_assert(grid_pos.x >= 0 && grid_pos.y >= 0 && grid_pos.x < level->size.x && grid_pos.y < level->size.y);
// 	level->tiles[grid_pos.y*level->size.x + grid_pos.x] = tile;
// }

// bool IsSolid(Level* level, ivec2s grid_pos) {
// 	Tile tile = GetTile(level, grid_pos);
// 	return tile.x != 0 && tile.y != 0;
// }

// void LoadLevel(Context* ctx) {
// 	size_t tiles_alloc_size = 1024;
// 	ctx->level.tiles = SDL_malloc(tiles_alloc_size * sizeof(Tile)); SDL_CHECK(ctx->level.tiles);
// 	size_t tile_idx = 0;

// 	{
// 		SDL_IOStream* fs = SDL_IOFromFile("level", "r");
// 		int32_t level_size_x = 0;
// 		for (;;) {
// 			uint8_t b;
// 			SDL_ReadU8(fs, &b); SDL_assert(b == '{');

// 			uint8_t str1[8];
// 			for (size_t i = 0; i < SDL_arraysize(str1); ++i) {
// 				SDL_ReadU8(fs, &str1[i]);
// 				if (str1[i] == ',') {
// 					str1[i] = '\0';
// 					break;
// 				}
// 			}
// 			uint8_t str2[8];
// 			for (size_t i = 0; i < SDL_arraysize(str2); ++i) {
// 				SDL_ReadU8(fs, &str2[i]);
// 				if (str2[i] == '}') {
// 					str2[i] = '\0';
// 					break;
// 				}
// 			}

// 			ivec2s tile;
// 			if (str1[0] == 'N') tile.x = -1;
// 			else tile.x = SDL_atoi((const char*)str1);
// 			if (str2[0] == 'N') tile.y = -1;
// 			else tile.y = SDL_atoi((const char*)str2);

// 			ctx->level.tiles[tile_idx++] = tile;
// 			if (tile_idx >= tiles_alloc_size) {
// 				tiles_alloc_size *= 2;
// 				ctx->level.tiles = SDL_realloc(ctx->level.tiles, tiles_alloc_size * sizeof(Tile)); SDL_CHECK(ctx->level.tiles);
// 			}

// 			SDL_ReadU8(fs, &b);
// 			SDL_assert(b == '\0' || b == ',' || b == '\r');
// 			if (b == '\0') break;
// 			if (b == ',') {
// 				level_size_x += 1;
// 			} else if (b == '\r') {
// 				SDL_ReadU8(fs, &b); SDL_assert(b == '\n');
// 				ctx->level.size.x = SDL_max(ctx->level.size.x, level_size_x); level_size_x = 0;
// 				ctx->level.size.y += 1;
// 			}
// 		}

// 		SDL_CloseIO(fs);
// 	}

// 	ctx->level.size = glms_ivec2_adds(ctx->level.size, 1);
// 	ctx->level.tiles = SDL_realloc(ctx->level.tiles, ctx->level.size.x * ctx->level.size.y * sizeof(Tile)); SDL_CHECK(ctx->level.tiles);
// }