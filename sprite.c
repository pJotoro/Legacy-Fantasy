void LoadSprite(SDL_Renderer* renderer, SDL_IOStream* fs, SpriteDesc* sd) {
	ASE_Header header;
	SDL_ReadStructChecked(fs, &header);

	SDL_assert(header.magic_number == 0xA5E0);
	SDL_assert(header.color_depth == 32);
	SDL_assert((header.pixel_w == 0 || header.pixel_w == 1) && (header.pixel_h == 0 || header.pixel_h == 1));
	SDL_assert(header.grid_w == 16 && header.grid_h == 16);
	
	sd->w = (size_t)header.w;
	sd->h = (size_t)header.h;
	sd->n_frames = (size_t)header.n_frames;
	sd->frames = SDL_calloc(sd->n_frames, sizeof(SpriteFrame)); SDL_CHECK(sd->frames);

	int64_t fs_save = SDL_TellIO(fs); SDL_CHECK(fs_save != -1);
	for (size_t frame_idx = 0; frame_idx < sd->n_frames; frame_idx += 1) {
		ASE_Frame frame;
		SDL_ReadStructChecked(fs, &frame);
		SDL_assert(frame.magic_number == 0xF1FA);

		// Would mean this aseprite file is very old.
		SDL_assert(frame.n_chunks != 0);

		for (size_t chunk_idx = 0; chunk_idx < frame.n_chunks; chunk_idx += 1) {
			ASE_ChunkHeader chunk_header;
			SDL_ReadStructChecked(fs, &chunk_header);
			if (chunk_header.size == sizeof(ASE_ChunkHeader)) continue;

			void* raw_chunk = SDL_malloc(chunk_header.size - sizeof(ASE_ChunkHeader)); SDL_CHECK(raw_chunk);
			SDL_ReadIOChecked(fs, raw_chunk, chunk_header.size - sizeof(ASE_ChunkHeader));

			switch (chunk_header.type) {
			case ASE_CHUNK_TYPE_LAYER: {
				sd->n_layers += 1;
			} break;
			case ASE_CHUNK_TYPE_CELL: {
				sd->frames[frame_idx].n_cells += 1;
			} break;
			}

			SDL_free(raw_chunk);
		}
	}

	SDL_CHECK(SDL_SeekIO(fs, fs_save, SDL_IO_SEEK_SET) != -1);
	sd->layers = SDL_calloc(sd->n_layers, sizeof(SpriteLayer)); SDL_CHECK(sd->layers);
	for (size_t frame_idx = 0; frame_idx < sd->n_frames; frame_idx += 1) {
		if (sd->frames[frame_idx].n_cells > 0) {
			sd->frames[frame_idx].cells = SDL_malloc(sizeof(SpriteCell) * sd->frames[frame_idx].n_cells); SDL_CHECK(sd->frames[frame_idx].cells);
		}
	}
	size_t layer_idx = 0;
	for (size_t frame_idx = 0; frame_idx < sd->n_frames; frame_idx += 1) {
		size_t cell_idx = 0;
		ASE_Frame frame;
		SDL_ReadStructChecked(fs, &frame);
		SDL_assert(frame.magic_number == 0xF1FA);

		// Would mean this aseprite file is very old.
		SDL_assert(frame.n_chunks != 0);

		sd->frames[frame_idx].dur = (size_t)((float)frame.frame_dur / (1000.0f / 60.0f)); // TODO: delta time

		for (size_t chunk_idx = 0; chunk_idx < frame.n_chunks; chunk_idx += 1) {
			ASE_ChunkHeader chunk_header;
			SDL_ReadStructChecked(fs, &chunk_header);
			if (chunk_header.size == sizeof(ASE_ChunkHeader)) continue;

			size_t chunk_size = chunk_header.size - sizeof(ASE_ChunkHeader);
			void* raw_chunk = SDL_malloc(chunk_size); SDL_CHECK(raw_chunk);
			SDL_ReadIOChecked(fs, raw_chunk, chunk_size);

			switch (chunk_header.type) {
			case ASE_CHUNK_TYPE_OLD_PALETTE: {
			} break;
			case ASE_CHUNK_TYPE_OLD_PALETTE2: {
			} break;
			case ASE_CHUNK_TYPE_LAYER: {
				// TODO: tileset_idx and uuid
				ASE_LayerChunk* chunk = raw_chunk;
				SpriteLayer sprite_layer = {0};
				sprite_layer.name = SDL_malloc(chunk->layer_name.len + 1);
				SDL_strlcpy(sprite_layer.name, (const char*)(chunk+1), chunk->layer_name.len + 1);
				sd->layers[layer_idx] = sprite_layer; layer_idx += 1;
			} break;
			case ASE_CHUNK_TYPE_CELL: {
				ASE_CellChunk* chunk = raw_chunk;
				SpriteCell cell = {
					.layer_idx = layer_idx,
					.frame_idx = frame_idx,
					.x_offset = (ssize_t)chunk->x,
					.y_offset = (ssize_t)chunk->y,
					.z_idx = (ssize_t)chunk->z_idx,
				};
				SDL_assert(chunk->type == ASE_CELL_TYPE_COMPRESSED_IMAGE);
				switch (chunk->type) {
				case ASE_CELL_TYPE_RAW: {
				} break;
				case ASE_CELL_TYPE_LINKED: {
				} break;
				case ASE_CELL_TYPE_COMPRESSED_IMAGE: {
					cell.w = (size_t)chunk->compressed_image.w;
					cell.h = (size_t)chunk->compressed_image.h;

					// It's the zero-sized array at the end of ASE_CellChunk.
					size_t src_buf_size = chunk_size - sizeof(ASE_CellChunk) - 2; 
					void* src_buf = (void*)((&chunk->compressed_image.h)+1);

					size_t dst_buf_size = sizeof(uint32_t)*cell.w*cell.h;
					void* dst_buf = SDL_malloc(dst_buf_size); SDL_CHECK(dst_buf);

					size_t res = INFL_ZInflate(dst_buf, dst_buf_size, src_buf, src_buf_size);
					SDL_Log("%s: %llu", sd->path, res);

					SDL_Surface* surf = SDL_CreateSurfaceFrom((int32_t)cell.w, (int32_t)cell.h, SDL_PIXELFORMAT_RGBA32, dst_buf, (int32_t)(sizeof(uint32_t)*cell.w)); SDL_CHECK(surf);
					cell.texture = SDL_CreateTextureFromSurface(renderer, surf); SDL_CHECK(cell.texture);
					// SDL_DestroySurface(surf);
					// SDL_free(dst_buf);
					cell.surf = surf;
					cell.surf_backing = dst_buf;
				} break;
				case ASE_CELL_TYPE_COMPRESSED_TILEMAP: {
				} break;
				}
				sd->frames[frame_idx].cells[cell_idx] = cell; cell_idx += 1;
			} break;
			case ASE_CHUNK_TYPE_CELL_EXTRA: {
				ASE_CellChunk* chunk = raw_chunk;
				(void)chunk;
				ASE_CellExtraChunk* extra_chunk = (ASE_CellExtraChunk*)((uintptr_t)raw_chunk + (uintptr_t)chunk_header.size - sizeof(ASE_CellExtraChunk));
				(void)extra_chunk;
			} break;
			case ASE_CHUNK_TYPE_COLOR_PROFILE: {
				ASE_ColorProfileChunk* chunk = raw_chunk;
				switch (chunk->type) {
				case ASE_COLOR_PROFILE_TYPE_NONE: {

				} break;
				case ASE_COLOR_PROFILE_TYPE_SRGB: {

				} break;
				case ASE_COLOR_PROFILE_TYPE_EMBEDDED_ICC: {

				} break;
				default: {
					SDL_assert(false);
				} break;
				}
			} break;
			case ASE_CHUNK_TYPE_EXTERNAL_FILES: {
				ASE_ExternalFilesChunk* chunk = raw_chunk;
				(void)chunk;
				SDL_Log("ASE_CHUNK_TYPE_EXTERNAL_FILES");
			} break;
			case ASE_CHUNK_TYPE_DEPRECATED_MASK: {
			} break;
			case ASE_CHUNK_TYPE_PATH: {
				SDL_Log("ASE_CHUNK_TYPE_PATH");
			} break;
			case ASE_CHUNK_TYPE_TAGS: {
				ASE_TagsChunk* chunk = raw_chunk;
				ASE_Tag* tags = (ASE_Tag*)(chunk+1);
				for (size_t tag_idx = 0; tag_idx < (size_t)chunk->n_tags; tag_idx += 1) {
					ASE_Tag* tag = &tags[tag_idx]; (void)tag;
					#if 0
					char* name = SDL_malloc(tag->name.len + 1); SDL_CHECK(name);
					SDL_strlcpy(name, (char*)(tag+1), tag->name.len + 1);
					SDL_Log("tag = {.from_frame = %u, .to_frame = %u, .loop_anim_dir = %u, .repeat = %u, .name = %s}", 
						(uint32_t)tag->from_frame, (uint32_t)tag->to_frame, (uint32_t)tag->loop_anim_dir, (uint32_t)tag->repeat, name);
					SDL_free(name);
					#endif
				}
			} break;
			case ASE_CHUNK_TYPE_PALETTE: {
				ASE_PaletteChunk* chunk = raw_chunk;
				(void)chunk;
			} break;
			case ASE_CHUNK_TYPE_USER_DATA: {
				SDL_Log("ASE_CHUNK_TYPE_USER_DATA");
			} break;
			case ASE_CHUNK_TYPE_SLICE: {
				SDL_Log("ASE_CHUNK_TYPE_SLICE");
			} break;
			case ASE_CHUNK_TYPE_TILESET: {
				SDL_Log("ASE_CHUNK_TYPE_TILESET");
			} break;
			default: {
				SDL_assert(false);
			} break;
			}

			SDL_free(raw_chunk);
		}
	}
}

// void DrawSprite(SDL_Renderer* renderer, SpriteDesc* sd, size_t frame_idx, const SDL_FRect* srcrect, const SDL_FRect* dstrect) {
// 	for (size_t cell_idx = 0; cell_idx < sd->frames[frame_idx].n_cells; cell_idx += 1) {
// 		SDL_RenderTexture(renderer, sd->frames[frame_idx].cells[cell_idx].texture, srcrect, dstrect);
// 	}
// }

// void DrawSpriteSheet(SDL_Renderer* renderer, SpriteDesc* sd, vec2s pos) {
// 	const SDL_FRect srcrect = {0.0f, 0.0f, (float)(sd->w*sd->n_frames), (float)sd->h};
// 	const SDL_FRect dstrect = {pos.x, pos.y, (float)(sd->w*sd->n_frames), (float)sd->h};
// 	for (size_t frame_idx = 0; frame_idx < sd->n_frames; frame_idx += 1) {
// 		DrawSprite(renderer, sd, frame_idx, &srcrect, &dstrect);
// 	}
// }

void DrawSprite(Context* ctx, Sprite sprite, size_t frame, vec2s pos, float dir) {
	SpriteDesc* sd = GetSpriteDesc(ctx, sprite);
	SpriteFrame* sf = &sd->frames[frame];
	for (size_t cell_idx = 0; cell_idx < sf->n_cells; cell_idx += 1) {
		SpriteCell* cell = &sf->cells[cell_idx];
		const SDL_FRect srcrect = {
			0.0f,
			0.0f,
			(float)(cell->w),
			(float)(cell->h),
		};
		SDL_FRect dstrect = {
			(float)(pos.x),
			(float)(pos.y + cell->y_offset),
			(float)(cell->w),
			(float)(cell->h),
		};
		if (dir > 0.0f) {
			dstrect.x += (float)cell->x_offset;
		} else {
			dstrect.x -= (float)cell->x_offset;
			dstrect.x += (float)sd->w;
			dstrect.w = -dstrect.w;
		}
		SDL_CHECK(SDL_RenderTexture(ctx->renderer, cell->texture, &srcrect, &dstrect));
	}
}

void DrawEntity(Context* ctx, Entity* entity) {
	DrawSprite(ctx, entity->anim.sprite, entity->anim.frame_idx, entity->pos, entity->dir);
}

void DrawAnim(Context* ctx, Anim* anim, vec2s pos, float dir) {
	DrawSprite(ctx, anim->sprite, anim->frame_idx, pos, dir);
}

// This is really stupid and could be written in like two lines probably.
int32_t CompareSpriteCells(SpriteCell* a, SpriteCell* b) {
	ssize_t a_order = (ssize_t)a->layer_idx + a->z_idx;
	ssize_t b_order = (ssize_t)b->layer_idx + b->z_idx;
	if ((a_order < b_order) || ((a_order == b_order) && (a->z_idx < b->z_idx))) {
		return -1;
	} else if ((b_order < a_order) || ((b_order == a_order) && (b->z_idx < a->z_idx))) {
		return 1;
	}
	return 0;
}