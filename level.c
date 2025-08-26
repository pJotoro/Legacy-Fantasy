Level LoadLevel(JSON_Node* level_node) {
	Level res = {0};

	JSON_Node* w = JSON_GetObjectItem(level_node, "pxWid", true);
	res.size.x = JSON_GetIntValue(w) / TILE_SIZE;

	JSON_Node* h = JSON_GetObjectItem(level_node, "pxHei", true);
	res.size.y = JSON_GetIntValue(h) / TILE_SIZE;

	JSON_Node* layer_instances = JSON_GetObjectItem(level_node, "layerInstances", true);
	JSON_Node* layer_instance; JSON_ArrayForEach(layer_instance, layer_instances) {
		JSON_Node* type_node = JSON_GetObjectItem(layer_instance, "__type", true);
		char* type = JSON_GetStringValue(type_node);
		if (SDL_strcmp(type, "Tiles") == 0) {
			JSON_Node* grid_tiles = JSON_GetObjectItem(layer_instance, "gridTiles", true);
			JSON_Node* grid_tile; JSON_ArrayForEach(grid_tile, grid_tiles) {
				res.n_entities += 1;
			}
		} else if (SDL_strcmp(type, "Entities") == 0) {
			JSON_Node* entity_instances = JSON_GetObjectItem(layer_instance, "entityInstances", true);
			JSON_Node* entity_instance; JSON_ArrayForEach(entity_instance, entity_instances) {
				res.n_entities += 1;
			}

		}
	}

	res.entities = SDL_calloc(res.n_entities, sizeof(Entity)); SDL_CHECK(res.entities);
	Entity* entity = res.entities;
	JSON_ArrayForEach(layer_instance, layer_instances) {
		JSON_Node* type_node = JSON_GetObjectItem(layer_instance, "__type", true);
		char* type = JSON_GetStringValue(type_node);
		if (SDL_strcmp(type, "Tiles") == 0) {
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

				entity->flags = EntityFlags_Tile;
				entity->pos = dst;
				entity->src_pos = src;
				++entity;
			}
		} else if (SDL_strcmp(type, "Entities") == 0) {
			JSON_Node* entity_instances = JSON_GetObjectItem(layer_instance, "entityInstances", true);

			JSON_Node* entity_instance; JSON_ArrayForEach(entity_instance, entity_instances) {
				JSON_Node* identifier_node = JSON_GetObjectItem(entity_instance, "__identifier", true);
				char* identifier = JSON_GetStringValue(identifier_node);
				JSON_Node* world_x = JSON_GetObjectItem(entity_instance, "__worldX", true);
				JSON_Node* world_y = JSON_GetObjectItem(entity_instance, "__worldY", true);
				entity->start_pos = (ivec2s){JSON_GetIntValue(world_x), JSON_GetIntValue(world_y)};
				if (SDL_strcmp(identifier, "Player") == 0) {
					entity->flags = EntityFlags_Player;
				} else if (SDL_strcmp(identifier, "Boar") == 0) {
					entity->flags = EntityFlags_Enemy|EntityFlags_Boar;
				}
				++entity;
			}
		}
	}

	return res;
}

FORCEINLINE bool RectIntersectsLevel(Level* level, Rect a, Rect* b) {
    for (size_t entity_idx = 0; entity_idx < level->n_entities; entity_idx += 1) {
        Entity* entity = &level->entities[entity_idx];
        if (HAS_FLAG(entity->flags, EntityFlags_Tile|EntityFlags_Solid)) {
            Rect tile;
            tile.min = entity->pos;
            tile.max = glms_ivec2_adds(tile.min, TILE_SIZE);
            if (RectsIntersect(a, tile)) {
                if (b) *b = tile;
                return true;
            }
        }
    }
    return false;
}
