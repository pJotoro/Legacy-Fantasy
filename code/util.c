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
    return &ctx->levels[ctx->level_idx].player;
}

function FORCEINLINE Entity* GetEnemies(Context* ctx, size_t* num_enemies) {
    SDL_assert(num_enemies);
    *num_enemies = ctx->levels[ctx->level_idx].num_enemies;
    return ctx->levels[ctx->level_idx].enemies;
}

function FORCEINLINE Level* GetCurrentLevel(Context* ctx) {
    return &ctx->levels[ctx->level_idx];
}

function FORCEINLINE SpriteDesc* GetSpriteDesc(Context* ctx, Sprite sprite) {
    SDL_assert(sprite.idx >= 0 && sprite.idx < MAX_SPRITES);
    SpriteDesc* sd = &ctx->sprites[sprite.idx];
    if (!sd->path) return NULL;
    return sd;
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

function FORCEINLINE void* ArenaAllocRaw(Arena* arena, uint64_t size) {
    void* res = (void*)arena->cur;
    SDL_assert(size > 0);
    size = (size - (size%1024ULL)) + 1024ULL;
    arena->cur += size;
    SDL_assert((uint64_t)arena->cur < (uint64_t)arena->last);
    SDL_memset(res, 0, size);
    return res;
}

#define ArenaAlloc(ARENA, COUNT, TYPE) (TYPE*)ArenaAllocRaw(ARENA, COUNT*sizeof(TYPE))

// TODO: Switch away from using bool to using a bit mask.

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

// https://www.gingerbill.org/article/2019/02/08/memory-allocation-strategies-002/#aligning-a-memory-address

function FORCEINLINE bool IsPowerOf2(VkDeviceSize x) {
    return (x & (x-1)) == 0;
}

function FORCEINLINE VkDeviceSize AlignForward(VkDeviceSize ptr, VkDeviceSize align) {
    SDL_assert(IsPowerOf2(align));
    VkDeviceSize p = ptr;
    VkDeviceSize a = align;
    VkDeviceSize modulo = p & (a-1);
    if (modulo != 0) {
        p += a - modulo;
    }
    return p;
}

// TODO: Is it okay to make these static?

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

function int32_t SDLCALL CompareTileSrc(const Tile* a, const Tile* b) {
    if (a->src.y < b->src.y) return -1;
    if (a->src.y > b->src.y) return 1;
    if (a->src.x < b->src.x) return -1;
    if (a->src.x > b->src.x) return 1;
    return -1; // this could be 1, but then the sort would be unstable
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

function VkPipelineShaderStageCreateInfo VulkanCreateShaderStage(VkDevice device, const char* path, VkShaderStageFlags stage) {
    VkPipelineShaderStageCreateInfo res = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = stage,
        .pName = "main",
    };

    size_t len;
    void* data = SDL_LoadFile(path, &len); SDL_CHECK(data);

    VkShaderModuleCreateInfo info = { 
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = len,
        .pCode = (uint32_t*)data,
    };
    VK_CHECK(vkCreateShaderModule(device, &info, NULL, &res.module));

    SDL_free(data);
    return res;
}

function void GetDynamicMemProperties(SDL_IOStream* dynamic_mem, void** out_ptr, size_t* out_size) {
    SDL_assert(dynamic_mem && out_ptr && out_size);
    {
        SDL_PropertiesID props = SDL_GetIOProperties(dynamic_mem); SDL_CHECK(props);    
        *out_ptr = SDL_GetPointerProperty(props, SDL_PROP_IOSTREAM_DYNAMIC_MEMORY_POINTER, NULL);
        SDL_assert(*out_ptr); 
        SDL_DestroyProperties(props);
    }
    {
        *out_size = (size_t)SDL_TellIO(dynamic_mem);
        SDL_assert(*out_size != -1);
    }
}