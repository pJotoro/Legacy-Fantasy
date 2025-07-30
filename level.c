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
	bool ok = true;
	ivec2s grid_pos = {0, 0};
	while (ok) {
		uint8_t b;
		ok = SDL_ReadU8(fs, &b);
		if (b == '\r') {
			ok = SDL_ReadU8(fs, &b); SDL_assert(b == '\n');
			ctx->level.size.x = SDL_max(ctx->level.size.x, grid_pos.x);
			ctx->level.size.y += 1;
			grid_pos.x = 0;
			grid_pos.y += 1;
		} else if (b == ',') {
			ok = SDL_ReadU8(fs, &b); SDL_assert(b == '{');
		} else {
			SDL_assert(b == '{');
		}
		ok = SDL_ReadU8(fs, &b);
		if (b == 'N') {
			uint8_t data[4];
			ok = SDL_ReadIO(fs, data, 4) == 4; SDL_assert(data[0] == ',' && data[1] == 'N' && data[2] == '}');
			if (data[3] == '\r') {
				ok = SDL_ReadU8(fs, &b); SDL_assert(b == '\n');
			} else {
				SDL_assert(data[3] == ',');
			}
		} else {
			for (size_t i = 0; i < 16; ++i) {
				ok = SDL_ReadU8(fs, &b);
				if (b == ',') {
					break;
				}
			}
			for (size_t i = 0; i < 16; ++i) {
				ok = SDL_ReadU8(fs, &b);
				if (b == '}') {
					break;
				}
			}
			ok = SDL_ReadU8(fs, &b); SDL_assert(b == ',');
		}
		if (ok) {
			grid_pos.x += 1;
		}
	}

	ctx->level.tiles = SDL_calloc(ctx->level.size.x * ctx->level.size.y, sizeof(Tile)); SDL_CHECK(ctx->level.tiles);

	SDL_SeekIO(fs, 0, SDL_IO_SEEK_SET);
	ok = true;
	grid_pos = (ivec2s){0, 0};
	while (ok) {
		uint8_t b;
		ok = SDL_ReadU8(fs, &b);
		if (b == '\r') {
			ok = SDL_ReadU8(fs, &b); SDL_assert(b == '\n');
			grid_pos.x = 0;
			grid_pos.y += 1;
		} else if (b == ',') {
			ok = SDL_ReadU8(fs, &b); SDL_assert(b == '{');
		}
		ok = SDL_ReadU8(fs, &b);
		if (b == 'N') {
			uint8_t data[4];
			ok = SDL_ReadIO(fs, data, 4) == 4; SDL_assert(data[0] == ',' && data[1] == 'N' && data[2] == '}');
			if (data[3] == '\r') {
				ok = SDL_ReadU8(fs, &b); SDL_assert(b == '\n');
			} else {
				SDL_assert(data[3] == ',');
			}
			SetTile(&ctx->level, grid_pos, (Tile){-1, -1});
		} else {
			Tile tile;
			uint8_t data[8]; data[0] = b;
			for (size_t i = 1; i < SDL_arraysize(data); ++i) {
				ok = SDL_ReadU8(fs, &data[i]);
				if (data[i] == ',') {
					data[i] = '\0';
					break;
				}
			}
			tile.x = SDL_atoi((const char*)data);

			for (size_t i = 0; i < SDL_arraysize(data); ++i) {
				ok = SDL_ReadU8(fs, &data[i]);
				if (data[i] == '}') {
					data[i] = '\0';
					break;
				}
			}
			tile.y = SDL_atoi((const char*)data);

			SetTile(&ctx->level, grid_pos, tile);
		}

	}

	SDL_CloseIO(fs);

}