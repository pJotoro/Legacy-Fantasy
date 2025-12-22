#define TOGGLE_PROFILING 1

#include "main.h"
#include "aseprite.h"

#define TOGGLE_FULLSCREEN 1
#define TOGGLE_REPLAY_FRAMES 1
#define TOGGLE_TESTS 0
#define TOGGLE_VULKAN_VALIDATION 1

#define GAMEPAD_THRESHOLD 0.5f

#define PLAYER_ACC 0.5f
#define PLAYER_FRIC 0.1f
#define PLAYER_MAX_VEL 2.5f
#define PLAYER_JUMP 5.0f

#define BOAR_ACC 0.01f
#define BOAR_FRIC 0.005f
#define BOAR_MAX_VEL 0.15f

#define TILE_SIZE 16
#define GRAVITY 0.2f

#define MAX_SPRITES 256

typedef struct Rect 
{
	ivec2s min;
	ivec2s max;
} Rect;

typedef struct SpriteCell 
{
	void* dst_buf; // invalid after VulkanCreateStaticStagingBuffer.

	ivec2s origin;
	ivec2s size;
	int32_t z_idx;
	uint32_t layer_idx;
} SpriteCell;

typedef struct SpriteFrame 
{
	SpriteCell* cells; size_t num_cells;
	Rect hitbox;
	float dur;
} SpriteFrame;

typedef struct SpriteDesc 
{
	char* name;

	ivec2s origin;
	ivec2s size;
	SpriteFrame* frames; size_t num_frames;
	
	VkImage vk_image;
	VkImageView vk_image_view;
	size_t vk_image_array_layers;
	VkDescriptorSet vk_descriptor_set;
} SpriteDesc;

typedef struct Sprite 
{
	size_t idx;
} Sprite;

typedef struct Anim 
{
	Sprite sprite;
	float dt_accumulator;
	uint32_t frame_idx;
	bool ended;
} Anim;

typedef uint32_t EntityType;
enum 
{
	EntityType_Player,
	EntityType_Boar,
};

typedef uint32_t EntityState;
enum 
{
	EntityState_Inactive,
	EntityState_Die,
	EntityState_Attack,
	EntityState_Fall,
	EntityState_Jump,
	EntityState_Free,
	EntityState_Hurt,	
};

typedef struct Instance 
{
	Rect rect;
	int32_t anim_frame_idx;
} Instance;

typedef struct Entity 
{
	Anim anim;

	ivec2s pos;
	ivec2s start_pos;
	vec2s pos_remainder;
	vec2s vel;
	int32_t dir;

	EntityType type;
	EntityState state;
} Entity;

typedef struct Tile 
{
	ivec2s src;
	ivec2s dst;
} Tile;

typedef struct TileLayer 
{
	Tile* tiles;
	size_t num_tiles;
} TileLayer;

typedef struct Level 
{
	ivec2s size; // measured in tiles

	// NOTE: entities[0] is always the player.
	Entity* entities; size_t num_entities;

	TileLayer* tile_layers; size_t num_tile_layers;
	bool* tiles; // num_tiles = size.x*size.y
} Level;

#if TOGGLE_REPLAY_FRAMES
typedef struct ReplayFrame 
{
	Entity* entities; size_t num_entities;
} ReplayFrame;
#endif // TOGGLE_REPLAY_FRAMES

// https://www.gingerbill.org/article/2019/02/08/memory-allocation-strategies-002/
typedef struct Arena 
{
	uint8_t* buf;
	size_t buf_len;
	size_t prev_offset;
	size_t curr_offset;
} Arena;

// https://www.gingerbill.org/article/2019/02/15/memory-allocation-strategies-003/
typedef Arena Stack;

typedef struct StackAllocHeader 
{
	size_t prev_offset;
	size_t padding;
} StackAllocHeader;

typedef struct VulkanFrame 
{
	VkCommandBuffer command_buffer;
	VkSemaphore sem_image_available;
	VkSemaphore sem_render_finished;
	VkFence fence_in_flight;
} VulkanFrame;

#define PIPELINE_COUNT 2

#if SDL_ASSERT_LEVEL >= 2
typedef enum VulkanBufferMode
{
	VulkanBufferMode_None,
	VulkanBufferMode_Write,
	VulkanBufferMode_Read,
} VulkanBufferMode;
#endif

typedef struct VulkanBuffer 
{
	VkBuffer handle;
	VkDeviceSize size;
	VkDeviceSize offset;
	VkDeviceSize start;
	VkDeviceMemory memory;
	void* mapped_memory;
#if SDL_ASSERT_LEVEL >= 2
	VulkanBufferMode mode;
#endif
} VulkanBuffer;

typedef struct Uniforms
{
	ivec2s viewport_size;
	ivec2s tileset_size;
	int32_t tile_size;
} Uniforms;

typedef struct Vulkan 
{
	VkInstance instance;

	VkPhysicalDevice physical_device;
	VkPhysicalDeviceProperties physical_device_properties;
	VkPhysicalDeviceMemoryProperties physical_device_memory_properties;

	VkLayerProperties* instance_layers; size_t num_instance_layers;
	VkExtensionProperties* instance_extensions; size_t num_instance_extensions;
	VkExtensionProperties* device_extensions; size_t num_device_extensions;

	VkSurfaceKHR surface;
	VkSurfaceCapabilitiesKHR surface_capabilities;
	VkSurfaceFormatKHR* surface_formats; size_t num_surface_formats;

	VkQueueFamilyProperties* queue_family_properties; size_t num_queue_family_properties;
	VkQueue* queues;
	VkQueue graphics_queue;

	VkDevice device;

	VkSwapchainKHR swapchain;
	VkSwapchainCreateInfoKHR swapchain_info;

	VkImage* swapchain_images;
	VkImageView* swapchain_image_views;
	VkFramebuffer* framebuffers;
	size_t num_swapchain_images;

	VkDescriptorSetLayout descriptor_set_layout_uniforms;
	VkDescriptorSetLayout descriptor_set_layout_sprites;
	VkPipelineLayout pipeline_layout;
	VkDescriptorSet descriptor_set_uniforms;
	VkPipeline pipelines[PIPELINE_COUNT];
	VkPipelineCache pipeline_cache;
	VkRenderPass render_pass;
	VkSampler sampler;

	VkCommandPool command_pool;

	VkDescriptorPool descriptor_pool;

	VulkanFrame* frames; size_t num_frames;
	size_t current_frame;

	/*
	Memory layout:
		uint32_t raw_image_data[][dst_buf_size];
		Uniforms uniforms;
		Tile tiles[];
	*/
	VulkanBuffer static_staging_buffer;
	
	/*
	Memory layout:
		EntityInstance entities[];
	*/
	VulkanBuffer dynamic_staging_buffer;

	/*
	Memory layout:
		Tile tiles[];
		EntityInstance entities[];
	*/
	VulkanBuffer vertex_buffer;

	/*
	Memory layout:
		Uniforms uniforms;
	*/
	VulkanBuffer uniform_buffer;

	VkDeviceMemory image_memory;

	bool staged;
} Vulkan;

typedef struct Context 
{
#if TOGGLE_PROFILING
	SpallProfile spall_ctx;
	SpallBuffer spall_buffer;
#endif // TOGGLE_PROFILING

	Arena arena;
	Stack stack;

	SDL_Window* window;
	ivec2s viewport_size;
	bool vsync;
	bool running;

	SDL_Gamepad* gamepad;
	vec2s gamepad_left_stick;

	bool button_left;
	bool button_right;
	bool button_jump;
	bool button_jump_released;
	bool button_attack;

	bool left_mouse_pressed;
	vec2s mouse_pos;
	
	Level level;

	// sprites is a hash map, not an array.
	// When looping through sprites, please loop MAX_SPRITES times, not num_sprites times.
	SpriteDesc sprites[MAX_SPRITES]; size_t num_sprites;

	Vulkan vk;

#if TOGGLE_REPLAY_FRAMES
	ReplayFrame* replay_frames; 
	size_t replay_frame_idx; 
	size_t replay_frame_idx_max;
	size_t c_replay_frames;
	bool paused;
#endif // TOGGLE_REPLAY_FRAMES
} Context;

typedef struct VkImageMemoryRequirements 
{
	VkMemoryRequirements memoryRequirements;
	VkImage image;
} VkImageMemoryRequirements;

#include "util.c"
#include "vk_util.c"

static Sprite player_idle;
static Sprite player_run;
static Sprite player_jump_start;
static Sprite player_jump_end;
static Sprite player_attack;
static Sprite player_die;

static Sprite boar_idle;
static Sprite boar_walk;
static Sprite boar_run;
static Sprite boar_hit;

static Sprite spr_tiles;

static float dt;

function ivec2s GetSpriteOrigin(Context* ctx, Sprite sprite, int32_t dir) 
{
	SpriteDesc* sd = GetSpriteDesc(ctx, sprite);
	ivec2s origin = sd->origin;
	if (dir == -1) 
	{
		origin.x = sd->size.x - origin.x;
	}
	return origin;
}

function ivec2s GetEntityOrigin(Context* ctx, Entity* entity)
{
	return GetSpriteOrigin(ctx, entity->anim.sprite, entity->dir);
}

function bool SetAnimSprite(Anim* anim, Sprite sprite) 
{
    bool sprite_changed = false;
    if (!SpritesEqual(anim->sprite, sprite)) 
    {
        sprite_changed = true;
        ResetAnim(anim);
        anim->sprite = sprite;
    }
    return sprite_changed;
}

function void UpdateAnim(Context* ctx, Anim* anim, bool loop) 
{
	SPALL_BUFFER_BEGIN();

    SpriteDesc* sd = GetSpriteDesc(ctx, anim->sprite);
    SDL_assert(anim->frame_idx >= 0 && (size_t)anim->frame_idx < sd->num_frames);
	float dur = sd->frames[anim->frame_idx].dur;
	size_t num_frames = sd->num_frames;

    if (loop || !anim->ended) 
    {
        anim->dt_accumulator += dt;
        if (anim->dt_accumulator >= dur) 
        {
            anim->dt_accumulator = 0.0f;
            anim->frame_idx += 1;
            if ((size_t)anim->frame_idx >= num_frames) 
            {
                if (loop) anim->frame_idx = 0;
                else 
                {
                    anim->frame_idx -= 1;
                    anim->ended = true;
                }
            }
        }
    }

    SPALL_BUFFER_END();
}

function void ResetGame(Context* ctx) 
{
	Entity* player = &ctx->level.entities[0];
	
	ResetAnim(&player->anim);
	SetAnimSprite(&player->anim, player_idle);
	player->pos = player->start_pos;
	player->state = EntityState_Free;
	player->vel = (vec2s){0.0f};
	player->dir = 1;

	for (size_t entity_idx = 1; entity_idx < ctx->level.num_entities; entity_idx += 1) 
	{
		Entity* enemy = &ctx->level.entities[entity_idx];

		ResetAnim(&enemy->anim);
		if (enemy->type == EntityType_Boar) 
		{
			SetAnimSprite(&enemy->anim, boar_idle);
		} 
		// else if (entity->type == EntityType_) {}
		enemy->pos = enemy->start_pos;
		enemy->state = EntityState_Free;
		enemy->vel = (vec2s){0.0f};
		enemy->dir = 1;
	}	
}

function ivec2s GetTilesetDimensions(Context* ctx, Sprite tileset) 
{
	SpriteDesc* sd = GetSpriteDesc(ctx, tileset);
	SDL_assert(sd->num_frames == 1);
	SDL_assert(sd->frames[0].num_cells == 1);
	return sd->size;
}

function bool GetSpriteHitbox(Context* ctx, Sprite sprite, size_t frame_idx, int32_t dir, Rect* hitbox) 
{
	bool res = false;
	SpriteDesc* sd = GetSpriteDesc(ctx, sprite); SDL_assert(sd);
	SDL_assert(frame_idx < sd->num_frames); 
	SpriteFrame* frame = &sd->frames[frame_idx];
	SDL_assert(hitbox);
	*hitbox = frame->hitbox;
	if (dir == -1) 
	{
		hitbox->min.x = -frame->hitbox.max.x + sd->size.x;
		hitbox->max.x = -frame->hitbox.min.x + sd->size.x;
	}
	if (hitbox->max.x > hitbox->min.x && hitbox->max.y > hitbox->min.y) 
	{
		res = true;
	}
	return res;
}

function ASE_ChunkType ASE_ReadChunk(SDL_IOStream* fs, Stack* stack, void** out_raw_chunk, size_t* out_raw_chunk_size) 
{
	ASE_ChunkHeader chunk_header = {0};
	SDL_ReadStructChecked(fs, &chunk_header);
	if (chunk_header.size != sizeof(ASE_ChunkHeader))
	{
		*out_raw_chunk_size = chunk_header.size - sizeof(ASE_ChunkHeader);
		*out_raw_chunk = StackAllocRaw(stack, *out_raw_chunk_size, alignof(ASE_ChunkHeader));
		SDL_ReadIOChecked(fs, *out_raw_chunk, *out_raw_chunk_size);
	}

	return chunk_header.type;
}

function void LoadSprite(Context* ctx, char* path) 
{
	SPALL_BUFFER_BEGIN();

	SDL_CHECK(SDL_GetPathInfo(path, NULL));

	Sprite sprite = GetSprite(path);
	SpriteDesc* sd = GetSpriteDesc(ctx, sprite);
	SDL_assert(!sd && "Collision");
	sd = &ctx->sprites[sprite.idx];

	// SetSpriteName (we need this for vkSetDebugUtilsObjectNameEXT)
	{
		size_t buf_size = SDL_strlen(path) + 1;
		sd->name = ArenaAllocRaw(&ctx->arena, buf_size, 1);
		SDL_strlcpy(sd->name, path, buf_size);
	}

	SDL_IOStream* fs = SDL_IOFromFile(path, "r"); 
	SDL_CHECK(fs);

	ASE_Header header; 
	SDL_ReadStructChecked(fs, &header);
	SDL_assert(header.magic_number == 0xA5E0);

	SDL_assert(header.color_depth == 32);
	SDL_assert((header.pixel_w == 0 || header.pixel_w == 1) && (header.pixel_h == 0 || header.pixel_h == 1));
	SDL_assert(header.grid_x == 0);
	SDL_assert(header.grid_y == 0);
	SDL_assert(header.grid_w == 0 || header.grid_w == 16);
	SDL_assert(header.grid_h == 0 || header.grid_h == 16);

	sd->size.x = (int32_t)header.w;
	sd->size.y = (int32_t)header.h;

	sd->num_frames = header.num_frames;
	sd->frames = ArenaAlloc(&ctx->arena, sd->num_frames, SpriteFrame);

	uint16_t hitbox_layer_idx = UINT16_MAX;
	uint16_t origin_layer_idx = UINT16_MAX;

	for (size_t frame_idx = 0; frame_idx < sd->num_frames; frame_idx += 1) 
	{
		ASE_Frame frame;
		SDL_ReadStructChecked(fs, &frame);
		SDL_assert(frame.magic_number == 0xF1FA);

		// Would mean this aseprite file is very old.
		SDL_assert(frame.num_chunks != 0);

		sd->frames[frame_idx].dur = ((float)frame.frame_dur)/(1000.0f/60.0f);

		int64_t fs_pos = SDL_TellIO(fs);

		// NOTE: According to the Aseprite spec, all layer chunks are found in the first frame, but not necessarily before everything else in that frame.
		// https://github.com/aseprite/aseprite/blob/main/docs/ase-file-specs.md#layer-chunk-0x2004
		if (frame_idx == 0) 
		{
			for (size_t chunk_idx = 0, layer_idx = 0; chunk_idx < frame.num_chunks; chunk_idx += 1) 
			{
				void* raw_chunk; 
				size_t raw_chunk_size;
				ASE_ChunkType chunk_type = ASE_ReadChunk(fs, &ctx->stack, &raw_chunk, &raw_chunk_size);
				if (chunk_type == ASE_ChunkType_Layer) 
				{
					ASE_LayerChunk* chunk = raw_chunk;
					SDL_assert(chunk->layer_name.len > 0);

					char* layer_name = StackAlloc(&ctx->stack, chunk->layer_name.len + 1, char);

					SDL_strlcpy(layer_name, (const char*)(chunk+1), chunk->layer_name.len + 1);

					if (SDL_strcmp(layer_name, "Hitbox") == 0) 
					{
						SDL_assert(hitbox_layer_idx == UINT16_MAX);
						hitbox_layer_idx = (uint16_t)layer_idx;
					} 
					else if (SDL_strcmp(layer_name, "Origin") == 0) 
					{
						SDL_assert(origin_layer_idx == UINT16_MAX);
						origin_layer_idx = (uint16_t)layer_idx;					
					}
					layer_idx += 1;

					StackFree(&ctx->stack, layer_name);
				}

				StackFree(&ctx->stack, raw_chunk);
			}

			SDL_SeekIO(fs, fs_pos, SDL_IO_SEEK_SET);
		}
		
		for (size_t chunk_idx = 0; chunk_idx < frame.num_chunks; chunk_idx += 1)
		{
			void* raw_chunk;
			size_t raw_chunk_size;
			ASE_ChunkType chunk_type = ASE_ReadChunk(fs, &ctx->stack, &raw_chunk, &raw_chunk_size);

			if (chunk_type == ASE_ChunkType_Cell)
			{
				ASE_CellChunk* chunk = raw_chunk;
				if (chunk->layer_idx == hitbox_layer_idx)
				{
					sd->frames[frame_idx].hitbox = (Rect)
					{
						.min.x = (int32_t)chunk->x,
						.min.y = (int32_t)chunk->y,
						.max.x = (int32_t)(chunk->x + chunk->w),
						.max.y = (int32_t)(chunk->y + chunk->h),
					};
				} 
				else if (chunk->layer_idx == origin_layer_idx) 
				{
					if (frame_idx == 0)
					{
						sd->origin = (ivec2s){(int32_t)chunk->x, (int32_t)chunk->y};
					}
				} 
				else 
				{
					sd->frames[frame_idx].num_cells += 1;
					SDL_assert(chunk->type == ASE_CellType_CompressedImage);
				}
			}

			StackFree(&ctx->stack, raw_chunk);
		}
#if TOGGLE_TESTS
		SDL_Log("sprites[%s].frames[%llu].num_cells = %llu", sd->name, frame_idx, sd->frames[frame_idx].num_cells);
#endif

		if (sd->frames[frame_idx].num_cells > 0) 
		{
			sd->frames[frame_idx].cells = ArenaAlloc(&ctx->arena, sd->frames[frame_idx].num_cells, SpriteCell);
			size_t cell_idx = 0;

			SDL_SeekIO(fs, fs_pos, SDL_IO_SEEK_SET);

			for (size_t chunk_idx = 0; chunk_idx < frame.num_chunks; chunk_idx += 1) 
			{
				void* raw_chunk; 
				size_t raw_chunk_size;
				ASE_ChunkType chunk_type = ASE_ReadChunk(fs, &ctx->stack, &raw_chunk, &raw_chunk_size);

				ASE_CellChunk* chunk = raw_chunk;
				if (chunk_type == ASE_ChunkType_Cell && 
					chunk->layer_idx != hitbox_layer_idx && 
					chunk->layer_idx != origin_layer_idx) 
				{

					SpriteCell cell = 
				{
						.origin.x = (int32_t)chunk->x,
						.origin.y = (int32_t)chunk->y,
						.z_idx = chunk->z_idx,
						.layer_idx = (uint32_t)chunk->layer_idx,
						.size.x = (int32_t)chunk->w,
						.size.y = (int32_t)chunk->h,
					};

					SDL_assert(cell.size.x != 0 && cell.size.y != 0);
					size_t dst_buf_size = cell.size.x*cell.size.y * sizeof(uint32_t);
					cell.dst_buf = SDL_malloc(dst_buf_size); SDL_CHECK(cell.dst_buf);

					// It's the zero-sized array at the end of ASE_CellChunk.
					size_t src_buf_size = raw_chunk_size - sizeof(ASE_CellChunk) - 2;
					void* src_buf = (void*)((&chunk->h)+1);

					SPALL_BUFFER_BEGIN_NAME("INFL_ZInflate");
					size_t res = INFL_ZInflate(cell.dst_buf, dst_buf_size, src_buf, src_buf_size);
					SPALL_BUFFER_END();
					SDL_assert(res > 0);

					SDL_assert(cell_idx < sd->frames[frame_idx].num_cells);
					sd->frames[frame_idx].cells[cell_idx++] = cell;
				}

				StackFree(&ctx->stack, raw_chunk);
			}

			// Makes the cells draw in the correct order.
			SDL_assert(frame_idx < sd->num_frames);
			SDL_qsort(
				sd->frames[frame_idx].cells, 
				sd->frames[frame_idx].num_cells, 
				sizeof(SpriteCell), 
				(SDL_CompareCallback)CompareSpriteCells);
		}
	}

	SDL_CloseIO(fs);
	SPALL_BUFFER_END();
}

function SDL_EnumerationResult SDLCALL EnumerateSpriteDirectory(void* userdata, const char* dirname, const char* fname) 
{
	Context* ctx = userdata;
	SPALL_BUFFER_BEGIN();

	char dir_path[1024]; // dirname\fname
	SDL_CHECK(SDL_snprintf(dir_path, sizeof(dir_path), "%s%s", dirname, fname) >= 0);

	int32_t num_files;
	char** files = SDL_GlobDirectory(dir_path, "*.aseprite", 0, &num_files); SDL_CHECK(files);
	if (num_files == 0) 
	{
		SDL_CHECK(SDL_EnumerateDirectory(dir_path, EnumerateSpriteDirectory, ctx));
	} 
	else 
	{
		ctx->num_sprites += (size_t)num_files;

		for (size_t file_idx = 0; file_idx < (size_t)num_files; file_idx += 1) 
		{
			char* file = files[file_idx];
			char sprite_path[1024]; // dirname\fname\file
			SDL_CHECK(SDL_snprintf(sprite_path, sizeof(sprite_path), "%s\\%s", dir_path, file) >= 0);

			LoadSprite(ctx, sprite_path);
		}
	}
	
	SDL_free(files);
	SPALL_BUFFER_END();
	return SDL_ENUM_CONTINUE;
}

function Rect GetEntityHitbox(Context* ctx, Entity* entity) 
{
	SPALL_BUFFER_BEGIN();
	Rect hitbox = {0};
	SpriteDesc* sd = GetSpriteDesc(ctx, entity->anim.sprite);

	bool res = false;
	for (ssize_t frame_idx = (ssize_t)entity->anim.frame_idx; frame_idx >= 0 && !res; frame_idx -= 1) 
	{
		res = GetSpriteHitbox(ctx, entity->anim.sprite, (size_t)frame_idx, entity->dir, &hitbox); 
	}
	if (!res && entity->anim.frame_idx == 0) 
	{
		for (size_t frame_idx = 1; frame_idx < sd->num_frames && !res; frame_idx += 1) 
		{
			res = GetSpriteHitbox(ctx, entity->anim.sprite, frame_idx, entity->dir, &hitbox);
		}
	}
	SDL_assert(res);

	SPALL_BUFFER_END();
	return hitbox;
}

function Rect GetEntityRect(Context* ctx, Entity* entity)
{
	Rect res = GetEntityHitbox(ctx, entity);
	res.min = glms_ivec2_add(res.min, entity->pos); 
	res.max = glms_ivec2_add(res.max, entity->pos);
	ivec2s origin = GetEntityOrigin(ctx, entity); 
	res.min = glms_ivec2_sub(res.min, origin); 
	res.max = glms_ivec2_sub(res.max, origin); 
	return res;
}

function bool EntitiesIntersect(Context* ctx, Entity* a, Entity* b) 
{
	SPALL_BUFFER_BEGIN();
	bool res = false;

    if (a->state != EntityState_Inactive && b->state != EntityState_Inactive)
    {
    	Rect ha = GetEntityRect(ctx, a);
    	Rect hb = GetEntityRect(ctx, b);
    	res = RectsIntersect(ha, hb);
    } 

    SPALL_BUFFER_END();
    return res;
}

function void MoveEntity(Entity* entity, vec2s vel)
{
	entity->pos_remainder = glms_vec2_add(entity->pos_remainder, glms_vec2_scale(vel, dt));
	entity->pos = glms_ivec2_add(entity->pos, ivec2_from_vec2(glms_vec2_round(entity->pos_remainder)));
	entity->pos_remainder = glms_vec2_sub(entity->pos_remainder, glms_vec2_round(entity->pos_remainder));
}

function void UpdateEntityPhysics(Context* ctx, Entity* entity, vec2s acc, float fric, float max_vel) 
{
	Rect entity_rect = GetEntityRect(ctx, entity);
	entity->vel = glms_vec2_add(entity->vel, glms_vec2_scale(acc, dt));

	if (entity->state == EntityState_Free) 
	{
		if (entity->vel.x < 0.0f) entity->vel.x = SDL_min(0.0f, entity->vel.x + fric*dt);
		else if (entity->vel.x > 0.0f) entity->vel.x = SDL_max(0.0f, entity->vel.x - fric*dt);
		entity->vel.x = SDL_clamp(entity->vel.x, -max_vel, max_vel);
	}

	vec2s vel = entity->vel;
	if (entity->vel.x < 0.0f && entity_rect.min.x % TILE_SIZE == 0) 
	{
		ivec2s tile_pos; // measured in tiles
		tile_pos.x = entity_rect.min.x/TILE_SIZE;
		for (tile_pos.y = entity_rect.min.y/TILE_SIZE; tile_pos.y <= entity_rect.max.y/TILE_SIZE; ++tile_pos.y) 
		{
			if (TileIsSolid(&ctx->level, tile_pos)) 
			{
				vel.x = 0.0f;
				break;
			}
		}
	} 
	else if (entity->vel.x > 0.0f && (entity_rect.max.x+1) % TILE_SIZE == 0) 
	{
		ivec2s tile_pos; // measured in tiles
		tile_pos.x = (entity_rect.max.x+1)/TILE_SIZE;
		for (tile_pos.y = entity_rect.min.y/TILE_SIZE; tile_pos.y <= entity_rect.max.y/TILE_SIZE; ++tile_pos.y) 
		{
			if (TileIsSolid(&ctx->level, tile_pos)) 
			{
				vel.x = 0.0f;
				break;
			}
		}
	}
	if (entity->vel.y < 0.0f && entity_rect.min.y % TILE_SIZE == 0) 
	{
		ivec2s tile_pos; // measured in tiles
		tile_pos.y = (entity_rect.min.y-TILE_SIZE)/TILE_SIZE;
		for (tile_pos.x = entity_rect.min.x/TILE_SIZE; tile_pos.x <= entity_rect.max.x/TILE_SIZE; ++tile_pos.x) 
		{
			if (TileIsSolid(&ctx->level, tile_pos)) 
			{
				vel.y = 0.0f;
				break;
			}
		}
	} 
	else if (entity->vel.y > 0.0f && (entity_rect.max.y+1) % TILE_SIZE == 0) 
	{
		ivec2s tile_pos; // measured in tiles
		tile_pos.y = (entity_rect.max.y+1)/TILE_SIZE;
		for (tile_pos.x = entity_rect.min.x/TILE_SIZE; tile_pos.x <= entity_rect.max.x/TILE_SIZE; ++tile_pos.x) 
		{
			if (TileIsSolid(&ctx->level, tile_pos)) 
			{
				vel.y = 0.0f;
				entity->vel.y = 0.0f;
				entity->state = EntityState_Free;
				break;
			}
		}
	}

    MoveEntity(entity, vel);

    Rect prev_entity_rect = entity_rect;
    entity_rect = GetEntityRect(ctx, entity);

	ivec2s tile_pos; // measured in tiles
	for (tile_pos.y = entity_rect.min.y/TILE_SIZE; tile_pos.y <= (entity_rect.max.y+1)/TILE_SIZE; ++tile_pos.y) 
	{
		for (tile_pos.x = entity_rect.min.x/TILE_SIZE; tile_pos.x <= (entity_rect.max.x+1)/TILE_SIZE; ++tile_pos.x) 
		{
			Rect tile_rect = TilePosToRect(tile_pos);
			if (TileIsSolid(&ctx->level, tile_pos) && RectsIntersect(entity_rect, tile_rect)) 
			{
				if (!RectsIntersect(prev_entity_rect, tile_rect)) continue;
				if (entity->vel.x != 0.0f)
				{
					Rect er = prev_entity_rect;
					er.min.x = entity_rect.min.x;
					er.max.x = entity_rect.max.x;
					if (RectsIntersect(er, tile_rect)) 
					{
						int32_t amount = 0;
						int32_t incr = (int32_t)glm_signf(entity->vel.x);
						while (RectsIntersect(er, tile_rect)) 
						{
							er.min.x -= incr;
							er.max.x -= incr;
							amount += incr;
						}
						entity->pos.x -= amount;
					}

				}
				if (entity->vel.y != 0.0f)
				{
					Rect er = prev_entity_rect;
					er.min.y = entity_rect.min.y;
					er.max.y = entity_rect.max.y;
					if (RectsIntersect(er, tile_rect)) 
					{
						int32_t amount = 0;
						int32_t incr = (int32_t)glm_signf(entity->vel.y);
						while (RectsIntersect(er, tile_rect)) 
						{
							er.min.y -= incr;
							er.max.y -= incr;
							amount += incr;
						}
						entity->pos.y -= amount;

						if (entity->vel.y > 0.0f) 
						{
							entity->vel.y = 0.0f;
							entity->state = EntityState_Free;
						}
					}
				}
			}
		}
	}

	if (vel.y > 0.0f && entity->vel.y > 0.0f && entity->state != EntityState_Free)
	{
		entity->state = EntityState_Fall;
	}
}

function void UpdatePlayer(Context* ctx) 
{
	SPALL_BUFFER_BEGIN();
	Entity* player = GetPlayer(ctx);

	int32_t input_dir = 0;
	if (ctx->gamepad) 
	{
		if (ctx->gamepad_left_stick.x == 0.0f) input_dir = 0;
		else if (ctx->gamepad_left_stick.x > 0.0f) input_dir = 1;
		else if (ctx->gamepad_left_stick.x < 0.0f) input_dir = -1;
	} 
	else 
	{
		input_dir = ctx->button_right - ctx->button_left;
	}

	switch (player->state) 
	{
		case EntityState_Free: 
		{
			if (ctx->button_attack) 
			{
				player->state = EntityState_Attack;
			} 
			else if (ctx->button_jump) 
			{
				player->state = EntityState_Jump;
			}
		} break;
		case EntityState_Jump: 
		{
			if (ctx->button_jump_released) 
			{
				player->vel.y /= 2.0f;
				player->state = EntityState_Fall;
			}
		} break;
		default: 
		{
			// TODO?
		} break;
	}

	if (player->pos.y > (float)(ctx->level.size.y*TILE_SIZE)) 
	{
		ResetGame(ctx);
	} 
	else switch (player->state)
	{
		case EntityState_Free: 
		{
			vec2s acc = {0.0f, 0.0f};

			acc.y += GRAVITY;

			if (!ctx->gamepad) 
			{
				acc.x = (float)input_dir * PLAYER_ACC;
			} 
			else 
			{
				acc.x = ctx->gamepad_left_stick.x * PLAYER_ACC;
			}

			if (input_dir == 0 && player->vel.x == 0.0f) 
			{
				SetAnimSprite(&player->anim, player_idle);
			} 
			else 
			{
				SetAnimSprite(&player->anim, player_run);
				if (player->vel.x != 0.0f) 
				{
					player->dir = (int32_t)glm_signf(player->vel.x);
				} 
				else if (input_dir != 0) 
				{
					player->dir = input_dir;
				}
			}

			UpdateEntityPhysics(ctx, player, acc, PLAYER_FRIC, PLAYER_MAX_VEL);		

			bool loop = true;
			UpdateAnim(ctx, &player->anim, loop);
		} break;
		case EntityState_Inactive:
			break;
	    case EntityState_Die: 
	    {
			SetAnimSprite(&player->anim, player_die);
			bool loop = false;
			UpdateAnim(ctx, &player->anim, loop);
			if (player->anim.ended) 
			{
				ResetGame(ctx);
			}
		} break;

	    case EntityState_Attack: 
	    {
			SetAnimSprite(&player->anim, player_attack);

			size_t num_enemies; Entity* enemies = GetEnemies(ctx, &num_enemies);
			for (size_t enemy_idx = 0; enemy_idx < num_enemies; enemy_idx += 1) 
			{
				Entity* enemy = &enemies[enemy_idx];
				if (EntitiesIntersect(ctx, player, enemy)) 
				{
					switch (enemy->type) 
					{
					case EntityType_Boar:
						enemy->state = EntityState_Hurt;
						break;
					default:
						break;
					}
				}
			}
			
			bool loop = false;
			UpdateAnim(ctx, &player->anim, loop);
			if (player->anim.ended) 
			{
				player->state = EntityState_Free;
			}
		} break;
	    	
	    case EntityState_Fall: 
	    {
	    	SetAnimSprite(&player->anim, player_jump_end);

	    	vec2s acc = {0.0f, GRAVITY};
	    	UpdateEntityPhysics(ctx, player, acc, PLAYER_FRIC, PLAYER_MAX_VEL);

	    	bool loop = false;
	    	UpdateAnim(ctx, &player->anim, loop);
		} break;
	    	
		case EntityState_Jump: 
		{
			vec2s acc = {0.0f};
			if (SetAnimSprite(&player->anim, player_jump_start)) 
			{
				acc.y -= PLAYER_JUMP;
			}

	    	acc.y += GRAVITY;
	    	UpdateEntityPhysics(ctx, player, acc, PLAYER_FRIC, PLAYER_MAX_VEL);

			bool loop = false;
	    	UpdateAnim(ctx, &player->anim, loop);
		} break;

		default: 
		{
			// TODO?
		} break;
	}

	SPALL_BUFFER_END();
}

function void UpdateBoar(Context* ctx, Entity* boar) 
{
	SPALL_BUFFER_BEGIN();

	switch (boar->state) 
	{
		case EntityState_Hurt: 
		{
			SetAnimSprite(&boar->anim, boar_hit);

			bool loop = false;
			UpdateAnim(ctx, &boar->anim, loop);

			if (boar->anim.ended) 
			{
				boar->state = EntityState_Inactive;
			}
		} break;

		case EntityState_Fall: 
		{
			SetAnimSprite(&boar->anim, boar_idle);

			vec2s acc = {0.0f, GRAVITY};
			UpdateEntityPhysics(ctx, boar, acc, BOAR_FRIC, BOAR_MAX_VEL);

			bool loop = true;
			UpdateAnim(ctx, &boar->anim, loop);
		} break;

		case EntityState_Free: 
		{
			SetAnimSprite(&boar->anim, boar_idle);

			vec2s acc = {0.0f, GRAVITY};
			UpdateEntityPhysics(ctx, boar, acc, BOAR_FRIC, BOAR_MAX_VEL);

			bool loop = true;
			UpdateAnim(ctx, &boar->anim, loop);
		} break;

		default: 
		{
			// TODO?
		} break;
	}

	SPALL_BUFFER_END();
}

#if TOGGLE_REPLAY_FRAMES
function void SetReplayFrame(Context* ctx, size_t replay_frame_idx) 
{
	if (replay_frame_idx < ctx->replay_frame_idx_max) 
	{
		ctx->replay_frame_idx = replay_frame_idx;
		ReplayFrame* replay_frame = &ctx->replay_frames[ctx->replay_frame_idx];
		SDL_memcpy(ctx->level.entities, replay_frame->entities, replay_frame->num_entities * sizeof(Entity));
		ctx->level.num_entities = replay_frame->num_entities;
	}
}
#endif // TOGGLE_REPLAY_FRAMES

#ifdef _DEBUG
VkBool32 VKAPI_CALL VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT types, const VkDebugUtilsMessengerCallbackDataEXT* data, void* user_data) 
{
	UNUSED(severity);
	UNUSED(types);
	UNUSED(user_data);
	SDL_Log(data->pMessage);
	return VK_FALSE;
}
#endif // _DEBUG

int32_t main(int32_t argc, char* argv[]) 
{
	UNUSED(argc);
	UNUSED(argv);

	// GetSprites
	{
		// This is the only time that we set the sprite variables.
		// After that, they are effectively constants.

		player_idle = GetSprite("assets\\legacy_fantasy_high_forest\\Character\\Idle\\Idle.aseprite");
		player_run = GetSprite("assets\\legacy_fantasy_high_forest\\Character\\Run\\Run.aseprite");
		player_jump_start = GetSprite("assets\\legacy_fantasy_high_forest\\Character\\Jump-Start\\Jump-Start.aseprite");
		player_jump_end = GetSprite("assets\\legacy_fantasy_high_forest\\Character\\Jump-End\\Jump-End.aseprite");
		player_attack = GetSprite("assets\\legacy_fantasy_high_forest\\Character\\Attack-01\\Attack-01.aseprite");
		player_die = GetSprite("assets\\legacy_fantasy_high_forest\\Character\\Dead\\Dead.aseprite");

		boar_idle = GetSprite("assets\\legacy_fantasy_high_forest\\Mob\\Boar\\Idle\\Idle.aseprite");
		boar_walk = GetSprite("assets\\legacy_fantasy_high_forest\\Mob\\Boar\\Walk\\Walk-Base.aseprite");
		boar_run = GetSprite("assets\\legacy_fantasy_high_forest\\Mob\\Boar\\Run\\Run.aseprite");
		boar_hit = GetSprite("assets\\legacy_fantasy_high_forest\\Mob\\Boar\\Hit-Vanish\\Hit.aseprite");

		spr_tiles = GetSprite("assets\\legacy_fantasy_high_forest\\Assets\\Tiles.aseprite");
	}

	// InitContext
	Context* ctx;
	{
		SDL_CHECK(SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD));

		uint64_t memory_size = 1024ULL * 1024ULL * 64ULL;
		uint8_t* memory = SDL_malloc(memory_size); SDL_CHECK(memory);

		Arena arena;
		arena.buf = memory;
		arena.buf_len = memory_size/256;
		arena.prev_offset = 0;
		arena.curr_offset = 0;

		Stack stack;
		stack.buf = memory + memory_size/256 + 1;
		stack.buf_len = memory_size - arena.buf_len;
		stack.prev_offset = 0;
		stack.curr_offset = 0;

		ctx = ArenaAlloc(&arena, 1, Context);
		ctx->arena = arena;
		ctx->stack = stack;
	}

#if TOGGLE_PROFILING
	{
		bool ok;

		ok = spall_init_file("profile/profile.spall", 1, &ctx->spall_ctx); SDL_assert(ok);

		int32_t buffer_size = 1 * 1024 * 1024;
		uint8_t* buffer = SDL_malloc(buffer_size); SDL_CHECK(buffer);
		ctx->spall_buffer = (SpallBuffer)
		{
			.length = buffer_size,
			.data = buffer,
		};
		ok = spall_buffer_init(&ctx->spall_ctx, &ctx->spall_buffer); SDL_assert(ok);
	}
#endif // TOGGLE_PROFILING

	// CreateWindow
	{
		SPALL_BUFFER_BEGIN_NAME("CreateWindow");

		SPALL_BUFFER_BEGIN_NAME("SDL_GetPrimaryDisplay");
		SDL_DisplayID display = SDL_GetPrimaryDisplay();
		SPALL_BUFFER_END();

		SPALL_BUFFER_BEGIN_NAME("SDL_GetDesktopDisplayMode");
		const SDL_DisplayMode* display_mode = SDL_GetDesktopDisplayMode(display);
		SPALL_BUFFER_END();
		SDL_CHECK(display_mode);
		
		// NOTE: After this, dt is effectively a constant.
		dt = 60.0f/display_mode->refresh_rate;

		{
			SPALL_BUFFER_BEGIN_NAME("SDL_CreateWindow");

			int32_t window_width = display_mode->w/2;
			int32_t window_height = display_mode->h/2;
			SDL_WindowFlags window_flags = SDL_WINDOW_HIDDEN|SDL_WINDOW_HIGH_PIXEL_DENSITY|SDL_WINDOW_VULKAN;
#if TOGGLE_FULLSCREEN
			window_width *= 2;
			window_height *= 2;
			window_flags |= SDL_WINDOW_FULLSCREEN;
#endif // TOGGLE_FULLSCREEN
			ctx->window = SDL_CreateWindow("LegacyFantasy", window_width, window_height, window_flags);
			SDL_CHECK(ctx->window);

			ctx->viewport_size.x = 480;
			ctx->viewport_size.y = 270;
			
			SPALL_BUFFER_END();
		}

		SPALL_BUFFER_BEGIN_NAME("SDL_Vulkan_GetVkGetInstanceProcAddr");
		PFN_vkGetInstanceProcAddr f = (PFN_vkGetInstanceProcAddr)SDL_Vulkan_GetVkGetInstanceProcAddr();
		SPALL_BUFFER_END();

		SPALL_BUFFER_BEGIN_NAME("volkInitializeCustom");
		volkInitializeCustom(f);
		SPALL_BUFFER_END();

		SPALL_BUFFER_END();
	}

	// VulkanCreateInstance
	{
		SPALL_BUFFER_BEGIN_NAME("VulkanCreateInstance");

#ifdef SDL_PLATFORM_WINDOWS
		#define VK_KHR_platform_surface "VK_KHR_win32_surface"
#else
		#error Unsupported platform.
#endif // SDL_PLATFORM_WINDOWS

#if TOGGLE_VULKAN_VALIDATION
		static char const * const layers[] = { "VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_monitor" };
		#define VULKAN_DEBUG_EXTENSIONS "VK_EXT_debug_utils", "VK_EXT_layer_settings",
#else
		#define VULKAN_DEBUG_EXTENSIONS
#endif // TOGGLE_VULKAN_VALIDATION
		static char const * const instance_extensions[] =
		{ 
			VULKAN_DEBUG_EXTENSIONS
			"VK_KHR_surface", 
			VK_KHR_platform_surface, 
		};

		VkApplicationInfo app_info =
		{
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pNext = NULL,
			.pApplicationName = "Legacy Fantasy",
			.applicationVersion = VK_API_VERSION_1_0,
			.pEngineName = "Legacy Fantasy",
			.engineVersion = VK_API_VERSION_1_0,
			.apiVersion = VK_API_VERSION_1_1,
		};

		VkInstanceCreateInfo create_info =
		{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pApplicationInfo = &app_info,
#if TOGGLE_VULKAN_VALIDATION
			.enabledLayerCount = SDL_arraysize(layers),
			.ppEnabledLayerNames = layers,
#endif // TOGGLE_VULKAN_VALIDATION
			.enabledExtensionCount = SDL_arraysize(instance_extensions),
			.ppEnabledExtensionNames = instance_extensions,
		};

#if TOGGLE_VULKAN_VALIDATION
		VkDebugUtilsMessengerCreateInfoEXT debug_info = 
		{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT,
			.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			.pfnUserCallback = VulkanDebugCallback,
			.pUserData = ctx,
		};

		VkValidationFeatureEnableEXT validation_enabled[] = 
		{
			VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
			VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
		};

		VkValidationFeaturesEXT validation_info = 
		{
			.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
			.enabledValidationFeatureCount = SDL_arraysize(validation_enabled),
			.pEnabledValidationFeatures = validation_enabled,
			.disabledValidationFeatureCount = 0,
			.pDisabledValidationFeatures = NULL,
		};

		create_info.pNext = &debug_info;
		debug_info.pNext = &validation_info;
#endif // TOGGLE_VULKAN_VALIDATION

		if (vkCreateInstance(&create_info, NULL, &ctx->vk.instance) != VK_SUCCESS)
		{
			SDL_CHECK(SDL_ShowSimpleMessageBox(
				SDL_MESSAGEBOX_ERROR, 
				"FATAL ERROR", 
				"Please update your graphics driver. If that doesn't work, your computer is a toaster!", 
				NULL));
			return -1;
		}
		volkLoadInstanceOnly(ctx->vk.instance);

		SPALL_BUFFER_END();
	}

	// VulkanGetInstanceLayers
	{
		SPALL_BUFFER_BEGIN_NAME("VulkanGetInstanceLayers");

		uint32_t count;
		VK_CHECK(vkEnumerateInstanceLayerProperties(&count, NULL));
		ctx->vk.num_instance_layers = (size_t)count;
		ctx->vk.instance_layers = ArenaAlloc(&ctx->arena, ctx->vk.num_instance_layers, VkLayerProperties);
		VK_CHECK(vkEnumerateInstanceLayerProperties(&count, ctx->vk.instance_layers));

		SPALL_BUFFER_END();
	}

	// VulkanGetInstanceExtensions
	{
		SPALL_BUFFER_BEGIN_NAME("VulkanGetInstanceExtensions");

		uint32_t count;

		VK_CHECK(vkEnumerateInstanceExtensionProperties(NULL, &count, NULL));
		for (size_t layer_idx = 0; 
			layer_idx < ctx->vk.num_instance_layers;
			++layer_idx, ctx->vk.num_instance_extensions += (size_t)count) 
		{
			VK_CHECK(vkEnumerateInstanceExtensionProperties(
				ctx->vk.instance_layers[layer_idx].layerName, 
				&count, 
				NULL));
		}
		ctx->vk.instance_extensions = ArenaAlloc(&ctx->arena, ctx->vk.num_instance_extensions, VkExtensionProperties);

		VK_CHECK(vkEnumerateInstanceExtensionProperties(NULL, &count, NULL));
		VK_CHECK(vkEnumerateInstanceExtensionProperties(NULL, &count, ctx->vk.instance_extensions));
		for (size_t layer_idx = 0, extension_idx = (size_t)count; 
			layer_idx < ctx->vk.num_instance_layers && extension_idx < ctx->vk.num_instance_extensions;
			++layer_idx, extension_idx += (size_t)count) 
		{
			VK_CHECK(vkEnumerateInstanceExtensionProperties(
				ctx->vk.instance_layers[layer_idx].layerName, 
				&count, 
				NULL));
			VK_CHECK(vkEnumerateInstanceExtensionProperties(
				ctx->vk.instance_layers[layer_idx].layerName, 
				&count, 
				&ctx->vk.instance_extensions[extension_idx]));
		}

		SPALL_BUFFER_END();
	}

	// VulkanGetPhysicalDevice
	{
		SPALL_BUFFER_BEGIN_NAME("VulkanGetPhysicalDevice");
		uint32_t count;
		VK_CHECK(vkEnumeratePhysicalDevices(ctx->vk.instance, &count, NULL));
		VkPhysicalDevice* physical_devices = StackAlloc(&ctx->stack, count, VkPhysicalDevice);
		VK_CHECK(vkEnumeratePhysicalDevices(ctx->vk.instance, &count, physical_devices));

		for (size_t i = 0; i < (size_t)count; i += 1)
		{
			ctx->vk.physical_device = physical_devices[i];
			vkGetPhysicalDeviceProperties(ctx->vk.physical_device, &ctx->vk.physical_device_properties);
			vkGetPhysicalDeviceMemoryProperties(ctx->vk.physical_device, &ctx->vk.physical_device_memory_properties);
			if (ctx->vk.physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				break;
			}
		}

		StackFree(&ctx->stack, physical_devices);
		SPALL_BUFFER_END();
	}

	// VulkanGetDeviceExtensions
	{
		SPALL_BUFFER_BEGIN_NAME("VulkanGetDeviceExtensions");

		uint32_t count;

		VK_CHECK(vkEnumerateDeviceExtensionProperties(ctx->vk.physical_device, NULL, &count, NULL));
		size_t layer_idx;
		for (layer_idx = 0, ctx->vk.num_device_extensions = (size_t)count; 
			layer_idx < ctx->vk.num_instance_layers;
			++layer_idx, ctx->vk.num_device_extensions += (size_t)count) 
		{
			VK_CHECK(vkEnumerateDeviceExtensionProperties(
				ctx->vk.physical_device, 
				ctx->vk.instance_layers[layer_idx].layerName, 
				&count, 
				NULL));
		}
		ctx->vk.device_extensions = ArenaAlloc(&ctx->arena, ctx->vk.num_device_extensions, VkExtensionProperties);

		VK_CHECK(vkEnumerateDeviceExtensionProperties(ctx->vk.physical_device, NULL, &count, NULL));
		VK_CHECK(vkEnumerateDeviceExtensionProperties(ctx->vk.physical_device, NULL, &count, ctx->vk.device_extensions));
		size_t extension_idx;
		for (layer_idx = 0, extension_idx = (size_t)count; 
			layer_idx < ctx->vk.num_instance_layers && extension_idx < ctx->vk.num_device_extensions;
			++layer_idx, extension_idx += (size_t)count) 
		{
			VK_CHECK(vkEnumerateDeviceExtensionProperties(
					ctx->vk.physical_device,
					ctx->vk.instance_layers[layer_idx].layerName, 
					&count, 
					NULL));
			VK_CHECK(vkEnumerateDeviceExtensionProperties(
					ctx->vk.physical_device, 
					ctx->vk.instance_layers[layer_idx].layerName, 
					&count, 
					&ctx->vk.device_extensions[extension_idx]));

		}

		SPALL_BUFFER_END();
	}

	// VulkanCreateSurface
	{
		SPALL_BUFFER_BEGIN_NAME("VulkanCreateSurface");

		SDL_CHECK(SDL_Vulkan_CreateSurface(ctx->window, ctx->vk.instance, NULL, &ctx->vk.surface));
		VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->vk.physical_device, ctx->vk.surface, &ctx->vk.surface_capabilities));

		SPALL_BUFFER_END();
	}

	// VulkanGetPhysicalDeviceSurfaceFormats
	{
		SPALL_BUFFER_BEGIN_NAME("VulkanGetPhysicalDeviceSurfaceFormats");

		uint32_t count;
		VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->vk.physical_device, ctx->vk.surface, &count, NULL));
		ctx->vk.num_surface_formats = (size_t)count;
		ctx->vk.surface_formats = ArenaAlloc(&ctx->arena, ctx->vk.num_surface_formats, VkSurfaceFormatKHR);
		VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->vk.physical_device, ctx->vk.surface, &count, ctx->vk.surface_formats));

		SPALL_BUFFER_END();
	}

	// VulkanCreateDeviceAndGetDeviceQueues
	{
		{
			uint32_t count;
			vkGetPhysicalDeviceQueueFamilyProperties(ctx->vk.physical_device, &count, NULL);
			ctx->vk.num_queue_family_properties = (size_t)count;
			ctx->vk.queue_family_properties = ArenaAlloc(&ctx->arena, ctx->vk.num_queue_family_properties, VkQueueFamilyProperties);
			vkGetPhysicalDeviceQueueFamilyProperties(ctx->vk.physical_device, &count, ctx->vk.queue_family_properties);
		}

		VkPhysicalDeviceFeatures2 physical_device_features = 
		{
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
		};
		vkGetPhysicalDeviceFeatures2(ctx->vk.physical_device, &physical_device_features);

		static float const queue_priorities[] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};

		VkDeviceQueueCreateInfo* queue_infos = StackAlloc(&ctx->stack, ctx->vk.num_queue_family_properties, VkDeviceQueueCreateInfo);
		size_t num_queue_infos = 0;
		size_t num_queues = 0;
		for (size_t queue_family_idx = 0; queue_family_idx < ctx->vk.num_queue_family_properties; queue_family_idx += 1) 
		{
			if (ctx->vk.queue_family_properties[queue_family_idx].queueCount == 0) continue;
			queue_infos[num_queue_infos] = (VkDeviceQueueCreateInfo)
			{
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.queueFamilyIndex = (uint32_t)queue_family_idx,
				.queueCount = ctx->vk.queue_family_properties[queue_family_idx].queueCount,
				.pQueuePriorities = queue_priorities,
			};
			num_queue_infos += 1;
			num_queues += (size_t)ctx->vk.queue_family_properties[queue_family_idx].queueCount;
		}

#ifdef _DEBUG
		static char const * const vk_device_extensions[] = { "VK_KHR_swapchain" };
#else
		static char const * const vk_device_extensions[] = { "VK_KHR_swapchain" };
#endif // _DEBUG

		VkDeviceCreateInfo device_info = 
		{
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pNext = &physical_device_features,
			.queueCreateInfoCount = (uint32_t)num_queue_infos,
			.pQueueCreateInfos = queue_infos,
			.enabledExtensionCount = SDL_arraysize(vk_device_extensions),
			.ppEnabledExtensionNames = vk_device_extensions,
		};
		if (vkCreateDevice(ctx->vk.physical_device, &device_info, NULL, &ctx->vk.device) != VK_SUCCESS)
		{
			SDL_CHECK(SDL_ShowSimpleMessageBox(
				SDL_MESSAGEBOX_ERROR, 
				"FATAL ERROR", 
				"Please update your graphics driver. If that doesn't work, your computer is a toaster!", 
				NULL));
			return -1;
		}

		volkLoadDevice(ctx->vk.device);

		ctx->vk.queues = ArenaAlloc(&ctx->arena, num_queues, VkQueue);
		size_t queue_array_idx = 0;
		for (size_t queue_family_idx = 0; queue_family_idx < ctx->vk.num_queue_family_properties; queue_family_idx += 1) 
		{
			for (uint32_t queue_idx = 0; queue_idx < ctx->vk.queue_family_properties[queue_family_idx].queueCount; queue_idx += 1) 
			{
				vkGetDeviceQueue(ctx->vk.device, (uint32_t)queue_family_idx, queue_idx, &ctx->vk.queues[queue_array_idx]);
				++queue_array_idx;
			}
		}

		ctx->vk.graphics_queue = ctx->vk.queues[0]; // TODO

		StackFree(&ctx->stack, queue_infos);
	}

	// VulkanCreateSwapchain
	{
		SPALL_BUFFER_BEGIN_NAME("VulkanCreateSwapchain");

		VkSurfaceFormatKHR format = ctx->vk.surface_formats[0];
		for (size_t i = 1; i < ctx->vk.num_surface_formats; i += 1) 
		{
			if (ctx->vk.surface_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB || ctx->vk.surface_formats[i].format == VK_FORMAT_R8G8B8A8_SRGB) 
			{
				format = ctx->vk.surface_formats[i];
				break;
			}
		}

		ctx->vk.swapchain_info = (VkSwapchainCreateInfoKHR)
		{
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.surface = ctx->vk.surface,
			.minImageCount = SDL_min(ctx->vk.surface_capabilities.minImageCount + 1, ctx->vk.surface_capabilities.maxImageCount),
			.imageFormat = format.format,
			.imageColorSpace = format.colorSpace,
			.imageExtent = ctx->vk.surface_capabilities.currentExtent,
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			.preTransform = ctx->vk.surface_capabilities.currentTransform,
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode = VK_PRESENT_MODE_FIFO_KHR,
			.clipped = VK_TRUE,
		};

		VK_CHECK(vkCreateSwapchainKHR(ctx->vk.device, &ctx->vk.swapchain_info, NULL, &ctx->vk.swapchain));

		SPALL_BUFFER_END();
	}

	// VulkanGetSwapchainImages
	{
		SPALL_BUFFER_BEGIN_NAME("VulkanGetSwapchainImages");

		uint32_t count;
		VK_CHECK(vkGetSwapchainImagesKHR(ctx->vk.device, ctx->vk.swapchain, &count, NULL));
		ctx->vk.num_swapchain_images = (size_t)count;
		ctx->vk.swapchain_images = ArenaAlloc(&ctx->arena, ctx->vk.num_swapchain_images, VkImage);
		VK_CHECK(vkGetSwapchainImagesKHR(ctx->vk.device, ctx->vk.swapchain, &count, ctx->vk.swapchain_images));

		SPALL_BUFFER_END();
	}

	// VulkanCreateSwapchainImageView
	{
		SPALL_BUFFER_BEGIN_NAME("VulkanCreateSwapchainImageView");

		ctx->vk.swapchain_image_views = ArenaAlloc(&ctx->arena, ctx->vk.num_swapchain_images, VkImageView);
		for (size_t swapchain_image_idx = 0; swapchain_image_idx < ctx->vk.num_swapchain_images; swapchain_image_idx += 1) 
		{
			VkImageViewCreateInfo info = 
			{
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = ctx->vk.swapchain_images[swapchain_image_idx],
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = ctx->vk.swapchain_info.imageFormat,
				.subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1},
			};
			VK_CHECK(vkCreateImageView(ctx->vk.device, &info, NULL, &ctx->vk.swapchain_image_views[swapchain_image_idx]));
		}

		SPALL_BUFFER_END();
	}

	// VulkanAllocateFrames
	{
		ctx->vk.num_frames = ctx->vk.num_swapchain_images;
		ctx->vk.frames = ArenaAlloc(&ctx->arena, ctx->vk.num_frames, VulkanFrame);
	}

	// VulkanCreateCommandPool
	{
		SPALL_BUFFER_BEGIN_NAME("VulkanCreateCommandPool");

		VkCommandPoolCreateInfo info =
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		};
	
		// TODO: Search for graphics queue family index.
		VK_CHECK(vkCreateCommandPool(ctx->vk.device, &info, NULL, &ctx->vk.command_pool));

		SPALL_BUFFER_END();
	}

	// VulkanAllocateCommandBuffers
	{
		SPALL_BUFFER_BEGIN_NAME("VulkanAllocateCommandBuffers");

		VkCommandBufferAllocateInfo info =
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = ctx->vk.command_pool,
			.commandBufferCount = (uint32_t)ctx->vk.num_frames,
		};

		VkCommandBuffer* command_buffers = StackAlloc(&ctx->stack, ctx->vk.num_frames, VkCommandBuffer);
		VK_CHECK(vkAllocateCommandBuffers(ctx->vk.device, &info, command_buffers));

		for (size_t i = 0; i < ctx->vk.num_frames; i += 1) 
		{
			ctx->vk.frames[i].command_buffer = command_buffers[i];			
		}

		StackFree(&ctx->stack, command_buffers);

		SPALL_BUFFER_END();
	}

	// VulkanCreateFences
	{
		SPALL_BUFFER_BEGIN_NAME("VulkanCreateFences");

		VkFenceCreateInfo info =
		{
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT,
		};

		for (size_t i = 0; i < ctx->vk.num_frames; i += 1) 
		{
			VK_CHECK(vkCreateFence(ctx->vk.device, &info, NULL, &ctx->vk.frames[i].fence_in_flight));
		}

		SPALL_BUFFER_END();
	}

	// VulkanCreateSemaphores
	{
		SPALL_BUFFER_BEGIN_NAME("VulkanCreateSemaphores");

		VkSemaphoreCreateInfo info = 
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		};

		for (size_t i = 0; i < ctx->vk.num_frames; i += 1) 
		{
			VK_CHECK(vkCreateSemaphore(ctx->vk.device, &info, NULL, &ctx->vk.frames[i].sem_image_available));
			VK_CHECK(vkCreateSemaphore(ctx->vk.device, &info, NULL, &ctx->vk.frames[i].sem_render_finished));
		}

		SPALL_BUFFER_END();
	}


	// VulkanCreateSampler
	{
		SPALL_BUFFER_BEGIN_NAME("VulkanCreateSampler");

		VkSamplerCreateInfo info =
		{
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.magFilter = VK_FILTER_NEAREST,
			.minFilter = VK_FILTER_NEAREST,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.compareOp = VK_COMPARE_OP_ALWAYS,
			.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		};
		VK_CHECK(vkCreateSampler(ctx->vk.device, &info, NULL, &ctx->vk.sampler));

		SPALL_BUFFER_END();
	}

	// VulkanCreateDescriptorSetLayouts
	{
		VkDescriptorSetLayoutBinding bindings[] =
		{
			{
				.binding = 0,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			},
		};

		VkDescriptorSetLayoutCreateInfo info =
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = SDL_arraysize(bindings),
			.pBindings = bindings,
		};

		VK_CHECK(vkCreateDescriptorSetLayout(ctx->vk.device, &info, NULL, &ctx->vk.descriptor_set_layout_uniforms));
	}
	{
		VkDescriptorSetLayoutBinding bindings[] =
		{
			{
				.binding = 0,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			}
		};

		VkDescriptorSetLayoutCreateInfo info =
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = SDL_arraysize(bindings),
			.pBindings = bindings,
		};

		VK_CHECK(vkCreateDescriptorSetLayout(ctx->vk.device, &info, NULL, &ctx->vk.descriptor_set_layout_sprites));
	}

	// VulkanCreatePipelineLayout
	{
		SPALL_BUFFER_BEGIN_NAME("VulkanCreatePipelineLayout");

		VkDescriptorSetLayout layouts[] = {ctx->vk.descriptor_set_layout_uniforms, ctx->vk.descriptor_set_layout_sprites};

		VkPipelineLayoutCreateInfo pipeline_layout_info =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = SDL_arraysize(layouts),
			.pSetLayouts = layouts,
		};

		VK_CHECK(vkCreatePipelineLayout(ctx->vk.device, &pipeline_layout_info, NULL, &ctx->vk.pipeline_layout));

		SPALL_BUFFER_END();
	}

	// VulkanCreateRenderPass
	{
		SPALL_BUFFER_BEGIN_NAME("VulkanCreateRenderPass");

		VkAttachmentDescription color_attachment = 
		{
			.format = ctx->vk.swapchain_info.imageFormat,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		};

		VkAttachmentReference color_attachment_ref = 
		{
			.attachment = 0, // index in attachments array
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		};

		VkSubpassDescription subpass = 
		{
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = &color_attachment_ref,
		};

		VkSubpassDependency subpass_dependency = 
		{
			.srcSubpass = VK_SUBPASS_EXTERNAL, // implicit subpass before and after
			.dstSubpass = 0, // subpass index
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		};

		VkAttachmentDescription attachments[] = { color_attachment };

		VkRenderPassCreateInfo info =
		{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = SDL_arraysize(attachments),
			.pAttachments = attachments,
			.subpassCount = 1,
			.pSubpasses = &subpass,
			.dependencyCount = 1,
			.pDependencies = &subpass_dependency,
		};

		VK_CHECK(vkCreateRenderPass(ctx->vk.device, &info, NULL, &ctx->vk.render_pass));

		SPALL_BUFFER_END();
	}

	// VulkanCreateFramebuffers
	{
		SPALL_BUFFER_BEGIN_NAME("VulkanCreateFramebuffers");

		ctx->vk.framebuffers = ArenaAlloc(&ctx->arena, ctx->vk.num_swapchain_images, VkFramebuffer);
		for (size_t i = 0; i < ctx->vk.num_swapchain_images; i += 1) 
		{
			VkImageView attachments[] = 
			{
				ctx->vk.swapchain_image_views[i],
			};

			VkFramebufferCreateInfo info = 
			{
				.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				.renderPass = ctx->vk.render_pass,
				.attachmentCount = SDL_arraysize(attachments),
				.pAttachments = attachments,
				.width = ctx->vk.swapchain_info.imageExtent.width,
				.height = ctx->vk.swapchain_info.imageExtent.height,
				.layers = 1,
			};
			VK_CHECK(vkCreateFramebuffer(ctx->vk.device, &info, NULL, &ctx->vk.framebuffers[i]));
		}

		SPALL_BUFFER_END();
	}

	// VulkanCreatePipelineCache
	{
		SPALL_BUFFER_BEGIN_NAME("VulkanCreatePipelineCache");

		VkPipelineCacheCreateInfo info = 
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
		};
		VK_CHECK(vkCreatePipelineCache(ctx->vk.device, &info, NULL, &ctx->vk.pipeline_cache));

		SPALL_BUFFER_END();
	}

	// VulkanCreateGraphicsPipelines
	{
		SPALL_BUFFER_BEGIN_NAME("VulkanCreateGraphicsPipelines");

		VkPipelineDynamicStateCreateInfo dynamic_state_info =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		};
		VkPipelineInputAssemblyStateCreateInfo input_assembly_info = 
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		};
		VkViewport viewport =
		{
			.width = (float)ctx->vk.swapchain_info.imageExtent.width,
			.height = (float)ctx->vk.swapchain_info.imageExtent.height,
			.maxDepth = 1.0f,
		};
		VkRect2D scissor = {
			.extent = ctx->vk.swapchain_info.imageExtent,
		};
		VkPipelineViewportStateCreateInfo viewport_info =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.viewportCount = 1,
			.pViewports = &viewport,
			.scissorCount = 1,
			.pScissors = &scissor,
		};
		VkPipelineRasterizationStateCreateInfo rasterization_info =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.cullMode = VK_CULL_MODE_BACK_BIT,
			.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
			.lineWidth = 1.0f
		};
		VkPipelineMultisampleStateCreateInfo multisample_info =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		};
		VkPipelineColorBlendAttachmentState blend_attachment_info = 
		{
			.blendEnable = VK_TRUE,
			.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.colorBlendOp = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
			.alphaBlendOp = VK_BLEND_OP_ADD,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		};
		VkPipelineColorBlendStateCreateInfo blend_info = 
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.attachmentCount = 1,
			.pAttachments = &blend_attachment_info,
		};

		VkGraphicsPipelineCreateInfo graphics_pipline_info =
		{
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
#ifdef _DEBUG
			.flags = VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT,
#endif // _DEBUG
			.pInputAssemblyState = &input_assembly_info,
			.pViewportState = &viewport_info,
			.pRasterizationState = &rasterization_info,
			.pMultisampleState = &multisample_info,
			.pColorBlendState = &blend_info,
			.pDynamicState = &dynamic_state_info,
			.layout = ctx->vk.pipeline_layout,
			.renderPass = ctx->vk.render_pass,
		};

		// VulkanCreateGraphicsPipelineTile
		VkPipelineShaderStageCreateInfo tile_vert = VulkanCreateShaderStage(ctx->vk.device, "build/shaders/tile_vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		VkPipelineShaderStageCreateInfo tile_frag = VulkanCreateShaderStage(ctx->vk.device, "build/shaders/tile_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		VkPipelineShaderStageCreateInfo tile_shader_stages[] = 
		{
			tile_vert,
			tile_frag,
		};
		VkVertexInputBindingDescription tile_vertex_input_bindings[] = 
		{
			{
				.binding = 0,
				.stride = sizeof(Tile),
				.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE,
			},
		};
		VkVertexInputAttributeDescription tile_vertex_attributes[] = 
		{
			{
				.location = 0,
				.binding = 0,
				.format = VK_FORMAT_R32G32_SINT,
				.offset = offsetof(Tile, src),
			},
			{
				.location = 1,
				.binding = 0,
				.format = VK_FORMAT_R32G32_SINT,
				.offset = offsetof(Tile, dst),
			},
		};
		VkPipelineVertexInputStateCreateInfo tile_vertex_input_info =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.vertexBindingDescriptionCount = SDL_arraysize(tile_vertex_input_bindings),
			.pVertexBindingDescriptions = tile_vertex_input_bindings,
			.vertexAttributeDescriptionCount = SDL_arraysize(tile_vertex_attributes),
			.pVertexAttributeDescriptions = tile_vertex_attributes,
		};
		VkGraphicsPipelineCreateInfo tile_graphics_pipeline_info = graphics_pipline_info;
		tile_graphics_pipeline_info.stageCount = SDL_arraysize(tile_shader_stages);
		tile_graphics_pipeline_info.pStages = tile_shader_stages;
		tile_graphics_pipeline_info.pVertexInputState = &tile_vertex_input_info;

		// VulkanCreateGraphicsPipelineEntity
		VkPipelineShaderStageCreateInfo entity_vert = VulkanCreateShaderStage(ctx->vk.device, "build/shaders/entity_vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		VkPipelineShaderStageCreateInfo entity_frag = VulkanCreateShaderStage(ctx->vk.device, "build/shaders/entity_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		VkPipelineShaderStageCreateInfo entity_shader_stages[] = 
		{
			entity_vert,
			entity_frag,
		};
		VkVertexInputBindingDescription entity_vertex_input_bindings[] = 
		{
			{
				.binding = 0,
				.stride = sizeof(Instance),
				.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE,
			},
		};
		VkVertexInputAttributeDescription entity_vertex_attributes[] = 
		{
			{
				.location = 0,
				.binding = 0,
				.format = VK_FORMAT_R32G32B32A32_SINT,
				.offset = offsetof(Instance, rect),
			},
			{
				.location = 1,
				.binding = 0,
				.format = VK_FORMAT_R32_SINT,
				.offset = offsetof(Instance, anim_frame_idx),
			},
		};
		VkPipelineVertexInputStateCreateInfo entity_vertex_input_info = 
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.vertexBindingDescriptionCount = SDL_arraysize(entity_vertex_input_bindings),
			.pVertexBindingDescriptions = entity_vertex_input_bindings,
			.vertexAttributeDescriptionCount = SDL_arraysize(entity_vertex_attributes),
			.pVertexAttributeDescriptions = entity_vertex_attributes,
		};
		VkGraphicsPipelineCreateInfo entity_graphics_pipeline_info = graphics_pipline_info;
		entity_graphics_pipeline_info.stageCount = SDL_arraysize(entity_shader_stages);
		entity_graphics_pipeline_info.pStages = entity_shader_stages;
		entity_graphics_pipeline_info.pVertexInputState = &entity_vertex_input_info;

		VkGraphicsPipelineCreateInfo infos[PIPELINE_COUNT] = 
		{
			tile_graphics_pipeline_info,
			entity_graphics_pipeline_info,
		};

		VK_CHECK(vkCreateGraphicsPipelines(ctx->vk.device, ctx->vk.pipeline_cache, SDL_arraysize(infos), infos, NULL, ctx->vk.pipelines));

		vkDestroyShaderModule(ctx->vk.device, entity_frag.module, NULL);
		vkDestroyShaderModule(ctx->vk.device, entity_vert.module, NULL);
		vkDestroyShaderModule(ctx->vk.device, tile_frag.module, NULL);
		vkDestroyShaderModule(ctx->vk.device, tile_vert.module, NULL);
		SPALL_BUFFER_END();
	}

	// LoadSprites
	SDL_CHECK(SDL_EnumerateDirectory("assets\\legacy_fantasy_high_forest", EnumerateSpriteDirectory, ctx));

#if TOGGLE_TESTS
	size_t error_count = 0;
	for (size_t sprite_idx = 0; sprite_idx < MAX_SPRITES; sprite_idx += 1) 
	{
		SpriteDesc* sd = GetSpriteDesc(ctx, (Sprite){sprite_idx});
		if (sd) 
		{
			for (size_t frame_idx = 0; frame_idx < sd->num_frames; frame_idx += 1) 
			{
				for (size_t cell_idx = 0; cell_idx < sd->frames[frame_idx].num_cells; cell_idx += 1) 
				{
					SpriteCell* cell = &ctx->sprites[sprite_idx].frames[frame_idx].cells[cell_idx];
					if (cell->size.x == 0 || cell->size.y == 0) 
					{
						SDL_Log("ERROR: (%s).frames[%llu].cells[%llu].size == 0", sd->name, frame_idx, cell_idx);
						error_count += 1;
					}
				}
			}
		}
	}
	SDL_Log("Error count: %llu", error_count);
#endif

	// LoadLevel
	{
		SPALL_BUFFER_BEGIN_NAME("LoadLevel");

		cJSON* head;
		{
			size_t file_len;

			void* file_data = SDL_LoadFile("assets\\levels\\test.ldtk", &file_len); 
			SDL_CHECK(file_data);

			SPALL_BUFFER_BEGIN_NAME("cJSON_ParseWithLength");
			head = cJSON_ParseWithLength((const char*)file_data, file_len);
			SPALL_BUFFER_END();

			SDL_free(file_data);
		}
		SDL_assert(HAS_FLAG(head->type, cJSON_Object));

		cJSON* level_nodes = cJSON_GetObjectItem(head, "levels");
		cJSON* level_node = level_nodes->child;

		cJSON* w = cJSON_GetObjectItem(level_node, "pxWid");
		ctx->level.size.x = ((int32_t)cJSON_GetNumberValue(w))/TILE_SIZE;

		cJSON* h = cJSON_GetObjectItem(level_node, "pxHei");
		ctx->level.size.y = ((int32_t)cJSON_GetNumberValue(h))/TILE_SIZE;

		size_t num_tiles = (size_t)(ctx->level.size.x*ctx->level.size.y);
		ctx->level.tiles = ArenaAlloc(&ctx->arena, num_tiles, bool);

		ctx->level.num_tile_layers = 3;
		ctx->level.tile_layers = ArenaAlloc(&ctx->arena, ctx->level.num_tile_layers, TileLayer);

		const char* layer_tiles = "Tiles";
		const char* layer_props = "Props";
		const char* layer_grass = "Grass";

		ctx->level.num_entities = 1; // the player

		const char* layer_player = "Player";
		const char* layer_enemies = "Enemies";

		cJSON* layer_instances = cJSON_GetObjectItem(level_node, "layerInstances");
		cJSON* layer_instance; 
		cJSON_ArrayForEach(layer_instance, layer_instances) 
		{
			cJSON* node_type = cJSON_GetObjectItem(layer_instance, "__type");
			char* type = cJSON_GetStringValue(node_type); SDL_assert(type);
			cJSON* node_ident = cJSON_GetObjectItem(layer_instance, "__identifier");
			char* ident = cJSON_GetStringValue(node_ident); SDL_assert(ident);

			if (SDL_strcmp(type, "Tiles") == 0) 
			{
				TileLayer* tile_layer = NULL;
				// TODO
 				if (SDL_strcmp(ident, layer_tiles) == 0) 
 				{
					tile_layer = &ctx->level.tile_layers[0];
				} 
				else if (SDL_strcmp(ident, layer_props) == 0) 
				{
					tile_layer = &ctx->level.tile_layers[1];
				} 
				else if (SDL_strcmp(ident, layer_grass) == 0) 
				{
					tile_layer = &ctx->level.tile_layers[2];
				} 
				else 
				{
					SDL_assert(!"Invalid layer!");
				}

				cJSON* grid_tiles = cJSON_GetObjectItem(layer_instance, "gridTiles");
				cJSON* grid_tile; 
				cJSON_ArrayForEach(grid_tile, grid_tiles) 
				{
					tile_layer->num_tiles += 1;
				}
				tile_layer->tiles = ArenaAlloc(&ctx->arena, tile_layer->num_tiles, Tile);
				SDL_memset(tile_layer->tiles, -1, tile_layer->num_tiles * sizeof(Tile));
			}
			else if (SDL_strcmp(ident, layer_enemies) == 0) 
			{
				cJSON* entity_instances = cJSON_GetObjectItem(layer_instance, "entityInstances");
				cJSON* entity_instance; 
				cJSON_ArrayForEach(entity_instance, entity_instances) 
				{
					ctx->level.num_entities += 1;
				}
			}
			else if (SDL_strcmp(ident, "IntGrid") == 0)
			{
				cJSON* tile_collisions = cJSON_GetObjectItem(layer_instance, "intGridCsv"); SDL_assert(tile_collisions);
				cJSON* tile_collision;
				size_t tile_collision_idx = 0;
				cJSON_ArrayForEach(tile_collision, tile_collisions) 
				{
					bool val = (bool)cJSON_GetNumberValue(tile_collision);
					ctx->level.tiles[tile_collision_idx++] = val;
				}
			}
		}

		ctx->level.entities = ArenaAlloc(&ctx->arena, ctx->level.num_entities, Entity);
		Entity* enemy = &ctx->level.entities[1];

		cJSON_ArrayForEach(layer_instance, layer_instances) 
		{
			cJSON* node_type = cJSON_GetObjectItem(layer_instance, "__type");
			char* type = cJSON_GetStringValue(node_type); SDL_assert(type);
			cJSON* node_ident = cJSON_GetObjectItem(layer_instance, "__identifier");
			char* ident = cJSON_GetStringValue(node_ident); SDL_assert(ident);
			if (SDL_strcmp(type, "Tiles") == 0) 
			{
				cJSON* grid_tiles = cJSON_GetObjectItem(layer_instance, "gridTiles");

				// TODO
				TileLayer* tile_layer = NULL;
 				if (SDL_strcmp(ident, layer_tiles) == 0) 
 				{
					tile_layer = &ctx->level.tile_layers[0];
				} 
				else if (SDL_strcmp(ident, layer_props) == 0) 
				{
					tile_layer = &ctx->level.tile_layers[1];
				} 
				else if (SDL_strcmp(ident, layer_grass) == 0) 
				{
					tile_layer = &ctx->level.tile_layers[2];
				} 
				else 
				{
					SDL_assert(!"Invalid layer!");
				}

				size_t i = 0;
				cJSON* grid_tile; 
				cJSON_ArrayForEach(grid_tile, grid_tiles) 
				{
					cJSON* src_node = cJSON_GetObjectItem(grid_tile, "src");
					ivec2s src = 
					{
						(int32_t)cJSON_GetNumberValue(src_node->child),
						(int32_t)cJSON_GetNumberValue(src_node->child->next),
					};

					cJSON* dst_node = cJSON_GetObjectItem(grid_tile, "px");
					ivec2s dst = 
					{
						(int32_t)cJSON_GetNumberValue(dst_node->child),
						(int32_t)cJSON_GetNumberValue(dst_node->child->next),
					};

					SDL_assert(i < tile_layer->num_tiles);
					tile_layer->tiles[i++] = (Tile){src, dst};
				}
			}
			else if (SDL_strcmp(type, "Entities") == 0) 
			{
				cJSON* entity_instances = cJSON_GetObjectItem(layer_instance, "entityInstances");

				if (SDL_strcmp(ident, layer_player) == 0) 
				{
					cJSON* entity_instance = entity_instances->child;
					cJSON* world_x = cJSON_GetObjectItem(entity_instance, "__worldX");
					cJSON* world_y = cJSON_GetObjectItem(entity_instance, "__worldY");
					ctx->level.entities[0].start_pos = (ivec2s){(int32_t)cJSON_GetNumberValue(world_x), (int32_t)cJSON_GetNumberValue(world_y)};
				} 
				else if (SDL_strcmp(ident, layer_enemies) == 0) 
				{
					cJSON* entity_instance; cJSON_ArrayForEach(entity_instance, entity_instances) 
					{
						cJSON* identifier_node = cJSON_GetObjectItem(entity_instance, "__identifier");
						char* identifier = cJSON_GetStringValue(identifier_node);
						cJSON* world_x = cJSON_GetObjectItem(entity_instance, "__worldX");
						cJSON* world_y = cJSON_GetObjectItem(entity_instance, "__worldY");
						enemy->start_pos = (ivec2s){(int32_t)cJSON_GetNumberValue(world_x), (int32_t)cJSON_GetNumberValue(world_y)};
						if (SDL_strcmp(identifier, "Boar") == 0) 
						{
							enemy->type = EntityType_Boar;
						} // else if (SDL_strcmp(identifier, "") == 0) {}
						enemy += 1;
					}
				}
			}	
		}

		cJSON_Delete(head);

		SPALL_BUFFER_END();
	}

#if TOGGLE_TESTS
	// PrintLevel
	{
		uint8_t* buf = StackAllocRaw(&ctx->stack, ctx->level.size.val.x + 1, 1);
		SDL_Log("level start");
		for (size_t y = 0; y < (size_t)(ctx->level.size.val.y); y += 1) 
		{
			for (size_t x = 0; x < (size_t)(ctx->level.size.val.x); x += 1) 
			{
				buf[x] = ctx->level.tiles[x + y*(size_t)ctx->level.size.val.x];
				if (buf[x] == 0) buf[x] = '0';
				else buf[x] = '1';
			}
			buf[ctx->level.size.val.x] = 0;
			SDL_Log((const char*)buf);
		}
		SDL_Log("level end");
		StackFree(&ctx->stack, buf);
	}
#endif
	
	// VulkanCreateStaticStagingBuffer
	{
		SPALL_BUFFER_BEGIN_NAME("VulkanCreateStaticStagingBuffer");

		for (size_t sprite_idx = 0; sprite_idx < MAX_SPRITES; sprite_idx += 1) 
		{
			SpriteDesc* sd = GetSpriteDesc(ctx, (Sprite){sprite_idx});
			if (sd) 
			{
				for (size_t frame_idx = 0; frame_idx < sd->num_frames; frame_idx += 1) 
				{
					for (size_t cell_idx = 0; cell_idx < sd->frames[frame_idx].num_cells; cell_idx += 1) 
					{
						SpriteCell* cell = &sd->frames[frame_idx].cells[cell_idx];
						ctx->vk.static_staging_buffer.size += cell->size.x*cell->size.y * sizeof(uint32_t);
					}
				}
			}
		}
		ctx->vk.static_staging_buffer.size += sizeof(Uniforms);
		for (size_t tile_layer_idx = 0; tile_layer_idx < ctx->level.num_tile_layers; tile_layer_idx += 1) 
		{
			TileLayer* tile_layer = &ctx->level.tile_layers[tile_layer_idx];
			ctx->vk.static_staging_buffer.size += tile_layer->num_tiles * sizeof(Tile);
		}

		ctx->vk.static_staging_buffer = VulkanCreateBuffer(&ctx->vk, ctx->vk.static_staging_buffer.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		VulkanSetBufferName(ctx->vk.device, ctx->vk.static_staging_buffer.handle, "Static Staging Buffer");

		VulkanMapBufferMemory(&ctx->vk, &ctx->vk.static_staging_buffer);

		for (size_t sprite_idx = 0; sprite_idx < MAX_SPRITES; sprite_idx += 1) 
		{
			SpriteDesc* sd = GetSpriteDesc(ctx, (Sprite){sprite_idx});
			if (sd) 
			{
				for (size_t frame_idx = 0; frame_idx < sd->num_frames; frame_idx += 1) 
				{
					for (size_t cell_idx = 0; cell_idx < sd->frames[frame_idx].num_cells; cell_idx += 1) 
					{
						SpriteCell* cell = &sd->frames[frame_idx].cells[cell_idx];
						VulkanCopyBuffer(cell->size.x*cell->size.y * sizeof(uint32_t), cell->dst_buf, &ctx->vk.static_staging_buffer);

						SDL_free(cell->dst_buf); 
						cell->dst_buf = NULL;
					}
				}
			}
		}
		ivec2s tileset_size = GetTilesetDimensions(ctx, spr_tiles);
		Uniforms uniforms = {
			.viewport_size = ctx->viewport_size,
			.tileset_size = tileset_size,
			.tile_size = 16,
		};
		VulkanCopyBuffer(sizeof(uniforms), &uniforms, &ctx->vk.static_staging_buffer);
		for (size_t tile_layer_idx = 0; tile_layer_idx < ctx->level.num_tile_layers; tile_layer_idx += 1) 
		{
			TileLayer* tile_layer = &ctx->level.tile_layers[tile_layer_idx];
			VulkanCopyBuffer(tile_layer->num_tiles*sizeof(Tile), tile_layer->tiles, &ctx->vk.static_staging_buffer);
		}

		SDL_assert(ctx->vk.static_staging_buffer.offset == ctx->vk.static_staging_buffer.size);
		VulkanUnmapBufferMemory(&ctx->vk, &ctx->vk.static_staging_buffer);

		SPALL_BUFFER_END();
	}

	// VulkanCreateImages
	{
		SPALL_BUFFER_BEGIN_NAME("VulkanCreateImages");

		VkMemoryRequirements mem_reqs[MAX_SPRITES];
		VkBindImageMemoryInfo bind_infos[MAX_SPRITES];
		VkDeviceSize memory_offset = 0;
		size_t i = 0;
		for (size_t sprite_idx = 0; sprite_idx < MAX_SPRITES; sprite_idx += 1) 
		{
			SpriteDesc* sd = GetSpriteDesc(ctx, (Sprite){sprite_idx});
			if (sd) 
			{
				VkImageCreateInfo info = 
				{
					.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
					.imageType = VK_IMAGE_TYPE_2D,
					.format = VK_FORMAT_R8G8B8A8_SRGB,
					.extent = (VkExtent3D){(uint32_t)sd->size.x, (uint32_t)sd->size.y, 1},
					.mipLevels = 1,
					.samples = VK_SAMPLE_COUNT_1_BIT,
					.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,
				};

				// Different frames might have different amounts of cells.
				for (size_t frame_idx = 0; frame_idx < sd->num_frames; frame_idx += 1) 
				{
					info.arrayLayers += (uint32_t)sd->frames[frame_idx].num_cells;
				}
				sd->vk_image_array_layers = (size_t)info.arrayLayers;

				VK_CHECK(vkCreateImage(ctx->vk.device, &info, NULL, &sd->vk_image));
				VulkanSetImageName(ctx->vk.device, sd->vk_image, sd->name);

				vkGetImageMemoryRequirements(ctx->vk.device, sd->vk_image, &mem_reqs[i]);
				memory_offset = AlignForward(memory_offset, mem_reqs[i].alignment);
				bind_infos[i] = (VkBindImageMemoryInfo)
				{
					.sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO,
					.image = sd->vk_image,
					//.memory = scroll down a little more!,
					.memoryOffset = memory_offset,
				};
				memory_offset += mem_reqs[i].size;
				i += 1;
			}
		}
		SDL_assert(i == ctx->num_sprites);

		VkMemoryAllocateInfo allocate_info = 
		{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = memory_offset,
			.memoryTypeIndex = VulkanGetMemoryTypeIdx(&ctx->vk, &mem_reqs[0], VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
		};
		VK_CHECK(vkAllocateMemory(ctx->vk.device, &allocate_info, NULL, &ctx->vk.image_memory));

		for (size_t i = 0; i < ctx->num_sprites; i += 1) 
		{
			bind_infos[i].memory = ctx->vk.image_memory;
		}
		VK_CHECK(vkBindImageMemory2(ctx->vk.device, (uint32_t)ctx->num_sprites, bind_infos));

		for (size_t sprite_idx = 0; sprite_idx < MAX_SPRITES; sprite_idx += 1) 
		{
			SpriteDesc* sd = GetSpriteDesc(ctx, (Sprite){sprite_idx});
			if (sd) 
			{
				bool is_tileset = false;
				is_tileset = sprite_idx == spr_tiles.idx;
				if (is_tileset) 
				{
					SDL_assert(sd->vk_image_array_layers == 1);
					VkImageViewCreateInfo info = 
					{
						.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
						.image = sd->vk_image,
						.viewType = VK_IMAGE_VIEW_TYPE_2D,
						.format = VK_FORMAT_R8G8B8A8_SRGB,
						.subresourceRange = 
						{
							.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
							.baseMipLevel = 0,
							.levelCount = 1,
							.baseArrayLayer = 0,
							.layerCount = 1,
						},
					};
					VK_CHECK(vkCreateImageView(ctx->vk.device, &info, NULL, &sd->vk_image_view));
				} 
				else 
				{
					VkImageViewCreateInfo info = 
					{
						.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
						.image = sd->vk_image,
						.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
						.format = VK_FORMAT_R8G8B8A8_SRGB,
						.subresourceRange = 
						{
							.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
							.baseMipLevel = 0,
							.levelCount = 1,
							.baseArrayLayer = 0,
							.layerCount = (uint32_t)sd->vk_image_array_layers,
						},
					};
					VK_CHECK(vkCreateImageView(ctx->vk.device, &info, NULL, &sd->vk_image_view));
				}
				VulkanSetImageViewName(ctx->vk.device, sd->vk_image_view, sd->name);
			}
		}

		SPALL_BUFFER_END();
	}

	// VulkanCreateDescriptorPool
	{
		SPALL_BUFFER_BEGIN_NAME("VulkanCreateDescriptorPool");

		VkDescriptorPoolSize sizes[] = {
			{
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				1,
			},
			{
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				(uint32_t)ctx->num_sprites,
			},
		};

		VkDescriptorPoolCreateInfo info =
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.maxSets = (uint32_t)ctx->num_sprites+1,
			.poolSizeCount = SDL_arraysize(sizes),
			.pPoolSizes = sizes,
		};

		VK_CHECK(vkCreateDescriptorPool(ctx->vk.device, &info, NULL, &ctx->vk.descriptor_pool));

		SPALL_BUFFER_END();
	}

	// VulkanCreateUniformBuffer
	{
		SPALL_BUFFER_BEGIN_NAME("VulkanCreateUniformBuffer");

		ctx->vk.uniform_buffer = VulkanCreateBuffer(&ctx->vk, sizeof(Uniforms), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VulkanSetBufferName(ctx->vk.device, ctx->vk.uniform_buffer.handle, "Uniform Buffer");

		SPALL_BUFFER_END();
	}

	// VulkanCreateDescriptorSets
	{
		SPALL_BUFFER_BEGIN_NAME("VulkanCreateDescriptorSets");

		const size_t descriptor_set_count = ctx->num_sprites + 1;

		VkDescriptorSetLayout* descriptor_set_layouts = StackAlloc(&ctx->stack, descriptor_set_count, VkDescriptorSetLayout);
		descriptor_set_layouts[0] = ctx->vk.descriptor_set_layout_uniforms;
		for (size_t i = 1; i < descriptor_set_count; i += 1) 
		{
			descriptor_set_layouts[i] = ctx->vk.descriptor_set_layout_sprites;
		}

		VkDescriptorSetAllocateInfo info =
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = ctx->vk.descriptor_pool,
			.descriptorSetCount = (uint32_t)descriptor_set_count,
			.pSetLayouts = descriptor_set_layouts,
		};

		VkDescriptorSet* descriptor_sets = StackAlloc(&ctx->stack, descriptor_set_count, VkDescriptorSet);
		VK_CHECK(vkAllocateDescriptorSets(ctx->vk.device, &info, descriptor_sets));

		VkDescriptorImageInfo* image_infos = StackAlloc(&ctx->stack, descriptor_set_count - 1, VkDescriptorImageInfo);

		// NOTE: For some strange reason, this array gets overwritten while writing to image_infos if we allocate it with the stack allocator.
		// I have tried using address sanitizer and it does not catch anything. Also the stack allocator isn't causing problems anywhere else,
		// which makes it strange that it seems to be causing problems here. I tried allocating more memory at startup, but that doesn't change anything
		// either. Whatever, we can just allocate it with malloc and it's fine.
		VkWriteDescriptorSet* writes = SDL_malloc(descriptor_set_count * sizeof(VkWriteDescriptorSet)); SDL_CHECK(writes); 

		ctx->vk.descriptor_set_uniforms = descriptor_sets[0];
		writes[0] = (VkWriteDescriptorSet)
		{
		    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		    .dstSet = ctx->vk.descriptor_set_uniforms,
		    .dstBinding = 0,
		    .descriptorCount = 1,
		    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		    .pBufferInfo = &(VkDescriptorBufferInfo){
		    	.buffer = ctx->vk.uniform_buffer.handle,
		    	.offset = 0,
		    	.range = ctx->vk.uniform_buffer.size,
		    },
		};

		size_t i = 1;
		for (size_t sprite_idx = 0; sprite_idx < MAX_SPRITES; sprite_idx += 1) 
		{
			SpriteDesc* sd = GetSpriteDesc(ctx, (Sprite){sprite_idx});
			if (sd) 
			{
				sd->vk_descriptor_set = descriptor_sets[i];

				image_infos[i-1] = (VkDescriptorImageInfo)
				{
					.sampler = ctx->vk.sampler,
					.imageView = sd->vk_image_view,
					.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				};

				writes[i] = (VkWriteDescriptorSet)
				{
				    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				    .dstSet = sd->vk_descriptor_set,
				    .dstBinding = 0,
				    .descriptorCount = 1,
				    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				    .pImageInfo = &image_infos[i-1],
				};

				i += 1;
			}
		}
		SDL_assert(i == descriptor_set_count);

		vkUpdateDescriptorSets(ctx->vk.device, (uint32_t)descriptor_set_count, writes, 0, NULL);

		SDL_free(writes);
		StackFree(&ctx->stack, image_infos);
		StackFree(&ctx->stack, descriptor_sets);
		StackFree(&ctx->stack, descriptor_set_layouts);

		SPALL_BUFFER_END();
	}

	// VulkanCreateDynamicStagingBuffer
	{
		SPALL_BUFFER_BEGIN_NAME("VulkanCreateDynamicStagingBuffer");

		VkDeviceSize size = 0;
		size += ctx->level.num_entities*sizeof(Instance)*2;
		ctx->vk.dynamic_staging_buffer = VulkanCreateBuffer(&ctx->vk, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		VulkanSetBufferName(ctx->vk.device, ctx->vk.dynamic_staging_buffer.handle, "Dynamic Staging Buffer");
		VulkanMapBufferMemory(&ctx->vk, &ctx->vk.dynamic_staging_buffer);

		SPALL_BUFFER_END();
	}

	// VulkanCreateVertexBuffer
	{
		SPALL_BUFFER_BEGIN_NAME("VulkanCreateVertexBuffer");

		size_t size = 0;
		size += ctx->level.num_entities*sizeof(Instance)*2;
		for (size_t tile_layer_idx = 0; tile_layer_idx < ctx->level.num_tile_layers; tile_layer_idx += 1) 
		{
			TileLayer* tile_layer = &ctx->level.tile_layers[tile_layer_idx];
			size += tile_layer->num_tiles*sizeof(Tile);
		}
		ctx->vk.vertex_buffer = VulkanCreateBuffer(&ctx->vk, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VulkanSetBufferName(ctx->vk.device, ctx->vk.vertex_buffer.handle, "Vertex Buffer");

		SPALL_BUFFER_END();
	}

	// InitReplayFrames
#if TOGGLE_REPLAY_FRAMES
	{
		ctx->c_replay_frames = 1024;
		ctx->replay_frames = SDL_malloc(ctx->c_replay_frames * sizeof(ReplayFrame)); SDL_CHECK(ctx->replay_frames);
	}
#endif // TOGGLE_REPLAY_FRAMES

	ResetGame(ctx);

	ctx->running = true;
	while (ctx->running) 
	{	
		// GetInput
		{
			SPALL_BUFFER_BEGIN_NAME("GetInput");

			if (!ctx->gamepad) 
			{
				int32_t joystick_count = 0;
				SDL_JoystickID* joysticks = SDL_GetGamepads(&joystick_count);
				if (joystick_count != 0) 
				{
					ctx->gamepad = SDL_OpenGamepad(joysticks[0]);
				}
			}

			ctx->button_jump = false;
			ctx->button_jump_released = false;
			ctx->button_attack = false;
			ctx->left_mouse_pressed = false;

			SDL_Event event;
			while (SDL_PollEvent(&event)) 
			{
				switch (event.type) 
				{
				case SDL_EVENT_KEY_DOWN:
					if (event.key.key == SDLK_ESCAPE) 
					{
						ctx->running = false;
					}
					if (!ctx->gamepad) 
					{
						switch (event.key.key) 
						{
						case SDLK_SPACE:
#if TOGGLE_REPLAY_FRAMES
							ctx->paused = !ctx->paused;
#endif // TOGGLE_REPLAY_FRAMES
							break;
						case SDLK_0:
							break;
						case SDLK_X:
							ctx->button_attack = true;
							break;
						case SDLK_LEFT:
							if (!event.key.repeat) 
							{
								ctx->button_left = 1;
							}
#if TOGGLE_REPLAY_FRAMES
							if (ctx->paused) 
							{
								SetReplayFrame(ctx, SDL_max(ctx->replay_frame_idx - 1, 0));
							}
#endif // TOGGLE_REPLAY_FRAMES
							break;
						case SDLK_RIGHT:
							if (!event.key.repeat) 
							{
								ctx->button_right = 1;
							}
#if TOGGLE_REPLAY_FRAMES
							if (ctx->paused) 
							{
								SetReplayFrame(ctx, SDL_min(ctx->replay_frame_idx + 1, ctx->replay_frame_idx_max - 1));
							}
#endif // TOGGLE_REPLAY_FRAMES
							break;
						case SDLK_UP:
							if (!event.key.repeat) 
							{
								ctx->button_jump = true;
							}
							break;
						case SDLK_DOWN:
							break;
						case SDLK_R:
							ResetGame(ctx);
							break;
						}
					}
					break;
				case SDL_EVENT_MOUSE_MOTION:
					ctx->mouse_pos.x = event.motion.x;
					ctx->mouse_pos.y = event.motion.y;
					break;
				case SDL_EVENT_MOUSE_BUTTON_DOWN:
					if (event.button.button == SDL_BUTTON_LEFT) 
					{
						ctx->left_mouse_pressed = true;
					}
					break;
				case SDL_EVENT_KEY_UP:
					if (!ctx->gamepad) 
					{
						switch (event.key.key) 
						{
						case SDLK_LEFT:
							ctx->button_left = 0;
							break;
						case SDLK_RIGHT:
							ctx->button_right = 0;
							break;
						case SDLK_UP:
							ctx->button_jump_released = true;
							break;
						}					
					}
					break;
				case SDL_EVENT_GAMEPAD_AXIS_MOTION:
					switch (event.gaxis.axis) 
					{
					case SDL_GAMEPAD_AXIS_LEFTX:
						ctx->gamepad_left_stick.x = NormInt16(event.gaxis.value);
						break;
					case SDL_GAMEPAD_AXIS_LEFTY:
						ctx->gamepad_left_stick.y = NormInt16(event.gaxis.value);
						break;
					}
					break;
				case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
					switch (event.gbutton.button) 
					{
					case SDL_GAMEPAD_BUTTON_SOUTH:
						ctx->button_jump = true;
						break;
					case SDL_GAMEPAD_BUTTON_WEST:
						ctx->button_attack = true;
						break;
					}
					break;
				case SDL_EVENT_GAMEPAD_BUTTON_UP:
					switch (event.gbutton.button) 
					{
					case SDL_GAMEPAD_BUTTON_SOUTH:
						ctx->button_jump_released = true;
						break;
					}
					break;
				case SDL_EVENT_QUIT:
					ctx->running = false;
					break;
				}		
			}

			if (SDL_fabs(ctx->gamepad_left_stick.x) > GAMEPAD_THRESHOLD) 
			{
				ctx->gamepad_left_stick.x = 0.0f;
			}

			SPALL_BUFFER_END();
		}
		bool paused;
#if TOGGLE_REPLAY_FRAMES
		paused = ctx->paused;
#else
		paused = false;
#endif // TOGGLE_REPLAY_FRAMES
		if (!paused) 
		{
			// UpdateGame
			{
				SPALL_BUFFER_BEGIN_NAME("UpdateGame");

				UpdatePlayer(ctx);
				size_t num_enemies; Entity* enemies = GetEnemies(ctx, &num_enemies);
				for (size_t enemy_idx = 0; enemy_idx < num_enemies; enemy_idx += 1) 
				{
					Entity* enemy = &enemies[enemy_idx];
					if (enemy->type == EntityType_Boar) 
					{
						UpdateBoar(ctx, enemy);
					} // else if (enemy->type == EntityType_) { }
				}

				SPALL_BUFFER_END();
			}

#if TOGGLE_REPLAY_FRAMES
			// RecordReplayFrame
			{
				SPALL_BUFFER_BEGIN_NAME("RecordReplayFrame");

				ReplayFrame replay_frame = {0};
				
				replay_frame.entities = SDL_malloc(ctx->level.num_entities * sizeof(Entity)); SDL_CHECK(replay_frame.entities);
				SDL_memcpy(replay_frame.entities, ctx->level.entities, ctx->level.num_entities * sizeof(Entity));
				replay_frame.num_entities = ctx->level.num_entities;
					
				ctx->replay_frames[ctx->replay_frame_idx++] = replay_frame;
				if (ctx->replay_frame_idx >= ctx->c_replay_frames - 1) 
				{
					ctx->c_replay_frames *= 8;
					ctx->replay_frames = SDL_realloc(ctx->replay_frames, ctx->c_replay_frames * sizeof(ReplayFrame)); SDL_CHECK(ctx->replay_frames);
				}
				ctx->replay_frame_idx_max = SDL_max(ctx->replay_frame_idx_max, ctx->replay_frame_idx);

				SPALL_BUFFER_END();
			}
#endif // TOGGLE_REPLAY_FRAMES
		}
		
		// VulkanCopyInstancesToDynamicStagingBuffer
		size_t num_instances;
		{
			SPALL_BUFFER_BEGIN_NAME("VulkanCopyInstancesToDynamicStagingBuffer");

			size_t num_entities; Entity* entities = GetEntities(ctx, &num_entities);
			num_instances = 0;
			for (size_t entity_idx = 0; entity_idx < num_entities; entity_idx += 1) 
			{
				Entity* entity = &entities[entity_idx];
				SpriteDesc* sd = GetSpriteDesc(ctx, entity->anim.sprite);
				num_instances += sd->frames[entity->anim.frame_idx].num_cells;
			}

			Instance* instances = StackAlloc(&ctx->stack, num_instances, Instance);

			for (size_t entity_idx = 0, instance_idx = 0; entity_idx < num_entities && instance_idx < num_instances; entity_idx += 1) 
			{
				Entity* entity = &entities[entity_idx];
				SpriteDesc* sd = GetSpriteDesc(ctx, entity->anim.sprite);
				size_t base_frame_idx = 0;
				for (size_t frame_idx = 0; frame_idx < entity->anim.frame_idx; frame_idx += 1) 
				{
					base_frame_idx += sd->frames[frame_idx].num_cells;
				}
				for (
					size_t cell_idx = 0; 
					cell_idx < sd->frames[entity->anim.frame_idx].num_cells && instance_idx < num_instances; 
					++cell_idx, ++instance_idx) 
				{
					Instance* instance = &instances[instance_idx];
					ivec2s origin = GetEntityOrigin(ctx, entity);
					instance->rect.min = glms_ivec2_sub(entity->pos, origin);
					instance->rect.max = glms_ivec2_add(instance->rect.min, sd->size);
					instance->anim_frame_idx = (int32_t)(base_frame_idx + cell_idx + 1)*entity->dir;
				}			
			}

			VulkanCopyBuffer(num_instances * sizeof(Instance), instances, &ctx->vk.dynamic_staging_buffer);
			VulkanResetBuffer(&ctx->vk.dynamic_staging_buffer);

			StackFree(&ctx->stack, instances);
			
			SPALL_BUFFER_END();
		}

		uint32_t image_idx;
		VkCommandBuffer cb;

		// DrawBegin
		{
			SPALL_BUFFER_BEGIN_NAME("DrawBegin");

			VK_CHECK(vkWaitForFences(ctx->vk.device, 1, &ctx->vk.frames[ctx->vk.current_frame].fence_in_flight, VK_TRUE, UINT64_MAX));
			VK_CHECK(vkResetFences(ctx->vk.device, 1, &ctx->vk.frames[ctx->vk.current_frame].fence_in_flight));

			VK_CHECK(vkAcquireNextImageKHR(ctx->vk.device, ctx->vk.swapchain, UINT64_MAX, ctx->vk.frames[ctx->vk.current_frame].sem_image_available, VK_NULL_HANDLE, &image_idx));
			if (image_idx == 0 && ctx->vk.staged && ctx->vk.static_staging_buffer.handle) 
			{
				VulkanDestroyBuffer(&ctx->vk, &ctx->vk.static_staging_buffer);

				SDL_ShowWindow(ctx->window);
			}

			cb = ctx->vk.frames[ctx->vk.current_frame].command_buffer;

			// VulkanBeginCommandBuffer
			{
				VkCommandBufferBeginInfo info = 
				{
					.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
					.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
				};
				VK_CHECK(vkBeginCommandBuffer(cb, &info));
			}

			// VulkanCopyStagingBufferToBuffers
			if (!ctx->vk.staged) 
			{
				ctx->vk.staged = true;

				VkImageMemoryBarrier* image_memory_barriers_before = StackAlloc(&ctx->stack, ctx->num_sprites, VkImageMemoryBarrier);
				VkImageMemoryBarrier* image_memory_barriers_after = StackAlloc(&ctx->stack, ctx->num_sprites, VkImageMemoryBarrier);
				size_t i = 0;
				for (size_t sprite_idx = 0; sprite_idx < MAX_SPRITES; sprite_idx += 1) 
				{
					SpriteDesc* sd = GetSpriteDesc(ctx, (Sprite){sprite_idx});
					if (!sd) continue;

					VkImageSubresourceRange subresource_range = 
					{
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.baseMipLevel = 0,
						.levelCount = 1,
						.baseArrayLayer = 0,
						.layerCount = (uint32_t)sd->vk_image_array_layers,
					};

					image_memory_barriers_before[i] = (VkImageMemoryBarrier)
					{
						.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
						.srcAccessMask = 0,
						.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
						.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
						.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						.image = sd->vk_image,
						.subresourceRange = subresource_range,
					};

					image_memory_barriers_after[i] = (VkImageMemoryBarrier)
					{
						.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
						.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
						.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
						.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
						.image = sd->vk_image,
						.subresourceRange = subresource_range,
					};

					i += 1;
				}
				SDL_assert(i == ctx->num_sprites);

				VkBufferMemoryBarrier buffer_memory_barriers_before[] = 
				{
					{
						.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
						.srcAccessMask = 0,
						.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
						.buffer = ctx->vk.static_staging_buffer.handle,
						.size = ctx->vk.static_staging_buffer.size,
					},
					{
						.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
						.srcAccessMask = 0,
						.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
						.buffer = ctx->vk.dynamic_staging_buffer.handle,
						.size = ctx->vk.dynamic_staging_buffer.size,
					},
					{
						.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
						.srcAccessMask = 0,
						.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
						.buffer = ctx->vk.vertex_buffer.handle,
						.size = ctx->vk.vertex_buffer.size,
					},
					{
						.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
						.srcAccessMask = 0,
						.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
						.buffer = ctx->vk.uniform_buffer.handle,
						.size = ctx->vk.uniform_buffer.size,
					},
				};
				vkCmdPipelineBarrier(cb, 
					VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 
					0, NULL, 
					SDL_arraysize(buffer_memory_barriers_before), buffer_memory_barriers_before, 
					(uint32_t)ctx->num_sprites, image_memory_barriers_before);
				
				for (size_t sprite_idx = 0; sprite_idx < MAX_SPRITES; sprite_idx += 1) 
				{
					SpriteDesc* sd = GetSpriteDesc(ctx, (Sprite){sprite_idx});
					if (!sd) continue;
					VkBufferImageCopy* regions = StackAlloc(&ctx->stack, sd->vk_image_array_layers, VkBufferImageCopy);
					size_t region_idx = 0;
					for (size_t frame_idx = 0; frame_idx < sd->num_frames; frame_idx += 1) 
					{
						SpriteFrame* sf = &sd->frames[frame_idx];
						for (size_t cell_idx = 0; cell_idx < sf->num_cells; cell_idx += 1) 
						{
							SpriteCell* cell = &sf->cells[cell_idx];
							SDL_assert(region_idx < sd->vk_image_array_layers);
							regions[region_idx] = (VkBufferImageCopy)
							{
								.bufferOffset = ctx->vk.static_staging_buffer.offset,
								.imageSubresource = (VkImageSubresourceLayers)
								{
									.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
									.mipLevel = 0,
									.baseArrayLayer = (uint32_t)region_idx,
									.layerCount = 1,
								},
								.imageOffset = (VkOffset3D)
								{
									.x = cell->origin.x,
									.y = cell->origin.y,
									.z = 0,
								},
								.imageExtent = (VkExtent3D)
								{
									.width = (uint32_t)cell->size.x,
									.height = (uint32_t)cell->size.y,
									.depth = 1,
								},
							};
							region_idx += 1;
							ctx->vk.static_staging_buffer.offset += cell->size.x*cell->size.y * sizeof(uint32_t);
						}
					}
					vkCmdCopyBufferToImage(cb, ctx->vk.static_staging_buffer.handle, sd->vk_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (uint32_t)sd->vk_image_array_layers, regions);
					StackFree(&ctx->stack, regions);
				}

				VulkanCmdCopyBuffer(cb, &ctx->vk.static_staging_buffer, &ctx->vk.uniform_buffer, UINT64_MAX);

				VulkanCmdCopyBuffer(cb, &ctx->vk.static_staging_buffer, &ctx->vk.vertex_buffer, UINT64_MAX);
				VkDeviceSize vertex_buffer_start = ctx->vk.vertex_buffer.offset;
				VulkanCmdCopyBuffer(cb, &ctx->vk.dynamic_staging_buffer, &ctx->vk.vertex_buffer, UINT64_MAX);
				ctx->vk.vertex_buffer.start = vertex_buffer_start;

				{
					VkBufferMemoryBarrier barrier = {
						.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
						.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
						.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
						.buffer = ctx->vk.vertex_buffer.handle,
						.size = ctx->vk.vertex_buffer.size,
					};

					vkCmdPipelineBarrier(cb, 
						VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 
						0, NULL, 
						1, &barrier, 
						0, NULL);
				}

				{
					VkBufferMemoryBarrier barrier = {
						.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
						.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
						.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT,
						.buffer = ctx->vk.uniform_buffer.handle,
						.size = ctx->vk.uniform_buffer.size,
					};

					vkCmdPipelineBarrier(cb, 
						VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 
						0, NULL,
						1, &barrier, 
						0, NULL);
				}				

				vkCmdPipelineBarrier(cb, 
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 
					0, NULL, 
					0, NULL, 
					(uint32_t)ctx->num_sprites, image_memory_barriers_after);

				StackFree(&ctx->stack, image_memory_barriers_after);				
				StackFree(&ctx->stack, image_memory_barriers_before);
			} 
			else 
			{
				VkBufferMemoryBarrier buffer_memory_barriers_before[] = 
				{
					{
						.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
						.srcAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
						.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
						.buffer = ctx->vk.vertex_buffer.handle,
						.size = ctx->vk.vertex_buffer.size,
					},
				};
				vkCmdPipelineBarrier(cb, 
					VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 
					0, NULL, 
					SDL_arraysize(buffer_memory_barriers_before), buffer_memory_barriers_before, 
					0, NULL);

				VulkanCmdCopyBuffer(cb, &ctx->vk.dynamic_staging_buffer, &ctx->vk.vertex_buffer, UINT64_MAX);

				VkBufferMemoryBarrier buffer_memory_barriers_after[] = 
				{
					{
						.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
						.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
						.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
						.buffer = ctx->vk.vertex_buffer.handle,
						.size = ctx->vk.vertex_buffer.size,
					},
				};
				vkCmdPipelineBarrier(cb, 
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 
					0, NULL, 
					SDL_arraysize(buffer_memory_barriers_after), buffer_memory_barriers_after, 
					0, NULL);
			}

			// VulkanBeginRenderPass
			{
				VkClearValue clear_value = {0};

				VkRenderPassBeginInfo info = {
					.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
					.renderPass = ctx->vk.render_pass,
					.framebuffer = ctx->vk.framebuffers[image_idx],
					.renderArea = { .extent = ctx->vk.swapchain_info.imageExtent },
					.clearValueCount = 1,
					.pClearValues = &clear_value,
				};
				vkCmdBeginRenderPass(cb, &info, VK_SUBPASS_CONTENTS_INLINE);
			}

			SPALL_BUFFER_END();
		}

		// Draw
		{
			SPALL_BUFFER_BEGIN_NAME("Draw");

			// DrawTiles
			{
				VkDeviceSize offset = 0;
				vkCmdBindVertexBuffers(cb, 0, 1, &ctx->vk.vertex_buffer.handle, &offset);

				vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->vk.pipelines[0]);

				SpriteDesc* sd =  GetSpriteDesc(ctx, spr_tiles);
				VkDescriptorSet descriptor_sets[] = {ctx->vk.descriptor_set_uniforms, sd->vk_descriptor_set};
				vkCmdBindDescriptorSets(cb, 
					VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->vk.pipeline_layout, 
					0, SDL_arraysize(descriptor_sets), descriptor_sets, 
					0, NULL);

				size_t num_tiles = 0;
				for (size_t layer_idx = 0; layer_idx < ctx->level.num_tile_layers; layer_idx += 1) 
				{
					num_tiles += ctx->level.tile_layers[layer_idx].num_tiles;
				}
				vkCmdDraw(cb, 6, (uint32_t)num_tiles, 0, 0);
			}
			// DrawEntities
			{
				vkCmdBindVertexBuffers(cb, 0, 1, &ctx->vk.vertex_buffer.handle, &ctx->vk.vertex_buffer.start);

				vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->vk.pipelines[1]);

				size_t num_entities; Entity* entities = GetEntities(ctx, &num_entities);
				
				// DrawPlayer
				SpriteDesc* sd = GetSpriteDesc(ctx, entities[0].anim.sprite);
				vkCmdBindDescriptorSets(cb, 
					VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->vk.pipeline_layout, 
					1, 1, &sd->vk_descriptor_set, 
					0, NULL);
				size_t num_instances_player = sd->frames[entities[0].anim.frame_idx].num_cells;
				vkCmdDraw(cb, 6, (uint32_t)num_instances_player, 0, 0);

				// DrawEnemies
				// See VulkanCopyInstancesToDynamicStagingBuffer to see how num_instances was defined.
				for (size_t
					first_instance = num_instances_player,
					num_instances_leftover = num_instances - num_instances_player,
					cur_num_instances = 0,
					entity_idx = 1;

					first_instance < num_instances && 
					num_instances_leftover > 0 &&
					entity_idx < num_entities;

					first_instance += cur_num_instances,
					num_instances_leftover -= cur_num_instances) 
				{
					Entity* entity = &entities[entity_idx];
					Sprite sprite = entity->anim.sprite;
					SpriteDesc* sd = GetSpriteDesc(ctx, sprite);

					cur_num_instances = sd->frames[entity->anim.frame_idx].num_cells;

					entity_idx += 1;
					while (
						entity_idx < num_entities && 
						cur_num_instances < num_instances_leftover && 
						SpritesEqual(sprite, entities[entity_idx].anim.sprite)) 
					{
						cur_num_instances += sd->frames[entities[entity_idx].anim.frame_idx].num_cells;
						entity_idx += 1;
					}

					vkCmdBindDescriptorSets(cb, 
						VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->vk.pipeline_layout, 
						1, 1, &sd->vk_descriptor_set, 
						0, NULL);
					vkCmdDraw(cb, 6, (uint32_t)cur_num_instances, 0, (uint32_t)first_instance);
				}
			}

			SPALL_BUFFER_END();
		}

		// DrawEnd
		{
			SPALL_BUFFER_BEGIN_NAME("DrawEnd");

			vkCmdEndRenderPass(cb);

			VK_CHECK(vkEndCommandBuffer(cb));

			VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

			VkSubmitInfo submit_info = 
			{
				.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
				.waitSemaphoreCount = 1,
				.pWaitSemaphores = &ctx->vk.frames[ctx->vk.current_frame].sem_image_available,
				.pWaitDstStageMask = &wait_stage,
				.commandBufferCount = 1,
				.pCommandBuffers = &cb,
				.signalSemaphoreCount = 1,
				.pSignalSemaphores = &ctx->vk.frames[ctx->vk.current_frame].sem_render_finished,
			};
			VK_CHECK(vkQueueSubmit(ctx->vk.graphics_queue, 1, &submit_info, ctx->vk.frames[ctx->vk.current_frame].fence_in_flight));

			VkPresentInfoKHR present_info = 
			{
				.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
				.waitSemaphoreCount = 1,
				.pWaitSemaphores = &ctx->vk.frames[ctx->vk.current_frame].sem_render_finished,
				.swapchainCount = 1,
				.pSwapchains = &ctx->vk.swapchain,
				.pImageIndices = &image_idx,
			};
			VK_CHECK(vkQueuePresentKHR(ctx->vk.graphics_queue, &present_info));

			ctx->vk.current_frame = (ctx->vk.current_frame + 1) % ctx->vk.num_frames;

			VulkanResetBuffer(&ctx->vk.dynamic_staging_buffer);
			VulkanResetBuffer(&ctx->vk.vertex_buffer);

			SPALL_BUFFER_END();
		}
	}

	// NOTE: If we don't do this, we might not get the last few events.
#if TOGGLE_PROFILING
	spall_buffer_quit(&ctx->spall_ctx, &ctx->spall_buffer);
	spall_quit(&ctx->spall_ctx);
#endif // TOGGLE_PROFILING

	// NOTE: If we don't do this, SDL might not reverse certain operations,
	// like changing the resolution of the monitor.
	SDL_Quit();

	return 0;
}