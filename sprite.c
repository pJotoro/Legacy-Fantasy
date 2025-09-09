void LoadSprite(SDL_Renderer* renderer, SDL_IOStream* fs, SpriteDesc* sd) {
	ASE_Header header;
	SDL_ReadStructChecked(fs, &header);

	SDL_assert(header.magic_number == 0xA5E0);
	SDL_assert(header.color_depth == 32);
	SDL_assert((header.pixel_w == 0 || header.pixel_w == 1) && (header.pixel_h == 0 || header.pixel_h == 1));
	
	sd->size.x = (int32_t)header.w;
	sd->size.y = (int32_t)header.h;
	sd->grid_offset.x = (int32_t)header.grid_x;
	sd->grid_offset.y = (int32_t)header.grid_y;
	sd->grid_size.x = (int32_t)header.grid_w; if (sd->grid_size.x == 0) sd->grid_size.x = 16;
	sd->grid_size.y = (int32_t)header.grid_h; if (sd->grid_size.y == 0) sd->grid_size.y = 16;
	sd->n_frames = (size_t)header.n_frames;
	sd->frames = SDL_calloc(sd->n_frames, sizeof(SpriteFrame)); SDL_CHECK(sd->frames);

	int64_t fs_save = SDL_TellIO(fs); SDL_CHECK(fs_save != -1);

	for (size_t frame_idx = 0; frame_idx < sd->n_frames; ++frame_idx) {
		ASE_Frame frame;
		SDL_ReadStructChecked(fs, &frame);
		SDL_assert(frame.magic_number == 0xF1FA);

		// Would mean this aseprite file is very old.
		SDL_assert(frame.n_chunks != 0);

		for (size_t chunk_idx = 0; chunk_idx < frame.n_chunks; ++chunk_idx) {
			ASE_ChunkHeader chunk_header;
			SDL_ReadStructChecked(fs, &chunk_header);
			if (chunk_header.size == sizeof(ASE_ChunkHeader)) continue;

			void* raw_chunk = SDL_malloc(chunk_header.size - sizeof(ASE_ChunkHeader)); SDL_CHECK(raw_chunk);
			SDL_ReadIOChecked(fs, raw_chunk, chunk_header.size - sizeof(ASE_ChunkHeader));

			switch (chunk_header.type) {
			case ASE_ChunkType_Layer: {
				++sd->n_layers;
			} break;
			case ASE_ChunkType_Cell: {
				++sd->frames[frame_idx].n_cells;
			} break;
			}

			SDL_free(raw_chunk);
		}
	}

	sd->layers = SDL_calloc(sd->n_layers, sizeof(SpriteLayer)); SDL_CHECK(sd->layers);
	for (size_t frame_idx = 0; frame_idx < sd->n_frames; ++frame_idx) {
		if (sd->frames[frame_idx].n_cells > 0) {
			sd->frames[frame_idx].cells = SDL_malloc(sizeof(SpriteCell) * sd->frames[frame_idx].n_cells); SDL_CHECK(sd->frames[frame_idx].cells);
		}
	}

	size_t layer_idx;

	SDL_CHECK(SDL_SeekIO(fs, fs_save, SDL_IO_SEEK_SET) != -1);
	layer_idx = 0;
	for (size_t frame_idx = 0; frame_idx < sd->n_frames; ++frame_idx) {
		ASE_Frame frame;
		SDL_ReadStructChecked(fs, &frame);
		SDL_assert(frame.magic_number == 0xF1FA);

		// Would mean this aseprite file is very old.
		SDL_assert(frame.n_chunks != 0);

		for (size_t chunk_idx = 0; chunk_idx < frame.n_chunks; ++chunk_idx) {
			ASE_ChunkHeader chunk_header;
			SDL_ReadStructChecked(fs, &chunk_header);
			if (chunk_header.size == sizeof(ASE_ChunkHeader)) continue;

			void* raw_chunk = SDL_malloc(chunk_header.size - sizeof(ASE_ChunkHeader)); SDL_CHECK(raw_chunk);
			SDL_ReadIOChecked(fs, raw_chunk, chunk_header.size - sizeof(ASE_ChunkHeader));

			switch (chunk_header.type) {
			case ASE_ChunkType_Layer: {
				// TODO: tileset_idx and uuid
				ASE_LayerChunk* chunk = raw_chunk;
				SpriteLayer sprite_layer = {0};
				if (chunk->layer_name.len > 0) {
					sprite_layer.name = SDL_calloc(1, chunk->layer_name.len + 1);
					SDL_strlcpy(sprite_layer.name, (const char*)(chunk+1), chunk->layer_name.len + 1);
				}
				sd->layers[layer_idx++] = sprite_layer;
			} break;
			}

			SDL_free(raw_chunk);
		}
	}

	SDL_CHECK(SDL_SeekIO(fs, fs_save, SDL_IO_SEEK_SET) != -1);
	layer_idx = 0;
	for (size_t frame_idx = 0; frame_idx < sd->n_frames; ++frame_idx) {
		size_t cell_idx = 0;
		ASE_Frame frame;
		SDL_ReadStructChecked(fs, &frame);
		SDL_assert(frame.magic_number == 0xF1FA);

		// Would mean this aseprite file is very old.
		SDL_assert(frame.n_chunks != 0);

		sd->frames[frame_idx].dur = (size_t)((float)frame.frame_dur / (1000.0f / 60.0f)); // TODO: delta time

		for (size_t chunk_idx = 0; chunk_idx < frame.n_chunks; ++chunk_idx) {
			ASE_ChunkHeader chunk_header;
			SDL_ReadStructChecked(fs, &chunk_header);
			if (chunk_header.size == sizeof(ASE_ChunkHeader)) continue;

			size_t chunk_size = chunk_header.size - sizeof(ASE_ChunkHeader);
			void* raw_chunk = SDL_malloc(chunk_size); SDL_CHECK(raw_chunk);
			SDL_ReadIOChecked(fs, raw_chunk, chunk_size);

			switch (chunk_header.type) {
			case ASE_ChunkType_OldPalette: {
			} break;
			case ASE_ChunkType_OldPalette2: {
			} break;
			case ASE_ChunkType_Cell: {
				ASE_CellChunk* chunk = raw_chunk;
				SpriteCell cell = {
					.layer_idx = layer_idx,
					.frame_idx = frame_idx,
					.offset.x = chunk->x,
					.offset.y = chunk->y,
					.z_idx = (ssize_t)chunk->z_idx,
				};
				SDL_assert(chunk->type == ASE_CellType_CompressedImage);
				switch (chunk->type) {
					case ASE_CellType_Raw: {
					} break;
					case ASE_CellType_Linked: {
					} break;
					case ASE_CellType_CompressedImage: {
						cell.size.x = chunk->compressed_image.w;
						cell.size.y = chunk->compressed_image.h;

						if (SDL_strcmp(sd->layers[chunk->layer_idx].name, "Hitbox") != 0) {
							// It's the zero-sized array at the end of ASE_CellChunk.
							size_t src_buf_size = chunk_size - sizeof(ASE_CellChunk) - 2; 
							void* src_buf = (void*)((&chunk->compressed_image.h)+1);

							size_t dst_buf_size = sizeof(uint32_t)*cell.size.x*cell.size.y;
							void* dst_buf = SDL_malloc(dst_buf_size); SDL_CHECK(dst_buf);

							size_t res = INFL_ZInflate(dst_buf, dst_buf_size, src_buf, src_buf_size); SDL_assert(res > 0);

							SDL_Surface* surf = SDL_CreateSurfaceFrom(cell.size.x, cell.size.y, SDL_PIXELFORMAT_RGBA32, dst_buf, sizeof(uint32_t)*cell.size.x); SDL_CHECK(surf);
							cell.texture = SDL_CreateTextureFromSurface(renderer, surf); SDL_CHECK(cell.texture);
							SDL_DestroySurface(surf);
							SDL_free(dst_buf);
						} else {
							cell.flags |= SpriteCellFlags_Hitbox;
						}
					} break;
					case ASE_CellType_CompressedTilemap: {
					} break;
				}
				sd->frames[frame_idx].cells[cell_idx++] = cell;
			} break;
			case ASE_ChunkType_CellExtra: {
				ASE_CellChunk* chunk = raw_chunk;
				UNUSED(chunk);
				ASE_CellExtraChunk* extra_chunk = (ASE_CellExtraChunk*)((uintptr_t)raw_chunk + (uintptr_t)chunk_header.size - sizeof(ASE_CellExtraChunk));
				UNUSED(extra_chunk);
			} break;
			case ASE_ChunkType_ColorProfile: {
				ASE_ColorProfileChunk* chunk = raw_chunk;
				switch (chunk->type) {
				case ASE_ColorProfileType_None: {

				} break;
				case ASE_ColorProfileType_SRGB: {

				} break;
				case ASE_ColorProfileType_EmbeddedICC: {

				} break;
				default: {
					SDL_assert(false);
				} break;
				}
			} break;
			case ASE_ChunkType_ExternalFiles: {
				ASE_ExternalFilesChunk* chunk = raw_chunk;
				UNUSED(chunk);
				SDL_Log("ASE_CHUNK_TYPE_EXTERNAL_FILES");
			} break;
			case ASE_ChunkType_DeprecatedMask: {
			} break;
			case ASE_ChunkType_Path: {
				SDL_Log("ASE_CHUNK_TYPE_PATH");
			} break;
			case ASE_ChunkType_Tags: {
				ASE_TagsChunk* chunk = raw_chunk;
				ASE_Tag* tags = (ASE_Tag*)(chunk+1);
				for (size_t tag_idx = 0; tag_idx < (size_t)chunk->n_tags; ++tag_idx) {
					ASE_Tag* tag = &tags[tag_idx]; UNUSED(tag);
				}
			} break;
			case ASE_ChunkType_Palette: {
				ASE_PaletteChunk* chunk = raw_chunk; // TODO: Do I have to do anything with this?
				UNUSED(chunk);
			} break;
			case ASE_ChunkType_UserData: {
				SDL_Log("ASE_CHUNK_TYPE_USER_DATA");
			} break;
			case ASE_ChunkType_Slice: {
				SDL_Log("ASE_CHUNK_TYPE_SLICE");
			} break;
			case ASE_ChunkType_Tileset: {
				SDL_Log("ASE_CHUNK_TYPE_TILESET");
			} break;
			}

			SDL_free(raw_chunk);
		}
	}
}

void DrawSprite(Context* ctx, Sprite sprite, size_t frame, vec2s pos, int32_t dir) {
	SpriteDesc* sd = GetSpriteDesc(ctx, sprite);
	SDL_assert(sd->frames && "invalid sprite");
	SpriteFrame* sf = &sd->frames[frame];
	for (size_t cell_idx = 0; cell_idx < sf->n_cells; ++cell_idx) {
		SpriteCell* cell = &sf->cells[cell_idx];
		SDL_FRect srcrect = {
			0.0f,
			0.0f,
			(float)(cell->size.x),
			(float)(cell->size.y),
		};
		SDL_FRect dstrect = {
			pos.x,
			pos.y + (float)cell->offset.y,
			(float)(cell->size.x),
			(float)(cell->size.y),
		};

		if (dir == 1) {
			dstrect.x += (float)cell->offset.x;
		} else {
			dstrect.x -= (float)cell->offset.x;
			dstrect.x += (float)sd->size.x;
			dstrect.w = -dstrect.w;
		}

		if (cell->texture) {
			SDL_CHECK(SDL_RenderTexture(ctx->renderer, cell->texture, &srcrect, &dstrect));
		}
	}
}

/* 	
I'll admit this function is kind of weird. I might end up changing it later.
The way it works is: we start from the current frame and go backward.
For each frame, check if there is a corresponding hitbox. If so, pick that one.

There is an edge case where we start at the first frame and the first frame has no hitbox.
In this case, we just go forward instead of backward, starting at the second frame.
*/
Rect GetEntityHitbox(Context* ctx, Entity* entity) {
	Rect hitbox = {0};
	bool res; ssize_t frame_idx;
	for (res = false, frame_idx = entity->anim.frame_idx; !res && frame_idx >= 0; --frame_idx) {
		res = GetSpriteHitbox(ctx, entity->anim.sprite, (size_t)frame_idx, entity->dir, &hitbox); 
	}

	if (!res && entity->anim.frame_idx == 0) {
		SpriteDesc* sd = GetSpriteDesc(ctx, entity->anim.sprite);
		for (frame_idx = 1; !res && frame_idx < (ssize_t)sd->n_frames; ++frame_idx) {
			res = GetSpriteHitbox(ctx, entity->anim.sprite, (size_t)frame_idx, entity->dir, &hitbox);
		}
	}

	SDL_assert(res);

	return hitbox;
}

void DrawSpriteTile(Context* ctx, Sprite sprite, ivec2s src, vec2s dst) {
	SpriteDesc* sd = GetSpriteDesc(ctx, sprite);
	SDL_assert(sd->n_layers == 1);
	SDL_assert(sd->n_frames == 1);
	SDL_assert(sd->frames[0].n_cells == 1);

	SDL_Texture* texture = sd->frames[0].cells[0].texture;

	SDL_FRect srcrect = {
		(float)src.x,
		(float)src.y,
		(float)sd->grid_size.x,
		(float)sd->grid_size.y,
	};
	SDL_FRect dstrect = {
		(float)dst.x,
		(float)dst.y,
		(float)(sd->grid_size.x),
		(float)(sd->grid_size.y),
	};

	SDL_CHECK(SDL_RenderTexture(ctx->renderer, texture, &srcrect, &dstrect));
}

bool GetSpriteHitbox(Context* ctx, Sprite sprite, size_t frame_idx, int32_t dir, Rect* hitbox) {
	UNUSED(dir);
	SDL_assert(hitbox);
	SpriteDesc* sd = GetSpriteDesc(ctx, sprite);
	SDL_assert(frame_idx < sd->n_frames);
	SpriteFrame* frame = &sd->frames[frame_idx];
	for (size_t cell_idx = 0; cell_idx < frame->n_cells; ++cell_idx) {
		SpriteCell* cell = &frame->cells[cell_idx];
		if (HAS_FLAG(cell->flags, SpriteCellFlags_Hitbox)) {
			#if 0
			if (dir == 1) {
				*hitbox = (Rect){cell->offset, glms_ivec2_add(cell->offset, cell->size)};
			} else {
				*hitbox = (Rect){glms_ivec2_scale(cell->offset, -1), glms_ivec2_sub(glms_ivec2_scale(cell->offset, -1), cell->size)};
			}
			#else
			*hitbox = (Rect){vec2_from_ivec2(cell->offset), vec2_from_ivec2(glms_ivec2_add(cell->offset, cell->size))};
			#endif
			return true;
		}
	}
	return false;
}

int32_t CompareSpriteCells(const SpriteCell* a, const SpriteCell* b) {
    ssize_t a_order = (ssize_t)a->layer_idx + a->z_idx;
    ssize_t b_order = (ssize_t)b->layer_idx + b->z_idx;
    if ((a_order < b_order) || ((a_order == b_order) && (a->z_idx < b->z_idx))) {
        return -1;
    } else if ((b_order < a_order) || ((b_order == a_order) && (b->z_idx < a->z_idx))) {
        return 1;
    }
    return 0;
}

void UpdateAnim(Context* ctx, Anim* anim, bool loop) {
	--anim->timer;
    SpriteDesc* sd = GetSpriteDesc(ctx, anim->sprite);
    if (loop || !anim->ended) {
        ++anim->frame_tick;
        if (anim->frame_tick >= sd->frames[anim->frame_idx].dur) {
            anim->frame_tick = 0;
            ++anim->frame_idx;
            if (anim->frame_idx >= sd->n_frames) {
                if (loop) anim->frame_idx = 0;
                else {
                    --anim->frame_idx;
                    anim->ended = true;
                }
            }
        }
    }
}

bool SetSprite(Entity* entity, Sprite sprite) {
    bool sprite_changed = false;
    if (entity->anim.sprite.idx != sprite.idx && entity->anim.timer <= 0) {
        sprite_changed = true;
        ResetAnim(&entity->anim);
        entity->anim.sprite = sprite;
    }
    return sprite_changed;
}

void ResetAnim(Anim* anim) {
    anim->frame_idx = 0;
    anim->frame_tick = 0;
    anim->ended = false;
    anim->timer = 0;
}
