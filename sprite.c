voidpf ZAllocFunc(voidpf opaque, uInt items, uInt size) {
	(void)opaque;
	void* res = SDL_malloc((size_t)(items * size)); SDL_CHECK(res);
	return (voidpf)res;
}

void ZFreeFunc(voidpf opaque, voidpf address) {
	(void)opaque;
	SDL_free((void*)address);
}

void Inflate(SDL_IOStream* src, SDL_IOStream* dst) {
	int32_t res;
	z_stream zs = {
		.zalloc = ZAllocFunc,
		.zfree = ZFreeFunc,
	};
	res = inflateInit(&zs); SDL_assert(res == Z_OK);

	do {
		uint8_t in[16384];
		uint8_t out[16384];

		SDL_ReadIO(src, in, SDL_arraysize(in));
		zs.next_in = in;

		do {
			zs.avail_out = SDL_arraysize(out);
			zs.next_out = out;
			res = inflate(&zs, Z_NO_FLUSH); SDL_assert(res != Z_STREAM_ERROR && res != Z_NEED_DICT && res != Z_DATA_ERROR && res != Z_MEM_ERROR);

		} while(zs.avail_out == 0);

	} while (res != Z_STREAM_END);
}

void LoadSprite(Context* ctx, SDL_IOStream* fs, SpriteDesc* sd) {
	(void)ctx;

	ASE_Header header;
	SDL_ReadStructChecked(fs, &header);

	SDL_assert(header.magic_number == 0xA5E0);
	SDL_assert(header.color_depth == 32);
	SDL_assert((header.pixel_w == 0 || header.pixel_w == 1) && (header.pixel_h == 0 || header.pixel_h == 1));
	SDL_assert(header.grid_w == 16 && header.grid_h == 16);
	
	sd->w = (size_t)header.w;
	sd->h = (size_t)header.h;
	sd->n_frames = (size_t)header.n_frames;
	sd->frames = SDL_malloc(sizeof(SpriteFrame) * sd->n_frames); SDL_CHECK(sd->frames);

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
				sd->n_cells += 1;
			} break;
			}

			SDL_free(raw_chunk);
		}
	}

	SDL_CHECK(SDL_SeekIO(fs, fs_save, SDL_IO_SEEK_SET) != -1);
	sd->layers = SDL_malloc(sizeof(SpriteLayer) * sd->n_layers); SDL_CHECK(sd->layers);
	sd->cells = SDL_malloc(sizeof(SpriteCell) * sd->n_cells); SDL_CHECK(sd->cells);
	size_t layer_idx = 0;
	size_t cell_idx = 0;
	for (size_t frame_idx = 0; frame_idx < sd->n_frames; frame_idx += 1) {
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
				sd->layers[layer_idx++] = sprite_layer;
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

					ASE_Pixel* pixels = (ASE_Pixel*)((&chunk->compressed_image.h)+1);
					size_t output_buf_size = sizeof(uint32_t)*cell.w*cell.h;
					void* output_buf = SDL_malloc(output_buf_size);
					z_stream zs = {
						.next_in = (Bytef*)pixels,
						.avail_in = (uInt)(chunk_size - sizeof(ASE_CellChunk)),
						.next_out = (Bytef*)output_buf,
						.avail_out = (uInt)(output_buf_size),
						.zalloc = ZAllocFunc,
						.zfree = ZFreeFunc,
						.opaque = NULL,
					};
					int32_t zres = inflateInit(&zs); SDL_assert(zres == Z_OK);
					zres = inflate(&zs, Z_FINISH); SDL_assert(zres == Z_STREAM_END);
					zres = inflateEnd(&zs); SDL_assert(zres == Z_OK);

					SDL_Surface* surf = SDL_CreateSurfaceFrom((int32_t)cell.w, (int32_t)cell.h, SDL_PIXELFORMAT_RGBA32, (void*)pixels, (int32_t)(sizeof(uint32_t)*cell.w)); SDL_CHECK(surf);
					cell.texture = SDL_CreateTextureFromSurface(ctx->renderer, surf); SDL_CHECK(cell.texture);

				} break;
				case ASE_CELL_TYPE_COMPRESSED_TILEMAP: {
				} break;
				}
				sd->cells[cell_idx++] = cell;
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
