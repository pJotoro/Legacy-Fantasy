Level LoadLevel(JSON_Node* level_node) {
	Level res = {0};

	JSON_Node* w = JSON_GetObjectItem(level_node, "pxWid", true);
	res.size.x = JSON_GetIntValue(w) / TILE_SIZE;

	JSON_Node* h = JSON_GetObjectItem(level_node, "pxHei", true);
	res.size.y = JSON_GetIntValue(h) / TILE_SIZE;

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
		} else if (SDL_strcmp(ident, layer_tiles) == 0) {
			JSON_Node* grid_tiles = JSON_GetObjectItem(layer_instance, "gridTiles", true);
			JSON_Node* grid_tile; JSON_ArrayForEach(grid_tile, grid_tiles) {
				++res.n_tiles;
			}
		}
	}

	res.enemies = SDL_calloc(res.n_enemies, sizeof(Entity)); SDL_CHECK(res.enemies);
	res.tiles = SDL_calloc(res.n_tiles, sizeof(Entity)); SDL_CHECK(res.tiles);
	Entity* enemy = res.enemies;
	Entity* tile = res.tiles;
	JSON_ArrayForEach(layer_instance, layer_instances) {
		JSON_Node* ident_node = JSON_GetObjectItem(layer_instance, "__identifier", true);
		char* ident = JSON_GetStringValue(ident_node); SDL_assert(ident);
		if (SDL_strcmp(ident, layer_player) == 0) {
			JSON_Node* entity_instances = JSON_GetObjectItem(layer_instance, "entityInstances", true);
			JSON_Node* entity_instance = entity_instances->child;
			JSON_Node* world_x = JSON_GetObjectItem(entity_instance, "__worldX", true);
			JSON_Node* world_y = JSON_GetObjectItem(entity_instance, "__worldY", true);
			res.player.start_pos = (vec2s){(float)JSON_GetIntValue(world_x), (float)JSON_GetIntValue(world_y)};
		} else if (SDL_strcmp(ident, layer_enemies) == 0) {
			JSON_Node* entity_instances = JSON_GetObjectItem(layer_instance, "entityInstances", true);

			JSON_Node* entity_instance; JSON_ArrayForEach(entity_instance, entity_instances) {
				JSON_Node* identifier_node = JSON_GetObjectItem(entity_instance, "__identifier", true);
				char* identifier = JSON_GetStringValue(identifier_node);
				JSON_Node* world_x = JSON_GetObjectItem(entity_instance, "__worldX", true);
				JSON_Node* world_y = JSON_GetObjectItem(entity_instance, "__worldY", true);
				enemy->start_pos = (vec2s){(float)JSON_GetIntValue(world_x), (float)JSON_GetIntValue(world_y)};
				if (SDL_strcmp(identifier, "Boar") == 0) {
					enemy->flags = EntityFlags_Boar;
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

				tile->start_pos = vec2_from_ivec2(dst);
				tile->pos = tile->start_pos; // TODO: Will these ever differ?
				tile->src = src;
				++tile;
			}
		}
	}

	return res;
}

bool RectIntersectsLevel(Level* level, Rect a, Rect* b) {
	Entity* tiles = level->tiles; size_t n_tiles = level->n_tiles;
    for (size_t tile_idx = 0; tile_idx < n_tiles; ++tile_idx) {
        Entity* tile = &tiles[tile_idx];
        if (HAS_FLAG(tile->flags, EntityFlags_Solid)) {
            Rect tile_rect;
            tile_rect.min = tile->pos;
            tile_rect.max = glms_vec2_adds(tile_rect.min, TILE_SIZE);
            if (RectsIntersect(a, tile_rect)) {
                if (b) *b = tile_rect;
                return true;
            }
        }
    }
    return false;
}
