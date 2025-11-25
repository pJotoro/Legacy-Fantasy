function FORCEINLINE vec2s vec2_from_ivec2(ivec2s v) {
	return (vec2s){(float)v.x, (float)v.y};
}

function FORCEINLINE ivec2s ivec2_from_vec2(vec2s v) {
	return (ivec2s){(int32_t)v.x, (int32_t)v.y};
}

function FORCEINLINE vec2s glms_vec2_round(vec2s v) {
    v.x = SDL_roundf(v.x);
    v.y = SDL_roundf(v.y);
    return v;
}

function FORCEINLINE bool RectsIntersect(Rect a, Rect b) {
	bool d0 = b.max.x < a.min.x;
    bool d1 = a.max.x < b.min.x;
    bool d2 = b.max.y < a.min.y;
    bool d3 = a.max.y < b.min.y;
    return !(d0 | d1 | d2 | d3);
}

function FORCEINLINE size_t HashString(char* key, size_t len) {
    if (len == 0) {
        len = SDL_strlen(key);
    }
    return XXH3_64bits((const void*)key, len);
}

// Why not just divide by 32768.0f? It's because there is one more negative value than positive value.
function FORCEINLINE float NormInt16(int16_t i16) {
    int32_t i32 = (int32_t)i16;
    i32 += 32768;                               // 0 <= i32 <= 65535
    i32 *= 2;                                   // 0 <= i32 <= 131070
    float res = ((float)i32 / 65535.0f) - 1.0f; // -1.0f <= res <= 1.0f
    return res;
}

function FORCEINLINE bool* GetTiles(Level* level, size_t* num_tiles) {
    SDL_assert(num_tiles);
    ivec2s size = level->size;
    *num_tiles = (size_t)(size.x*size.y/TILE_SIZE);
    return level->tiles;
}

function FORCEINLINE Entity* GetPlayer(Context* ctx) {
    return &ctx->levels[ctx->level_idx].entities[0];
}

function FORCEINLINE Entity* GetEntities(Context* ctx, size_t* num_entities) {
    SDL_assert(num_entities);
    if (ctx->levels[ctx->level_idx].num_entities == 0) {
        *num_entities = 0;
        return NULL;
    } else {
        *num_entities = ctx->levels[ctx->level_idx].num_entities;
        return ctx->levels[ctx->level_idx].entities;
    }
}

function FORCEINLINE Entity* GetEnemies(Context* ctx, size_t* num_enemies) {
    SDL_assert(num_enemies);
    if (ctx->levels[ctx->level_idx].num_entities <= 1) {
        *num_enemies = 0;
        return NULL;
    } else {
        *num_enemies = ctx->levels[ctx->level_idx].num_entities - 1;
        return &ctx->levels[ctx->level_idx].entities[1];
    }
}

function FORCEINLINE Level* GetCurrentLevel(Context* ctx) {
    return &ctx->levels[ctx->level_idx];
}

function FORCEINLINE bool SpriteIsValid(Context* ctx, Sprite sprite) {
    if (!(sprite.idx >= 0 && sprite.idx < MAX_SPRITES)) return false;
    SpriteDesc* sd = &ctx->sprites[sprite.idx];
    bool size_check = sd->size.x != 0 && sd->size.y != 0;
    bool layers_check = sd->num_layers > 0 && sd->num_layers < (uint16_t)ctx->num_sprite_layers;
    bool frames_check = sd->num_frames > 0 && sd->num_frames < (uint16_t)ctx->num_sprite_frames;
    return size_check && layers_check && frames_check;
}

function FORCEINLINE SpriteDesc* GetSpriteDesc(Context* ctx, Sprite sprite) {
    if (!SpriteIsValid(ctx, sprite)) return NULL;
    return &ctx->sprites[sprite.idx];
}

function FORCEINLINE bool SpritesEqual(Sprite a, Sprite b) {
    return a.idx == b.idx;
}

function FORCEINLINE void ResetAnim(Anim* anim) {
    anim->frame_idx = 0;
    anim->dt_accumulator = 0.0;
    anim->ended = false;
}

function FORCEINLINE TileLayer* GetTileLayer(Level* level, size_t tile_layer_idx) {
	return &level->tile_layers[tile_layer_idx];
}

function FORCEINLINE Tile* GetLayerTiles(Level* level, size_t tile_layer_idx, size_t* num_tiles) {
    SDL_assert(num_tiles);
    *num_tiles = (size_t)level->size.x*level->size.y/TILE_SIZE;
    return level->tile_layers[tile_layer_idx].tiles;
}

// https://www.gingerbill.org/series/memory-allocation-strategies/

function FORCEINLINE bool IsPowerOf2(uintptr_t x) {
    return (x & (x-1)) == 0;
}

function FORCEINLINE uintptr_t AlignForward(uintptr_t ptr, uintptr_t align) {
    SDL_assert(IsPowerOf2(align));
    size_t p = ptr;
    size_t a = align;
    size_t modulo = p & (a-1);
    if (modulo != 0) {
        p += a - modulo;
    }
    return p;
}

function FORCEINLINE void* ArenaAllocRaw(Arena* arena, size_t size, size_t align) {
    // Align 'curr_offset' forward to the specified alignment
    uintptr_t curr_ptr = (uintptr_t)arena->buf + (uintptr_t)arena->curr_offset;
    uintptr_t offset = AlignForward(curr_ptr, align);
    offset -= (uintptr_t)arena->buf; // Change to relative offset

    SDL_assert(offset+size <= arena->buf_len);

    void *ptr = &arena->buf[offset];
    arena->prev_offset = offset;
    arena->curr_offset = offset+size;

    // Zero new memory by default
    SDL_memset(ptr, 0, size);
    return ptr;
}

#define ArenaAlloc(ARENA, COUNT, TYPE) (TYPE*)ArenaAllocRaw((ARENA), (COUNT)*sizeof(TYPE), alignof(TYPE))

function size_t CalcPaddingWithHeader(uintptr_t ptr, uintptr_t alignment, size_t header_size) {
    SDL_assert(IsPowerOf2(alignment));
    uintptr_t modulo = ptr & (alignment-1); // (ptr % alignment) as it assumes alignment is a power of two
    uintptr_t padding = 0;

    if (modulo != 0) { // Same logic as 'align_forward'
        padding = alignment - modulo;
    }

    uintptr_t needed_space = (uintptr_t)header_size;

    if (padding < needed_space) {
        needed_space -= padding;

        if ((needed_space & (alignment-1)) != 0) {
            padding += alignment * (1+(needed_space/alignment));
        } else {
            padding += alignment * (needed_space/alignment);
        }
    }

    return (size_t)padding;
}

function void* StackAllocRaw(Stack* stack, size_t size, size_t alignment) {
#if 1
    SDL_assert(IsPowerOf2(alignment));

    uintptr_t curr_addr = (uintptr_t)stack->buf + (uintptr_t)stack->curr_offset;
    size_t padding = CalcPaddingWithHeader(curr_addr, (uintptr_t)alignment, sizeof(StackAllocHeader));
    SDL_assert(stack->curr_offset + padding + size <= stack->buf_len);

    stack->prev_offset = stack->curr_offset; // Store the previous offset
    stack->curr_offset += padding;

    uintptr_t next_addr = curr_addr + (uintptr_t)padding;
    StackAllocHeader* header = (StackAllocHeader*)(next_addr - sizeof(StackAllocHeader));
    header->padding = padding;
    header->prev_offset = stack->prev_offset;

    stack->curr_offset += size;

    return SDL_memset((void*)next_addr, 0, size);
#else
    UNUSED(stack);
    UNUSED(alignment);
    return SDL_calloc(1, size);
#endif
}

#define StackAlloc(STACK, COUNT, TYPE) (TYPE*)StackAllocRaw(STACK, COUNT*sizeof(TYPE), alignof(TYPE))

function void StackFree(Stack* stack, void* ptr) {
#if 1
    if (ptr) {
        uintptr_t start = (uintptr_t)stack->buf;
        uintptr_t end = start + (uintptr_t)stack->buf_len;
        uintptr_t curr_addr = (uintptr_t)ptr;

        if (!(start <= curr_addr && curr_addr < end)) {
            assert(0 && "Out of bounds memory address passed to stack allocator (free)");
            return;
        }

        if (curr_addr >= start+(uintptr_t)stack->curr_offset) {
            // Allow double frees
            return;
        }

        StackAllocHeader* header = (StackAllocHeader*)(curr_addr - sizeof(StackAllocHeader));
        size_t prev_offset = (size_t)(curr_addr - (uintptr_t)header->padding - start);

        SDL_assert(prev_offset == header->prev_offset);

        stack->curr_offset = stack->prev_offset;
        stack->prev_offset = header->prev_offset;
    }
#else
    UNUSED(stack);
    SDL_free(ptr);
#endif
}

function void StackFreeAll(Stack* stack) {
    stack->curr_offset = 0;
}

function FORCEINLINE bool IsSolid(Level* level, ivec2s pos) {
    if ((!(pos.x >= 0 && pos.x*TILE_SIZE < level->size.x && pos.y >= 0 && pos.y*TILE_SIZE < level->size.y))) return false;
    size_t idx = (size_t)(pos.x + pos.y*level->size.x);
    return level->tiles[idx];
}

function FORCEINLINE bool TileIsValid(Tile tile) {
    return tile.src.x != -1 && tile.src.y != -1 && tile.dst.x != -1 && tile.dst.y != -1;
}

function FORCEINLINE bool TilesEqual(Tile a, Tile b) {
    return a.src.x == b.src.x && a.src.y == b.src.y && a.dst.x == b.dst.x && a.dst.y == b.dst.y;
}

function int32_t SDLCALL VulkanCompareImageMemoryRequirements(const VkImageMemoryRequirements* a, const VkImageMemoryRequirements* b) {
    SDL_assert(a && b);
    if (a->memoryRequirements.memoryTypeBits < b->memoryRequirements.memoryTypeBits) return -1;
    if (a->memoryRequirements.memoryTypeBits > b->memoryRequirements.memoryTypeBits) return 1;
    if (a->memoryRequirements.alignment > b->memoryRequirements.alignment) return -1;
    if (a->memoryRequirements.alignment < b->memoryRequirements.alignment) return 1;
    if (a->memoryRequirements.size > b->memoryRequirements.size) return -1;
    if (a->memoryRequirements.size < b->memoryRequirements.size) return 1;
    return -1; // this could be 1, but then the sort would be unstable
}

function int32_t SDLCALL CompareSpriteCells(const SpriteCell* a, const SpriteCell* b) {
    ssize_t a_order = (ssize_t)a->layer_idx + a->z_idx;
    ssize_t b_order = (ssize_t)b->layer_idx + b->z_idx;
    if ((a_order < b_order) || ((a_order == b_order) && (a->z_idx < b->z_idx))) {
        return -1;
    } else if ((b_order < a_order) || ((b_order == a_order) && (b->z_idx < a->z_idx))) {
        return 1;
    }
    return 0;
}