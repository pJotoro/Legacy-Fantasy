Level LoadLevel(Context* ctx, JSON_Node* level_node) {
	Level res = {0};

	JSON_Node* w = JSON_GetObjectItem(level_node, "pxWid", true);
	res.size.x = JSON_GetIntValue(w);

	JSON_Node* h = JSON_GetObjectItem(level_node, "pxHei", true);
	res.size.y = JSON_GetIntValue(h);

	size_t n_tiles = (size_t)(res.size.x * res.size.y);
	res.tiles = SDL_calloc(n_tiles, sizeof(Tile)); SDL_CHECK(res.tiles);

	char* layer_player = "Player";
	char* layer_enemies = "Enemies";
	char* layer_tiles = "Tiles";

	JSON_Node* layer_instances = JSON_GetObjectItem(level_node, "layerInstances", true);
	JSON_Node* layer_instance; JSON_ArrayForEach(layer_instance, layer_instances) {
		JSON_Node* ident_node = JSON_GetObjectItem(layer_instance, "__identifier", true);
		char* ident = JSON_GetStringValue(ident_node); SDL_assert(ident);

		if (SDL_strcmp(ident, layer_enemies) == 0) {
			JSON_Node* entity_instances = JSON_GetObjectItem(layer_instance, "entityInstances", true);
			JSON_Node* entity_instance; JSON_ArrayForEach(entity_instance, entity_instances) {
				++res.n_enemies;
			}
		}
	}

	res.enemies = SDL_calloc(res.n_enemies, sizeof(Entity)); SDL_CHECK(res.enemies);
	Entity* enemy = res.enemies;
	JSON_ArrayForEach(layer_instance, layer_instances) {
		JSON_Node* ident_node = JSON_GetObjectItem(layer_instance, "__identifier", true);
		char* ident = JSON_GetStringValue(ident_node); SDL_assert(ident);
		if (SDL_strcmp(ident, layer_player) == 0) {
			JSON_Node* entity_instances = JSON_GetObjectItem(layer_instance, "entityInstances", true);
			JSON_Node* entity_instance = entity_instances->child;
			JSON_Node* world_x = JSON_GetObjectItem(entity_instance, "__worldX", true);
			JSON_Node* world_y = JSON_GetObjectItem(entity_instance, "__worldY", true);
			res.player.start_pos = (ivec2s){JSON_GetIntValue(world_x), JSON_GetIntValue(world_y)};
		} else if (SDL_strcmp(ident, layer_enemies) == 0) {
			JSON_Node* entity_instances = JSON_GetObjectItem(layer_instance, "entityInstances", true);

			JSON_Node* entity_instance; JSON_ArrayForEach(entity_instance, entity_instances) {
				JSON_Node* identifier_node = JSON_GetObjectItem(entity_instance, "__identifier", true);
				char* identifier = JSON_GetStringValue(identifier_node);
				JSON_Node* world_x = JSON_GetObjectItem(entity_instance, "__worldX", true);
				JSON_Node* world_y = JSON_GetObjectItem(entity_instance, "__worldY", true);
				enemy->start_pos = (ivec2s){JSON_GetIntValue(world_x), JSON_GetIntValue(world_y)};
				if (SDL_strcmp(identifier, "Boar") == 0) {
					enemy->type = EntityType_Boar;
				} // else if (SDL_strcmp(identifier, "") == 0) {}
				++enemy;
			}
		} else if (SDL_strcmp(ident, layer_tiles) == 0) {
			JSON_Node* grid_tiles = JSON_GetObjectItem(layer_instance, "gridTiles", true);

			JSON_Node* grid_tile; JSON_ArrayForEach(grid_tile, grid_tiles) {
				JSON_Node* src_node = JSON_GetObjectItem(grid_tile, "src", true);
				ivec2s src = {
					JSON_GetIntValue(src_node->child),
					JSON_GetIntValue(src_node->child->next),
				};

				JSON_Node* dst_node = JSON_GetObjectItem(grid_tile, "px", true);
				ivec2s dst = {
					JSON_GetIntValue(dst_node->child),
					JSON_GetIntValue(dst_node->child->next),
				};

				ivec2s tileset_dimensions = GetTilesetDimensions(ctx, spr_tiles);

				Tile tile = {
					.src_idx = (uint16_t)((src.x + src.y*tileset_dimensions.x)/TILE_SIZE),
					
					// tile.type might actually be TileType_Level.
					// We check for this in LoadLevels.
					.type = TileType_Decor,
				};
				int32_t dst_idx = (dst.x + dst.y*res.size.x)/TILE_SIZE;
				SDL_assert(dst_idx >= 0 && dst_idx < res.size.x*res.size.y);
				res.tiles[dst_idx] = tile;
			}
		}
	}

	return res;
}

void LoadLevels(Context* ctx) {
	JSON_Node* head;
	{
		size_t file_len;
		void* file_data = SDL_LoadFile("assets\\levels\\test.ldtk", &file_len); SDL_CHECK(file_data);
		head = JSON_ParseWithLength((const char*)file_data, file_len);
		SDL_free(file_data);
	}
	SDL_assert(HAS_FLAG(head->type, JSON_Object));

	JSON_Node* levels = JSON_GetObjectItem(head, "levels", true);
	JSON_Node* level; JSON_ArrayForEach(level, levels) {
		++ctx->n_levels;
	}
	ctx->levels = SDL_calloc(ctx->n_levels, sizeof(Level)); SDL_CHECK(ctx->levels);
	size_t level_idx = 0;
	JSON_ArrayForEach(level, levels) {
		ctx->levels[level_idx++] = LoadLevel(ctx, level);
	}

	int32_t* tiles_collide = NULL; size_t n_tiles_collide = 0;
	JSON_Node* defs = JSON_GetObjectItem(head, "defs", true);
	JSON_Node* tilesets = JSON_GetObjectItem(defs, "tilesets", true);
	bool break_all = false;
	JSON_Node* tileset; JSON_ArrayForEach(tileset, tilesets) {
		JSON_Node* enum_tags = JSON_GetObjectItem(tileset, "enumTags", true);
		JSON_Node* enum_tag; JSON_ArrayForEach(enum_tag, enum_tags) {
			JSON_Node* id_node = JSON_GetObjectItem(enum_tag, "enumValueId", true);
			char* id = JSON_GetStringValue(id_node);
			if (SDL_strcmp(id, "Collide") != 0) continue;
			
			JSON_Node* tile_ids = JSON_GetObjectItem(enum_tag, "tileIds", true);
			JSON_Node* tile_id; JSON_ArrayForEach(tile_id, tile_ids) {
				++n_tiles_collide;
			}
			tiles_collide = SDL_calloc(n_tiles_collide, sizeof(int32_t)); SDL_CHECK(tiles_collide);
			size_t i = 0;
			JSON_ArrayForEach(tile_id, tile_ids) {
				tiles_collide[i++] = JSON_GetIntValue(tile_id);
			}
			break_all = true;
			break;
		}
		if (break_all) break;
	}
	break_all = false;

	for (size_t level_idx = 0; level_idx < ctx->n_levels; ++level_idx) {
		Level* level = &ctx->levels[level_idx];
		size_t n_tiles;
		Tile* tiles = GetLevelTiles(level, &n_tiles);
		for (size_t tile_idx = 0; tile_idx < n_tiles; ++tile_idx) {
			Tile* tile = &tiles[tile_idx];
			int32_t src_idx = (int32_t)tile->src_idx;
			for (size_t tiles_collide_idx = 0; tiles_collide_idx < n_tiles_collide; ++tiles_collide_idx) {
				int32_t i = tiles_collide[tiles_collide_idx];
				if (i == src_idx) {
					tile->type = TileType_Level;
					break;
				}
			}
		}
	}

	SDL_free(tiles_collide);
	JSON_Delete(head);
}

Tile* GetLevelTiles(Level* level, size_t* n_tiles) {
	SDL_assert(n_tiles);
	ivec2s size = level->size;
	*n_tiles = (size_t)(size.x * size.y);
	return level->tiles;
}