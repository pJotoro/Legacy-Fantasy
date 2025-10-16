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
				};
				int32_t dst_idx = (dst.x + dst.y*res.size.x)/TILE_SIZE;
				SDL_assert(dst_idx >= 0 && dst_idx < res.size.x*res.size.y);
				res.tiles[dst_idx] = tile;
			}
		}
	}

	return res;
}

Tile* GetLevelTiles(Level* level, size_t* n_tiles) {
	SDL_assert(n_tiles);
	ivec2s size = level->size;
	*n_tiles = (size_t)(size.x * size.y);
	return level->tiles;
}