Tile GetTile(Level* level, ivec2s grid_pos) {
	return level->tiles[grid_pos.y*level->size.x + grid_pos.x];
}

void SetTile(Level* level, ivec2s grid_pos, Tile tile) {
	// SDL_assert(grid_pos.x >= 0 && grid_pos.y >= 0 && grid_pos.x < level->size.x && grid_pos.y < level->size.y);
	level->tiles[grid_pos.y*level->size.x + grid_pos.x] = tile;
}

bool IsSolid(Level* level, ivec2s grid_pos) {
	Tile tile = GetTile(level, grid_pos);
	return tile.x != 0 && tile.y != 0;
}

void LoadLevel(Context* ctx) {
	SDL_IOStream* fs = SDL_IOFromFile("level", "r");
	// ivec2s grid_pos = {0, 0};
	for (;;) {
		uint8_t b;
		SDL_ReadU8(fs, &b); SDL_assert(b == '{');

		uint8_t str1[8];
		for (size_t i = 0; i < SDL_arraysize(str1); ++i) {
			SDL_ReadU8(fs, &str1[i]);
			if (str1[i] == ',') {
				str1[i] = '\0';
				break;
			}
		}
		uint8_t str2[8];
		for (size_t i = 0; i < SDL_arraysize(str2); ++i) {
			SDL_ReadU8(fs, &str2[i]);
			if (str2[i] == '}') {
				str2[i] = '\0';
				break;
			}
		}

		int32_t i1, i2;
		if (str1[0] == 'N') i1 = -1;
		else i1 = SDL_atoi((const char*)str1);
		if (str2[0] == 'N') i2 = -1;
		else i2 = SDL_atoi((const char*)str2);
		SDL_Log("{%d, %d}", i1, i2);

		SDL_ReadU8(fs, &b);
		SDL_assert(b == '\0' || b == ',' || b == '\r');
		if (b == '\0') break;
		if (b == '\r') {
			SDL_ReadU8(fs, &b); SDL_assert(b == '\n');
		}

	}
	// while (ok) {
	// 	uint8_t b;
	// 	ok = SDL_ReadU8(fs, &b);
	// 	if (b == '\r') {
	// 		ok = SDL_ReadU8(fs, &b); SDL_assert(b == '\n');
	// 		ctx->level.size.x = SDL_max(ctx->level.size.x, grid_pos.x);
	// 		ctx->level.size.y += 1;
	// 		grid_pos.x = 0;
	// 		grid_pos.y += 1;
	// 	} else if (b == ',') {
	// 		ok = SDL_ReadU8(fs, &b); SDL_assert(b == '{');
	// 	} else {
	// 		SDL_assert(b == '{');
	// 	}
	// 	ok = SDL_ReadU8(fs, &b);
	// 	if (b == 'N') {
	// 		uint8_t data[4];
	// 		ok = SDL_ReadIO(fs, data, 4) == 4; SDL_assert(data[0] == ',' && data[1] == 'N' && data[2] == '}');
	// 		if (data[3] == '\r') {
	// 			ok = SDL_ReadU8(fs, &b); SDL_assert(b == '\n');
	// 		} else {
	// 			SDL_assert(data[3] == ',');
	// 		}
	// 	} else {
	// 		for (size_t i = 0; i < 16; ++i) {
	// 			ok = SDL_ReadU8(fs, &b);
	// 			if (b == ',') {
	// 				break;
	// 			}
	// 		}
	// 		for (size_t i = 0; i < 16; ++i) {
	// 			ok = SDL_ReadU8(fs, &b);
	// 			if (b == '}') {
	// 				break;
	// 			}
	// 		}
	// 		ok = SDL_ReadU8(fs, &b); SDL_assert(b == ',');
	// 	}
	// 	if (ok) {
	// 		grid_pos.x += 1;
	// 	}
	// }

	SDL_assert(ctx->level.size.x > 0 && ctx->level.size.y > 0);
	ctx->level.tiles = SDL_calloc(ctx->level.size.x * ctx->level.size.y, sizeof(Tile)); SDL_CHECK(ctx->level.tiles);

	// SDL_SeekIO(fs, 0, SDL_IO_SEEK_SET);
	// ok = true;
	// grid_pos = (ivec2s){0, 0};
	// while (ok) {
	// 	uint8_t b;
	// 	ok = SDL_ReadU8(fs, &b);
	// 	if (b == '\r') {
	// 		ok = SDL_ReadU8(fs, &b); SDL_assert(b == '\n');
	// 		grid_pos.x = 0;
	// 		grid_pos.y += 1;
	// 	} else if (b == ',') {
	// 		ok = SDL_ReadU8(fs, &b); SDL_assert(b == '{');
	// 	}
	// 	ok = SDL_ReadU8(fs, &b);
	// 	if (b == 'N') {
	// 		uint8_t data[4];
	// 		ok = SDL_ReadIO(fs, data, 4) == 4; SDL_assert(data[0] == ',' && data[1] == 'N' && data[2] == '}');
	// 		if (data[3] == '\r') {
	// 			ok = SDL_ReadU8(fs, &b); SDL_assert(b == '\n');
	// 		} else {
	// 			SDL_assert(data[3] == ',');
	// 		}
	// 		SetTile(&ctx->level, grid_pos, (Tile){-1, -1});
	// 	} else {
	// 		Tile tile;
	// 		uint8_t data[8]; data[0] = b;
	// 		for (size_t i = 1; i < SDL_arraysize(data); ++i) {
	// 			ok = SDL_ReadU8(fs, &data[i]);
	// 			if (data[i] == ',') {
	// 				data[i] = '\0';
	// 				break;
	// 			}
	// 		}
	// 		tile.x = SDL_atoi((const char*)data);

	// 		for (size_t i = 0; i < SDL_arraysize(data); ++i) {
	// 			ok = SDL_ReadU8(fs, &data[i]);
	// 			if (data[i] == '}') {
	// 				data[i] = '\0';
	// 				break;
	// 			}
	// 		}
	// 		tile.y = SDL_atoi((const char*)data);

	// 		SetTile(&ctx->level, grid_pos, tile);
	// 	}

	// }

	SDL_CloseIO(fs);

}