#define ENABLE_PROFILING 1
#define FULLSCREEN 1

#pragma warning(push, 0)
#include <SDL.h>
#include <SDL_main.h>
#include <SDL_vulkan.h>

#include <cglm/struct.h>

#define XXH_STATIC_LINKING_ONLY /* access advanced declarations */
#define XXH_IMPLEMENTATION      /* access definitions */
#include <xxhash.h>

#include <infl.h>

#include <cJson/cJson.h>

#include <Volk/volk.h>

#ifndef _DEBUG
#define RADDBG_MARKUP_STUBS
#endif
#define RADDBG_MARKUP_IMPLEMENTATION
#include <raddbg_markup.h>

#if ENABLE_PROFILING
#include <spall/spall.h>
#define SPALL_BUFFER_BEGIN_NAME(NAME) STMT( \
	SDL_Time time; \
	SDL_GetCurrentTime(&time); \
	spall_buffer_begin( \
		&ctx->spall_ctx, \
		&ctx->spall_buffer, \
		NAME, \
		sizeof(NAME) - 1, \
		time); \
)
#define SPALL_BUFFER_BEGIN() STMT( \
	SDL_Time time; \
	SDL_GetCurrentTime(&time); \
	spall_buffer_begin( \
		&ctx->spall_ctx, \
		&ctx->spall_buffer, \
		__FUNCTION__, \
		sizeof(__FUNCTION__) - 1, \
		time); \
)
#define SPALL_BUFFER_END() STMT( \
	SDL_Time time; \
	SDL_GetCurrentTime(&time); \
	spall_buffer_end( \
		&ctx->spall_ctx, \
		&ctx->spall_buffer, \
		time); \
)
#else
#define SPALL_BUFFER_BEGIN()
#endif
#pragma warning(pop)

typedef int64_t ssize_t;

#include "aseprite.h"

#define STMT(X) do {X} while (false)

#define FORCEINLINE SDL_FORCE_INLINE
#define function static

#define UNUSED(X) (void)X

#define HAS_FLAG(FLAGS, FLAG) ((FLAGS) & (FLAG)) // TODO: Figure out why I can't have multiple flags set in the second argument.
#define FLAG(X) (1u << X##u)

#define GetSprite(path) ((Sprite){HashString(path, 0) & (MAX_SPRITES - 1)})

#ifdef _DEBUG
#define SDL_CHECK(E) STMT( \
	if (!(E)) { \
		SDL_LogMessage(SDL_LOG_CATEGORY_ASSERT, SDL_LOG_PRIORITY_CRITICAL, "%s(%d): %s", __FILE__, __LINE__, SDL_GetError()); \
		SDL_TriggerBreakpoint(); \
	} \
)

#define VK_CHECK(E) STMT( SDL_assert((E) == VK_SUCCESS); )
#else
#define SDL_CHECK(E) E
#define VK_CHECK(E) E
#endif

#define SDL_ReadStruct(S, STRUCT) SDL_ReadIO(S, (STRUCT), sizeof(*(STRUCT)))

#define SDL_ReadIOChecked(S, P, SZ) SDL_CHECK(SDL_ReadIO(S, P, SZ) == SZ)
#define SDL_ReadStructChecked(S, STRUCT) SDL_CHECK(SDL_ReadStruct(S, STRUCT) == sizeof(*(STRUCT)))

#define GAME_WIDTH 960
#define GAME_HEIGHT 540

#define GAMEPAD_THRESHOLD 0.5f

#define PLAYER_ACC 0.02f
#define PLAYER_FRIC 0.006f
#define PLAYER_MAX_VEL 0.3f
#define PLAYER_JUMP 1.3f
#define PLAYER_JUMP_REMAINDER 0.2f

#define BOAR_ACC 0.01f
#define BOAR_FRIC 0.005f
#define BOAR_MAX_VEL 0.15f

#define TILE_SIZE 16
#define GRAVITY 0.009f

#define MAX_SPRITES 256

#define MIN_FPS 15

typedef struct SpriteLayer {
	char* name;
} SpriteLayer;

enum {
	SpriteCellType_Sprite,
	SpriteCellType_Hitbox,
	SpriteCellType_Hurtbox,
	SpriteCellType_Origin,
};
typedef uint32_t SpriteCellType;

typedef struct SpriteCell {
	ivec2s offset;
	ivec2s size;
	uint32_t layer_idx;
	uint32_t frame_idx;
	int32_t z_idx;
	SpriteCellType type;
	
	VkImage vk_image;
	// VkImageView vk_image_view;
	VkMemoryRequirements vk_mem_req;
} SpriteCell;

typedef struct SpriteFrame {
	double dur;
	SpriteCell* cells; size_t num_cells;
} SpriteFrame;

typedef struct SpriteDesc {
	char* path;
	ivec2s size;
	SpriteLayer* layers; size_t num_layers;
	SpriteFrame* frames; size_t num_frames;
} SpriteDesc;

typedef struct Sprite {
	int32_t idx;
} Sprite;

typedef struct Anim {
	Sprite sprite;
	int32_t frame_idx;
	float dt_accumulator;
	bool ended;
} Anim;

enum {
	EntityType_Player,
	EntityType_Boar,
};
typedef uint32_t EntityType;

enum {
	EntityState_Inactive,
	EntityState_Die,
	EntityState_Attack,
	EntityState_Fall,
	EntityState_Jump,
	EntityState_Free,
	
	// TODO
	EntityState_Hurt,
	
};
typedef uint32_t EntityState;

typedef struct Entity {
	Anim anim;

	// The position is the position of the origin.
	// The origin is defined by the current sprite.
	// Each sprite has its own origin relative to its canvas.
	ivec2s start_pos;
	ivec2s pos;
	vec2s pos_remainder;

	vec2s vel;
	
	int32_t dir;

	EntityType type;
	EntityState state;
} Entity;

typedef struct EntityVertex {
	ivec2s pos;
	int32_t dir;
	int32_t frame_idx;
} EntityVertex;

typedef struct Tile {
	ivec2s src;
	ivec2s dst;
} Tile;

typedef struct TileLayer {
	Tile* tiles;
	size_t num_tiles;
} TileLayer;

typedef struct Level {
	ivec2s size;
	Entity player;
	Entity* enemies; size_t num_enemies;
	TileLayer* tile_layers; size_t num_tile_layers;
	bool* tiles; // num_tiles = size.x*size.y/TILE_SIZE
} Level;

typedef struct Rect {
	ivec2s min;
	ivec2s max;
} Rect;

typedef struct ReplayFrame {
	Entity player;
	Entity* enemies; size_t num_enemies;
} ReplayFrame;

typedef struct Arena {
	uint8_t* first;
	uint8_t* last;
	uint8_t* cur;
} Arena;

typedef struct VulkanFrame {
	VkCommandBuffer command_buffer;
	VkSemaphore sem_image_available;
	VkSemaphore sem_render_finished;
	VkFence fence_in_flight;
} VulkanFrame;

#define PIPELINE_COUNT 2

typedef struct VulkanBuffer {
	VkBuffer handle;
	VkDeviceSize size;
	VkDeviceSize offset;
	VkDeviceMemory memory;
	void* mapped;
} VulkanBuffer;

typedef struct Vulkan {
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

	// TODO: Swapchain recreation.
	VkSwapchainKHR swapchain;
	VkSwapchainCreateInfoKHR swapchain_info;

	VkImage* swapchain_images;
	VkImageView* swapchain_image_views;
	VkFramebuffer* framebuffers;
	size_t num_swapchain_images;

	VkDescriptorSetLayout descriptor_set_layout;
	VkPipelineLayout pipeline_layout;
	VkPipeline pipelines[PIPELINE_COUNT];
	VkPipelineCache pipeline_cache;
	VkRenderPass render_pass;
	VkSampler sampler;

	VkCommandPool command_pool;

	VkDescriptorPool descriptor_pool;
	VkDescriptorSet descriptor_set;

	VkViewport viewport;
	VkRect2D scissor;

	VkImage depth_stencil_image;
	VkDeviceMemory depth_stencil_image_memory;
	VkImageView depth_stencil_image_view;
	
	VulkanFrame* frames; size_t num_frames;
	size_t current_frame;

	/*
	Memory layout:
		uint32_t raw_image_data[][];
		Tile tiles[];
	*/
	VulkanBuffer static_staging_buffer;
	SDL_IOStream* static_staging_buffer_stream; // The data in this will be copied over to the staging buffer.
	
	/*
	Memory layout:
		EntityVertex player;
		EntityVertex enemies[];
	*/
	VulkanBuffer dynamic_staging_buffer;

	/*
	Memory layout:
		EntityVertex player;
		EntityVertex enemies[];
		Tile tiles[];
	*/
	VulkanBuffer vertex_buffer;

	VkDeviceMemory image_memory;

	// invalid after startup
	VkImageMemoryBarrier* image_memory_barriers_before;
	VkImageMemoryBarrier* image_memory_barriers_after;
	
	VkImageMemoryBarrier* image_memory_barriers;

	bool staged;
} Vulkan;

typedef struct Context {
#if ENABLE_PROFILING
	SpallProfile spall_ctx;
	SpallBuffer spall_buffer;
#endif

	Arena arena;

	SDL_Window* window;
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
	
	Level* levels; size_t num_levels;
	size_t level_idx;

	SpriteDesc sprites[MAX_SPRITES];
	size_t num_sprite_cells; // not the same thing as the number of sprites

	Vulkan vk;

	ReplayFrame* replay_frames; 
	size_t replay_frame_idx; 
	size_t replay_frame_idx_max;
	size_t c_replay_frames;
	bool paused;
} Context;

typedef struct VkImageMemoryRequirements {
	VkMemoryRequirements memoryRequirements;
	VkImage image;
} VkImageMemoryRequirements;

#include "util.c"

static Sprite player_idle;
static Sprite player_run;
static Sprite player_jump_start;
static Sprite player_jump_end;
static Sprite player_attack;
static Sprite player_die;

static Sprite boar_idle;
static Sprite boar_walk;
static Sprite boar_run;
static Sprite boar_attack;
static Sprite boar_hit;

static Sprite spr_tiles;

static float dt;

function EntityVertex* GetEntityVertices(Level* level, size_t* num_entity_vertices) {
	SDL_assert(num_entity_vertices);
	*num_entity_vertices = 1 + level->num_enemies;
	EntityVertex* res = SDL_malloc((*num_entity_vertices) * sizeof(EntityVertex)); SDL_CHECK(res);
	res[0].pos = level->player.pos;
	res[0].dir = level->player.dir;
	res[0].frame_idx = level->player.anim.frame_idx;
	for (size_t i = 0; i < level->num_enemies; i += 1) {
		res[i+1].pos = level->enemies[i].pos;
		res[i+1].dir = level->enemies[i].dir;
		res[i+1].frame_idx = level->enemies[i].anim.frame_idx;
	}
	return res;
}

typedef bool (*EnumerateSpriteCellsCallback)(Context* ctx, SpriteCell* cell, size_t sprite_cell_idx, void* user_data);

function void EnumerateSpriteCells(Context* ctx, EnumerateSpriteCellsCallback callback, void* user_data) {
    SDL_assert(callback);
    size_t sprite_cell_idx = 0;
    for (size_t sprite_idx = 0; sprite_idx < MAX_SPRITES; sprite_idx += 1) {
        SpriteDesc* sd = GetSpriteDesc(ctx, (Sprite){(int32_t)sprite_idx});
        if (sd) {
            for (size_t frame_idx = 0; frame_idx < sd->num_frames; frame_idx += 1) {
                for (size_t cell_idx = 0; cell_idx < sd->frames[frame_idx].num_cells; cell_idx += 1) {
                    if (sd->frames[frame_idx].cells[cell_idx].type == SpriteCellType_Sprite) {
                        if (!callback(ctx, &sd->frames[frame_idx].cells[cell_idx], sprite_cell_idx, user_data)) {
                            return;
                        }
                        sprite_cell_idx += 1;
                    }
                }
            }
        }
    }
}

function bool VulkanCopyBufferToImageCallback(Context* ctx, SpriteCell* cell, size_t sprite_cell_idx, void* user_data) {
	UNUSED(sprite_cell_idx);
	VkBufferImageCopy* region = user_data;
	VkCommandBuffer cb = ctx->vk.frames[ctx->vk.current_frame].command_buffer;

	region->imageExtent = (VkExtent3D){cell->size.x, cell->size.y, 1};
	vkCmdCopyBufferToImage(cb, ctx->vk.static_staging_buffer.handle, cell->vk_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, region);
	region->bufferOffset += cell->size.x*cell->size.y * sizeof(uint32_t);

	return true;
}

function bool VulkanGetSpriteCellsImagesCallback(Context* ctx, SpriteCell* cell, size_t sprite_cell_idx, void* user_data) {
    UNUSED(ctx);
    VkImage* images = (VkImage*)user_data;
    images[sprite_cell_idx] = cell->vk_image;
    return true;
}

function VkImage* VulkanGetSpriteCellsImages(Context* ctx) {
	VkImage* images = SDL_malloc(ctx->num_sprite_cells * sizeof(VkImage)); SDL_CHECK(images);
	EnumerateSpriteCells(ctx, VulkanGetSpriteCellsImagesCallback, images);
	return images;
}

function bool VulkanGetSpriteCellsMemoryRequirementsCallback(Context* ctx, SpriteCell* cell, size_t sprite_cell_idx, void* user_data) {
    UNUSED(ctx);
    VkMemoryRequirements* mem_reqs = (VkMemoryRequirements*)user_data;
    mem_reqs[sprite_cell_idx] = cell->vk_mem_req;
    return true;
}

function VkMemoryRequirements* VulkanGetSpriteCellsMemoryRequirements(Context* ctx) {
    VkMemoryRequirements* mem_reqs = SDL_malloc(ctx->num_sprite_cells * sizeof(VkMemoryRequirements)); SDL_CHECK(mem_reqs);
    EnumerateSpriteCells(ctx, VulkanGetSpriteCellsMemoryRequirementsCallback, mem_reqs);
    return mem_reqs;
}

function uint32_t VulkanGetMemoryTypeIdx(Vulkan* vk, VkMemoryRequirements* mem_req) {
	return VulkanGetMemoryTypeIdxWithProperties(vk, mem_req, (VkMemoryPropertyFlags)-1);
}

function bool SetSprite(Entity* entity, Sprite sprite) {
    bool sprite_changed = false;
    if (!SpritesEqual(entity->anim.sprite, sprite)) {
        sprite_changed = true;
        entity->anim.sprite = sprite;
        ResetAnim(&entity->anim);
    }
    return sprite_changed;
}

function void ResetGame(Context* ctx) {
	SPALL_BUFFER_BEGIN();

	ctx->level_idx = 0;
	for (size_t level_idx = 0; level_idx < ctx->num_levels; level_idx += 1) {
		Level* level = &ctx->levels[level_idx];
		{
			Entity* player = &level->player;
			player->state = EntityState_Free;
			player->pos = player->start_pos;
			player->vel = (vec2s){0.0f};
			player->dir = 1;
			ResetAnim(&player->anim);
			SetSprite(player, player_idle);
		}
		for (size_t enemy_idx = 0; enemy_idx < level->num_enemies; enemy_idx += 1) {
			Entity* enemy = &level->enemies[enemy_idx];
			enemy->state = EntityState_Free;
			enemy->pos = enemy->start_pos;
			enemy->vel = (vec2s){0.0f};
			enemy->dir = 1;
			ResetAnim(&enemy->anim);
			if (enemy->type == EntityType_Boar) {
				SetSprite(enemy, boar_idle);
			}
		}
	}

	SPALL_BUFFER_END();
}

function ivec2s GetSpriteOrigin(Context* ctx, Sprite sprite, int32_t dir) {
	SPALL_BUFFER_BEGIN();
	ivec2s res = {0};

	// Find origin
	SpriteDesc* sd = GetSpriteDesc(ctx, sprite);
	SpriteFrame* frame = &sd->frames[0];
	for (size_t cell_idx = 0; cell_idx < frame->num_cells; cell_idx += 1) {
		SpriteCell* cell = &frame->cells[cell_idx];
		if (cell->type == SpriteCellType_Origin) {
			res = cell->offset;
			break;
		}
	}

	// Flip if necessary
	if (dir == -1) {
		res.x = sd->size.x - res.x;
	}

	SPALL_BUFFER_END();
	return res;
}

#if 0
function void DrawSprite(Context* ctx, Sprite sprite, size_t frame, vec2s pos, int32_t dir) {
	SPALL_BUFFER_BEGIN();

	SpriteDesc* sd = GetSpriteDesc(ctx, sprite);
	SDL_assert(sd->frames && "invalid sprite");
	SpriteFrame* sf = &sd->frames[frame];
	ivec2s origin = GetSpriteOrigin(ctx, sprite, dir);
	for (size_t cell_idx = 0; cell_idx < sf->num_cells; cell_idx += 1) {
		SpriteCell* cell = &sf->cells[cell_idx];

		// if (cell->texture) {
		// 	SDL_FRect srcrect = {
		// 		0.0f,
		// 		0.0f,
		// 		(float)(cell->size.x),
		// 		(float)(cell->size.y),
		// 	};

		// 	SDL_FRect dstrect = {
		// 		pos.x + (float)(cell->offset.x*dir - origin.x),
		// 		pos.y + (float)(cell->offset.y - origin.y),
		// 		(float)(cell->size.x*dir),
		// 		(float)(cell->size.y),
		// 	};

		// 	if (dir == -1) {
		// 		dstrect.x += (float)sd->size.x;
		// 	}

		// 	SDL_CHECK(SDL_RenderTexture(ctx->renderer, cell->texture, &srcrect, &dstrect));
		// }
	}

	SPALL_BUFFER_END();
}
#endif

function ivec2s GetTilesetDimensions(Context* ctx, Sprite tileset) {
	SpriteDesc* sd = GetSpriteDesc(ctx, tileset);
	SDL_assert(sd->num_layers == 1);
	SDL_assert(sd->num_frames == 1);
	SDL_assert(sd->frames[0].num_cells == 1);
	return sd->size;
}

function bool GetSpriteHitbox(Context* ctx, Sprite sprite, size_t frame_idx, int32_t dir, Rect* hitbox) {
	SPALL_BUFFER_BEGIN();
	bool res = false;

	SDL_assert(hitbox);
	SDL_assert(dir == 1 || dir == -1);
	SpriteDesc* sd = GetSpriteDesc(ctx, sprite);
	SDL_assert(frame_idx < sd->num_frames);
	SpriteFrame* frame = &sd->frames[frame_idx];
	for (size_t cell_idx = 0; cell_idx < frame->num_cells; cell_idx += 1) {
		SpriteCell* cell = &frame->cells[cell_idx];
		if (cell->type == SpriteCellType_Hitbox) {
			if (dir == 1) {
				hitbox->min.x = cell->offset.x;
				hitbox->max.x = cell->offset.x + cell->size.x;
				hitbox->min.y = cell->offset.y;
				hitbox->max.y = cell->offset.y + cell->size.y;
			} else {
				hitbox->min.x = -cell->offset.x - cell->size.x;
				hitbox->max.x = -cell->offset.x;
				hitbox->min.y = cell->offset.y;
				hitbox->max.y = cell->offset.y + cell->size.y;
				
				hitbox->min.x += sd->size.x;
				hitbox->max.x += sd->size.x;
			}

			// HACK
			hitbox->max.x -= 1;
			hitbox->max.y -= 1;

			ivec2s origin = GetSpriteOrigin(ctx, sprite, dir);
			hitbox->min = glms_ivec2_sub(hitbox->min, origin);
			hitbox->max = glms_ivec2_sub(hitbox->max, origin);

			res = true;
			break;
		}
	}

	SPALL_BUFFER_END();
	return res;
}

function void UpdateAnim(Context* ctx, Anim* anim, bool loop) {
	SPALL_BUFFER_BEGIN();

    SpriteDesc* sd = GetSpriteDesc(ctx, anim->sprite);
    SDL_assert(anim->frame_idx >= 0 && anim->frame_idx < (int32_t)sd->num_frames);
	double dur = sd->frames[anim->frame_idx].dur;
	size_t num_frames = sd->num_frames;

    if (loop || !anim->ended) {
        anim->dt_accumulator += dt;
        if (anim->dt_accumulator >= dur) {
            anim->dt_accumulator = 0.0;
            ++anim->frame_idx;
            if (anim->frame_idx >= (int32_t)num_frames) {
                if (loop) anim->frame_idx = 0;
                else {
                    anim->frame_idx -= 1;
                    anim->ended = true;
                }
            }
        }
    }

    SPALL_BUFFER_END();
}

function SDL_EnumerationResult EnumerateSpriteDirectory(void *userdata, const char *dirname, const char *fname) {
	Context* ctx = userdata;
	SPALL_BUFFER_BEGIN();

	// dirname\fname\file

	char dir_path[1024];
	SDL_CHECK(SDL_snprintf(dir_path, 1024, "%s%s", dirname, fname) >= 0);

	int32_t num_files;
	char** files = SDL_GlobDirectory(dir_path, "*.aseprite", 0, &num_files); SDL_CHECK(files);
	if (num_files > 0) {
		size_t raw_chunk_alloc_size = 4096ULL * 32ULL;
		void* raw_chunk = SDL_malloc(raw_chunk_alloc_size); SDL_CHECK(raw_chunk);

		for (size_t file_idx = 0; file_idx < (size_t)num_files; file_idx += 1) {
			char* file = files[file_idx];

			char sprite_path[2048];
			SDL_CHECK(SDL_snprintf(sprite_path, sizeof(sprite_path), "%s\\%s", dir_path, file) >= 0);
			SDL_CHECK(SDL_GetPathInfo(sprite_path, NULL));

			Sprite sprite = GetSprite(sprite_path);
			SpriteDesc* sd = GetSpriteDesc(ctx, sprite);
			SDL_assert(!sd && "Collision");
			sd = &ctx->sprites[sprite.idx];

			{
				size_t buf_len = SDL_strlen(sprite_path) + 1;
				sd->path = ArenaAlloc(&ctx->arena, (uint64_t)buf_len, char);
				SDL_strlcpy(sd->path, sprite_path, buf_len);
			}
			
			SDL_IOStream* fs = SDL_IOFromFile(sprite_path, "r"); SDL_CHECK(fs);
			
			// LoadSprite
			{
				SPALL_BUFFER_BEGIN_NAME("LoadSprite");
				/* 
				This loops through the frames in three passes. These are:
				1. Get layer count and for each frame, get cell count.
				2. Get layer names.
				3. Everything else, including decompressing texture data and uploading it to the GPU.
				*/

				ASE_Header header;
				SDL_ReadStructChecked(fs, &header);

				SDL_assert(header.magic_number == 0xA5E0);
				SDL_assert(header.color_depth == 32);
				SDL_assert((header.pixel_w == 0 || header.pixel_w == 1) && (header.pixel_h == 0 || header.pixel_h == 1));
				
				sd->size.x = (int32_t)header.w;
				sd->size.y = (int32_t)header.h;
				SDL_assert(header.grid_x == 0);
				SDL_assert(header.grid_y == 0);
				SDL_assert(header.grid_w == 0 || header.grid_w == 16);
				SDL_assert(header.grid_h == 0 || header.grid_h == 16);
				sd->num_frames = (size_t)header.num_frames;
				sd->frames = ArenaAlloc(&ctx->arena, sd->num_frames, SpriteFrame);

				int64_t fs_save = SDL_TellIO(fs); SDL_CHECK(fs_save != -1);

				for (size_t frame_idx = 0; frame_idx < sd->num_frames; frame_idx += 1) {
					ASE_Frame frame;
					SDL_ReadStructChecked(fs, &frame);
					SDL_assert(frame.magic_number == 0xF1FA);

					// Would mean this aseprite file is very old.
					SDL_assert(frame.num_chunks != 0);

					for (size_t chunk_idx = 0; chunk_idx < frame.num_chunks; chunk_idx += 1) {
						ASE_ChunkHeader chunk_header;
						SDL_ReadStructChecked(fs, &chunk_header);
						if (chunk_header.size == sizeof(ASE_ChunkHeader)) continue;

						size_t raw_chunk_size = chunk_header.size - sizeof(ASE_ChunkHeader);
						SDL_assert(raw_chunk_alloc_size >= raw_chunk_size);
						SDL_ReadIOChecked(fs, raw_chunk, raw_chunk_size);

						switch (chunk_header.type) {
						case ASE_ChunkType_Layer: {
							++sd->num_layers;
						} break;
						case ASE_ChunkType_Cell: {
							++sd->frames[frame_idx].num_cells;
						} break;
						}
					}
				}
				sd->layers = ArenaAlloc(&ctx->arena, sd->num_layers, SpriteLayer);
				
				for (size_t frame_idx = 0; frame_idx < sd->num_frames; frame_idx += 1) {
					if (sd->frames[frame_idx].num_cells > 0) {
						sd->frames[frame_idx].cells = ArenaAlloc(&ctx->arena, sd->frames[frame_idx].num_cells, SpriteCell);
					}
				}

				size_t layer_idx;

				SDL_CHECK(SDL_SeekIO(fs, fs_save, SDL_IO_SEEK_SET) != -1);
				layer_idx = 0;
				for (size_t frame_idx = 0; frame_idx < sd->num_frames; frame_idx += 1) {
					ASE_Frame frame;
					SDL_ReadStructChecked(fs, &frame);
					SDL_assert(frame.magic_number == 0xF1FA);

					// Would mean this aseprite file is very old.
					SDL_assert(frame.num_chunks != 0);

					for (size_t chunk_idx = 0; chunk_idx < frame.num_chunks; chunk_idx += 1) {
						ASE_ChunkHeader chunk_header;
						SDL_ReadStructChecked(fs, &chunk_header);
						if (chunk_header.size == sizeof(ASE_ChunkHeader)) continue;

						size_t raw_chunk_size = chunk_header.size - sizeof(ASE_ChunkHeader);
						SDL_ReadIOChecked(fs, raw_chunk, raw_chunk_size);

						switch (chunk_header.type) {
						case ASE_ChunkType_Layer: {
							// TODO: tileset_idx and uuid
							ASE_LayerChunk* chunk = raw_chunk;
							SpriteLayer sprite_layer = {0};
							if (chunk->layer_name.len > 0) {
								sprite_layer.name = ArenaAlloc(&ctx->arena, chunk->layer_name.len + 1, char);
								SDL_strlcpy(sprite_layer.name, (const char*)(chunk+1), chunk->layer_name.len + 1);
							}
							sd->layers[layer_idx++] = sprite_layer;
						} break;
						}
					}
				}

				SDL_CHECK(SDL_SeekIO(fs, fs_save, SDL_IO_SEEK_SET) != -1);
				layer_idx = 0;
				for (size_t frame_idx = 0; frame_idx < sd->num_frames; frame_idx += 1) {
					size_t cell_idx = 0;
					ASE_Frame frame;
					SDL_ReadStructChecked(fs, &frame);
					SDL_assert(frame.magic_number == 0xF1FA);

					// Would mean this aseprite file is very old.
					SDL_assert(frame.num_chunks != 0);

					sd->frames[frame_idx].dur = ((double)frame.frame_dur)/1000.0;

					for (size_t chunk_idx = 0; chunk_idx < frame.num_chunks; chunk_idx += 1) {
						ASE_ChunkHeader chunk_header;
						SDL_ReadStructChecked(fs, &chunk_header);
						if (chunk_header.size == sizeof(ASE_ChunkHeader)) continue;

						size_t raw_chunk_size = chunk_header.size - sizeof(ASE_ChunkHeader);
						SDL_ReadIOChecked(fs, raw_chunk, raw_chunk_size);

						switch (chunk_header.type) {
						case ASE_ChunkType_OldPalette: {
						} break;
						case ASE_ChunkType_OldPalette2: {
						} break;
						case ASE_ChunkType_Cell: {
							ASE_CellChunk* chunk = raw_chunk;
							SpriteCell cell = {
								.layer_idx = (uint32_t)layer_idx,
								.frame_idx = (uint32_t)frame_idx,
								.offset.x = chunk->x,
								.offset.y = chunk->y,
								.z_idx = chunk->z_idx,
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

									if (SDL_strcmp(sd->layers[chunk->layer_idx].name, "Hitbox") == 0) {
										cell.type = SpriteCellType_Hitbox;
									} else if (SDL_strcmp(sd->layers[chunk->layer_idx].name, "Origin") == 0) {
										cell.type = SpriteCellType_Origin;
									} else {
										// DecompressImageAndWriteToStream
										{
											// It's the zero-sized array at the end of ASE_CellChunk.
											size_t src_buf_size = raw_chunk_size - sizeof(ASE_CellChunk) - 2; 
											void* src_buf = (void*)((&chunk->compressed_image.h)+1);

											size_t dst_buf_size = cell.size.x*cell.size.y*sizeof(uint32_t);
											void* dst_buf = SDL_malloc(dst_buf_size); SDL_CHECK(dst_buf);

											SPALL_BUFFER_BEGIN_NAME("INFL_ZInflate");
											size_t res = INFL_ZInflate(dst_buf, dst_buf_size, src_buf, src_buf_size);
											SPALL_BUFFER_END();
											SDL_assert(res > 0);

											SDL_WriteIO(ctx->vk.static_staging_buffer_stream, dst_buf, dst_buf_size);
											SDL_free(dst_buf);
										}

										uint32_t queue_family_idx = 0;
										VkImageCreateInfo image_info = {
											.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
											.imageType = VK_IMAGE_TYPE_2D,
											.format = VK_FORMAT_R8G8B8A8_SRGB,
											.extent = {cell.size.x, cell.size.y, 1},
											.mipLevels = 1,
											.arrayLayers = 1,
											.samples = VK_SAMPLE_COUNT_1_BIT,
											.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,

											// TODO
											.queueFamilyIndexCount = 1,
											.pQueueFamilyIndices = &queue_family_idx,
										};
										VK_CHECK(vkCreateImage(ctx->vk.device, &image_info, NULL, &cell.vk_image));
										vkGetImageMemoryRequirements(ctx->vk.device, cell.vk_image, &cell.vk_mem_req);

										ctx->num_sprite_cells += 1;

										// VkImageViewCreateInfo image_view_info = {
										// 	.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
										// 	.image = cell.vk_image,
										// 	.viewType = VK_IMAGE_VIEW_TYPE_2D,
										// 	.format = image_info.format,
										// 	.subresourceRange = {
										// 		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, 
										// 		.baseMipLevel = 0, 
										// 		.levelCount = 1,
										// 		.baseArrayLayer = 0, 
										// 		.layerCount = 1,
										// 	},
										// };
										// VK_CHECK(vkCreateImageView(ctx->vk.device, &image_view_info, NULL, &cell.vk_image_view));
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
							for (size_t tag_idx = 0; tag_idx < (size_t)chunk->num_tags; tag_idx += 1) {
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
					}
				}

				SPALL_BUFFER_END();
			}

			SDL_CloseIO(fs);
		}

		SDL_free(raw_chunk);
	} else if (num_files == 0) {
		SDL_CHECK(SDL_EnumerateDirectory(dir_path, EnumerateSpriteDirectory, ctx));
	}
	
	SDL_free(files);
	SPALL_BUFFER_END();
	return SDL_ENUM_CONTINUE;
}

#if 0
function void DrawAnim(Context* ctx, Anim* anim, vec2s pos, int32_t dir) {
    DrawSprite(ctx, anim->sprite, anim->frame_idx, pos, dir);
}
#endif

#if 0
function void DrawEntity(Context* ctx, Entity* entity) {
    if (entity->state != EntityState_Inactive) {
        DrawAnim(ctx, &entity->anim, vec2_from_ivec2(entity->pos), entity->dir);
    }
}
#endif

function Rect GetEntityHitbox(Context* ctx, Entity* entity) {
	SPALL_BUFFER_BEGIN();
	Rect hitbox = {0};
	SpriteDesc* sd = GetSpriteDesc(ctx, entity->anim.sprite);

	/* 	
	I'll admit this part of the function is kind of weird. I might end up changing it later.
	The way it works is: we start from the current frame and go backward.
	For each frame, check if there is a corresponding hitbox. If so, pick that one.
	If no hitbox is found and the frame index is 0, loop forward instead.
	*/
	{
		bool res; ssize_t frame_idx;
		for (res = false, frame_idx = entity->anim.frame_idx; !res && frame_idx >= 0; frame_idx -= 1) {
			res = GetSpriteHitbox(ctx, entity->anim.sprite, (size_t)frame_idx, entity->dir, &hitbox); 
		}
		if (!res && entity->anim.frame_idx == 0) {
			for (frame_idx = 1; !res && frame_idx < (ssize_t)sd->num_frames; frame_idx += 1) {
				res = GetSpriteHitbox(ctx, entity->anim.sprite, (size_t)frame_idx, entity->dir, &hitbox);
			}
		}
		SDL_assert(res);
	}

	hitbox.min = glms_ivec2_add(hitbox.min, entity->pos);
	hitbox.max = glms_ivec2_add(hitbox.max, entity->pos);

	SPALL_BUFFER_END();
	return hitbox;
}

function bool EntitiesIntersect(Context* ctx, Entity* a, Entity* b) {
    if (a->state == EntityState_Inactive || b->state == EntityState_Inactive) return false;
    Rect ha = GetEntityHitbox(ctx, a);
    Rect hb = GetEntityHitbox(ctx, b);
    return RectsIntersect(ha, hb);
}

// This returns a new EntityState instead of setting the 
// entity state directly because depending on the entity,
// certain states might not make sense.
function EntityState EntityMoveAndCollide(Context* ctx, Entity* entity, vec2s acc, float fric, float max_vel) {
	SPALL_BUFFER_BEGIN();
	Level* level = GetCurrentLevel(ctx);
	Rect prev_hitbox = GetEntityHitbox(ctx, entity);

	entity->vel = glms_vec2_add(entity->vel, glms_vec2_scale(acc, dt));

	if (entity->state == EntityState_Free) {
		if (entity->vel.x < 0.0f) entity->vel.x = SDL_min(0.0f, entity->vel.x + fric);
		else if (entity->vel.x > 0.0f) entity->vel.x = SDL_max(0.0f, entity->vel.x - fric);
		entity->vel.x = SDL_clamp(entity->vel.x, -max_vel, max_vel);
	}

	bool horizontal_collision_happened = false;
	vec2s vel = entity->vel;
	if (entity->vel.x < 0.0f && prev_hitbox.min.x % TILE_SIZE == 0) {
		ivec2s grid_pos;
		grid_pos.x = (prev_hitbox.min.x-TILE_SIZE)/TILE_SIZE;
		for (grid_pos.y = prev_hitbox.min.y/TILE_SIZE; grid_pos.y <= prev_hitbox.max.y/TILE_SIZE; grid_pos.y += 1) {
			if (IsSolid(level, grid_pos)) {
				vel.x = 0.0f;
				horizontal_collision_happened = true;
				break;
			}
		}
	} else if (entity->vel.x > 0.0f && (prev_hitbox.max.x+1) % TILE_SIZE == 0) {
		ivec2s grid_pos;
		grid_pos.x = (prev_hitbox.max.x+1)/TILE_SIZE;
		for (grid_pos.y = prev_hitbox.min.y/TILE_SIZE; grid_pos.y <= prev_hitbox.max.y/TILE_SIZE; grid_pos.y += 1) {
			if (IsSolid(level, grid_pos)) {
				vel.x = 0.0f;
				horizontal_collision_happened = true;
				break;
			}
		}
	}
	bool touching_floor = false;
	bool vertical_collision_happened = false;
	if (entity->vel.y < 0.0f && prev_hitbox.min.y % TILE_SIZE == 0) {
		ivec2s grid_pos;
		grid_pos.y = (prev_hitbox.min.y-TILE_SIZE)/TILE_SIZE;
		for (grid_pos.x = prev_hitbox.min.x/TILE_SIZE; grid_pos.x <= prev_hitbox.max.x/TILE_SIZE; grid_pos.x += 1) {
			if (IsSolid(level, grid_pos)) {
				vel.y = 0.0f;
				vertical_collision_happened = true;
				break;
			}
		}
	} else if (entity->vel.y > 0.0f && (prev_hitbox.max.y+1) % TILE_SIZE == 0) {
		ivec2s grid_pos;
		grid_pos.y = (prev_hitbox.max.y+1)/TILE_SIZE;
		for (grid_pos.x = prev_hitbox.min.x/TILE_SIZE; grid_pos.x <= prev_hitbox.max.x/TILE_SIZE; grid_pos.x += 1) {
			if (IsSolid(level, grid_pos)) {
				vel.y = 0.0f;
				entity->vel.y = 0.0f;
				entity->state = EntityState_Free;
				touching_floor = true;
				vertical_collision_happened = true;
				break;
			}
		}
	}

    entity->pos_remainder = glms_vec2_add(entity->pos_remainder, glms_vec2_scale(vel, dt));
    entity->pos = glms_ivec2_add(entity->pos, ivec2_from_vec2(glms_vec2_round(entity->pos_remainder)));
    entity->pos_remainder = glms_vec2_sub(entity->pos_remainder, glms_vec2_round(entity->pos_remainder));

    Rect hitbox = GetEntityHitbox(ctx, entity);

	ivec2s grid_pos;
	for (grid_pos.y = hitbox.min.y/TILE_SIZE; 
		(!horizontal_collision_happened || !vertical_collision_happened) && grid_pos.y <= hitbox.max.y/TILE_SIZE; 
		grid_pos.y += 1) {
		for (grid_pos.x = hitbox.min.x/TILE_SIZE; 
			(!horizontal_collision_happened || !vertical_collision_happened) && grid_pos.x <= hitbox.max.x/TILE_SIZE;
			 grid_pos.x += 1) {
			if (IsSolid(level, grid_pos)) {
				Rect tile_rect;
				tile_rect.min = glms_ivec2_scale(grid_pos, TILE_SIZE);
				tile_rect.max = glms_ivec2_adds(tile_rect.min, TILE_SIZE);
				if (RectsIntersect(hitbox, tile_rect)) {
					if (RectsIntersect(prev_hitbox, tile_rect)) continue;

					if (!horizontal_collision_happened) {
						Rect h = prev_hitbox;
						h.min.x = hitbox.min.x;
						h.max.x = hitbox.max.x;
						if (RectsIntersect(h, tile_rect)) {
							int32_t amount = 0;
							int32_t incr = (int32_t)glm_signf(entity->vel.x);
							while (RectsIntersect(h, tile_rect)) {
								h.min.x -= incr;
								h.max.x -= incr;
								amount += incr;
							}
							entity->pos.x -= amount;
							horizontal_collision_happened = true;
						}

					}
					if (!vertical_collision_happened) {
						Rect h = prev_hitbox;
						h.min.y = hitbox.min.y;
						h.max.y = hitbox.max.y;
						if (RectsIntersect(h, tile_rect)) {
							int32_t amount = 0;
							int32_t incr = (int32_t)glm_signf(entity->vel.y);
							while (RectsIntersect(h, tile_rect)) {
								h.min.y -= incr;
								h.max.y -= incr;
								amount += incr;
							}
							entity->pos.y -= amount;					
							vertical_collision_happened = true;
						}
					}
				}
			}
		}
	}

	EntityState res;
	if (!touching_floor && entity->vel.y > 0.0f) {
		res = EntityState_Fall;
	} else {
		res = entity->state;
	}

	SPALL_BUFFER_END();
	return res;
}

function void UpdatePlayer(Context* ctx) {
	SPALL_BUFFER_BEGIN();
	Entity* player = GetPlayer(ctx);
	Level* level = GetCurrentLevel(ctx);

	int32_t input_dir = 0;
	if (ctx->gamepad) {
		if (ctx->gamepad_left_stick.x == 0.0f) input_dir = 0;
		else if (ctx->gamepad_left_stick.x > 0.0f) input_dir = 1;
		else if (ctx->gamepad_left_stick.x < 0.0f) input_dir = -1;
	} else {
		input_dir = ctx->button_right - ctx->button_left;
	}

	switch (player->state) {
	case EntityState_Free: {
		if (ctx->button_attack) {
			player->state = EntityState_Attack;
		} else if (ctx->button_jump) {
			player->state = EntityState_Jump;
		}
	} break;
	case EntityState_Jump: {
		if (ctx->button_jump_released) {
			player->vel.y /= 2.0f;
			player->state = EntityState_Fall;
		}
	} break;
	default: {
		// TODO?
	} break;
	}

	if (player->pos.y > (float)level->size.y) {
		ResetGame(ctx);
	} else switch (player->state) {
	case EntityState_Inactive:
		break;
    case EntityState_Die: {
		SetSprite(player, player_die);
		bool loop = false;
		UpdateAnim(ctx, &player->anim, loop);
		if (player->anim.ended) {
			ResetGame(ctx);
		}
	} break;

    case EntityState_Attack: {
		SetSprite(player, player_attack);

		size_t num_enemies; Entity* enemies = GetEnemies(ctx, &num_enemies);
		for (size_t enemy_idx = 0; enemy_idx < num_enemies; enemy_idx += 1) {
			Entity* enemy = &enemies[enemy_idx];
			if (EntitiesIntersect(ctx, player, enemy)) {
				switch (enemy->type) {
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
		if (player->anim.ended) {
			player->state = EntityState_Free;
		}
	} break;
    	
    case EntityState_Fall: {
    	SetSprite(player, player_jump_end);

    	vec2s acc = {0.0f, GRAVITY};
    	player->state = EntityMoveAndCollide(ctx, player, acc, PLAYER_FRIC, PLAYER_MAX_VEL);

    	bool loop = false;
    	UpdateAnim(ctx, &player->anim, loop);
	} break;
    	
	case EntityState_Jump: {
		vec2s acc = {0.0f};
		if (SetSprite(player, player_jump_start)) {
			acc.y -= PLAYER_JUMP;
		}

    	acc.y += GRAVITY;
    	player->state = EntityMoveAndCollide(ctx, player, acc, PLAYER_FRIC, PLAYER_MAX_VEL);

		bool loop = false;
    	UpdateAnim(ctx, &player->anim, loop);
	} break;

	case EntityState_Free: {
		vec2s acc = {0.0f, 0.0f};

		acc.y += GRAVITY;

		if (!ctx->gamepad) {
			acc.x = (float)input_dir * PLAYER_ACC;
		} else {
			acc.x = ctx->gamepad_left_stick.x * PLAYER_ACC;
		}

		if (input_dir == 0 && player->vel.x == 0.0f) {
			SetSprite(player, player_idle);
		} else {
			SetSprite(player, player_run);
			if (player->vel.x != 0.0f) {
				player->dir = (int32_t)glm_signf(player->vel.x);
			} else if (input_dir != 0) {
				player->dir = input_dir;
			}
		}

		player->state = EntityMoveAndCollide(ctx, player, acc, PLAYER_FRIC, PLAYER_MAX_VEL);		

		bool loop = true;
		UpdateAnim(ctx, &player->anim, loop);
	} break;

	default: {
		// TODO?
	} break;
	}

	SPALL_BUFFER_END();
}

function void UpdateBoar(Context* ctx, Entity* boar) {
	SPALL_BUFFER_BEGIN();

	switch (boar->state) {
	case EntityState_Hurt: {
		SetSprite(boar, boar_hit);

		bool loop = false;
		UpdateAnim(ctx, &boar->anim, loop);

		if (boar->anim.ended) {
			boar->state = EntityState_Inactive;
		}
	} break;

	case EntityState_Fall: {
		SetSprite(boar, boar_idle); // TODO: Is there a boar fall sprite? Could I make one?

		vec2s acc = {0.0f, GRAVITY};
		boar->state = EntityMoveAndCollide(ctx, boar, acc, BOAR_FRIC, BOAR_MAX_VEL);

		bool loop = true;
		UpdateAnim(ctx, &boar->anim, loop);
	} break;

	case EntityState_Free: {
		SetSprite(boar, boar_idle);

		vec2s acc = {0.0f, GRAVITY};
		boar->state = EntityMoveAndCollide(ctx, boar, acc, BOAR_FRIC, BOAR_MAX_VEL);

		bool loop = true;
		UpdateAnim(ctx, &boar->anim, loop);
	} break;

	default: {
		// TODO?
	} break;
	}

	SPALL_BUFFER_END();
}

function void SetReplayFrame(Context* ctx, size_t replay_frame_idx) {
	SPALL_BUFFER_BEGIN();

	ctx->replay_frame_idx = replay_frame_idx;
	Level* level = GetCurrentLevel(ctx);
	ReplayFrame* replay_frame = &ctx->replay_frames[ctx->replay_frame_idx];
	level->player = replay_frame->player;
	SDL_memcpy(level->enemies, replay_frame->enemies, replay_frame->num_enemies * sizeof(Entity));
	level->num_enemies = replay_frame->num_enemies;

	SPALL_BUFFER_END();
}

#ifdef _DEBUG
VkBool32 VKAPI_CALL VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT types, const VkDebugUtilsMessengerCallbackDataEXT* data, void* user_data) {
	UNUSED(severity);
	UNUSED(types);
	UNUSED(user_data);
	SDL_Log(data->pMessage);
	return VK_FALSE;
}
#endif

// TODO: Remove global variables.
// TODO: Platform specific extensions should only be added conditionally.

#ifdef _DEBUG
static char const * const g_vk_layers[] = { "VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_monitor" };
static char const * const g_vk_instance_extensions[] = { "VK_KHR_surface", "VK_KHR_win32_surface", "VK_EXT_debug_utils"};
static char const * const g_vk_device_extensions[] = { "VK_KHR_swapchain", /*"VK_NV_external_memory", "VK_NV_external_memory_win32"*/ };
#else
static char const * const g_vk_instance_extensions[] = { "VK_KHR_surface", "VK_KHR_win32_surface" };
static char const * const g_vk_device_extensions[] = { "VK_KHR_swapchain", /*"VK_NV_external_memory", "VK_NV_external_memory_win32"*/ };
#endif

static float const g_queue_priorities[] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};

int32_t main(int32_t argc, char* argv[]) {
	UNUSED(argc);
	UNUSED(argv);

	// SetSprites
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
		boar_attack = GetSprite("assets\\legacy_fantasy_high_forest\\Mob\\Boar\\Walk\\Walk-Base.aseprite");
		boar_hit = GetSprite("assets\\legacy_fantasy_high_forest\\Mob\\Boar\\Hit-Vanish\\Hit.aseprite");

		spr_tiles = GetSprite("assets\\legacy_fantasy_high_forest\\Assets\\Tiles.aseprite");
	}

	// InitContext
	Context* ctx;
	{
		SDL_CHECK(SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD));

		uint64_t memory_size = 4096ULL * 128ULL;

		Arena arena;
		arena.first = SDL_malloc(memory_size); SDL_CHECK(arena.first);
		arena.last = arena.first + memory_size;
		arena.cur = arena.first;

		ctx = ArenaAlloc(&arena, 1, Context);
		ctx->arena = arena;
	}

#if ENABLE_PROFILING
	{
		bool ok;

		ok = spall_init_file("profile/profile.spall", 1, &ctx->spall_ctx); SDL_assert(ok);

		int32_t buffer_size = 1 * 1024 * 1024;
		uint8_t* buffer = SDL_malloc(buffer_size); SDL_CHECK(buffer);
		ctx->spall_buffer = (SpallBuffer){
			.length = buffer_size,
			.data = buffer,
		};
		ok = spall_buffer_init(&ctx->spall_ctx, &ctx->spall_buffer); SDL_assert(ok);
	}
#endif

	// CreateWindow
	{
		SDL_DisplayID display = SDL_GetPrimaryDisplay();
		const SDL_DisplayMode* display_mode = SDL_GetDesktopDisplayMode(display); SDL_CHECK(display_mode);
		dt = 1.0f/display_mode->refresh_rate; // NOTE: After this, dt is effectively a constant.
	#if !FULLSCREEN
		ctx->window = SDL_CreateWindow("LegacyFantasy", display_mode->w/2, display_mode->h/2, SDL_WINDOW_VULKAN|SDL_WINDOW_HIGH_PIXEL_DENSITY); SDL_CHECK(ctx->window);
	#else
		ctx->window = SDL_CreateWindow("LegacyFantasy", display_mode->w, display_mode->h, SDL_WINDOW_VULKAN|SDL_WINDOW_HIGH_PIXEL_DENSITY|SDL_WINDOW_FULLSCREEN); SDL_CHECK(ctx->window);
	#endif

		PFN_vkGetInstanceProcAddr f = (PFN_vkGetInstanceProcAddr)SDL_Vulkan_GetVkGetInstanceProcAddr();
		volkInitializeCustom(f);
	}

	// VulkanCreateInstance
	{
		VkApplicationInfo app_info = { 
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pApplicationName = "Legacy Fantasy",
			.applicationVersion = VK_API_VERSION_1_0,
			.pEngineName = "Legacy Fantasy",
			.engineVersion = VK_API_VERSION_1_0,
			.apiVersion = VK_API_VERSION_1_1,
		};

		VkInstanceCreateInfo create_info = { 
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pApplicationInfo = &app_info,
	#ifdef _DEBUG
			.enabledLayerCount = SDL_arraysize(g_vk_layers),
			.ppEnabledLayerNames = g_vk_layers,
	#endif
			.enabledExtensionCount = SDL_arraysize(g_vk_instance_extensions),
			.ppEnabledExtensionNames = g_vk_instance_extensions,
		};

	#ifdef _DEBUG
		VkDebugUtilsMessengerCreateInfoEXT debug_info = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT,
			.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			.pfnUserCallback = VulkanDebugCallback,
			.pUserData = ctx,
		};

		create_info.pNext = &debug_info;
	#endif

		VK_CHECK(vkCreateInstance(&create_info, NULL, &ctx->vk.instance));
		volkLoadInstanceOnly(ctx->vk.instance);
	}

	// VulkanGetInstanceLayers
	{
		uint32_t count;
		VK_CHECK(vkEnumerateInstanceLayerProperties(&count, NULL));
		ctx->vk.num_instance_layers = (size_t)count;
		ctx->vk.instance_layers = ArenaAlloc(&ctx->arena, ctx->vk.num_instance_layers, VkLayerProperties);
		VK_CHECK(vkEnumerateInstanceLayerProperties(&count, ctx->vk.instance_layers));
	}

	// VulkanGetInstanceExtensions
	{
		uint32_t count;

		VK_CHECK(vkEnumerateInstanceExtensionProperties(NULL, &count, NULL));
		for (size_t layer_idx = 0; 
			layer_idx < ctx->vk.num_instance_layers;
			++layer_idx, ctx->vk.num_instance_extensions += (size_t)count) {
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
			++layer_idx, extension_idx += (size_t)count) {
			VK_CHECK(vkEnumerateInstanceExtensionProperties(
				ctx->vk.instance_layers[layer_idx].layerName, 
				&count, 
				NULL));
			VK_CHECK(vkEnumerateInstanceExtensionProperties(
				ctx->vk.instance_layers[layer_idx].layerName, 
				&count, 
				&ctx->vk.instance_extensions[extension_idx]));
		}
	}

	// VulkanGetPhysicalDevice
	{
		// HACK: This only happens to work and make sense on my laptop!.
		uint32_t physical_device_count = 2;
		VkPhysicalDevice physical_devices[2];
		VK_CHECK(vkEnumeratePhysicalDevices(ctx->vk.instance, &physical_device_count, physical_devices));
		ctx->vk.physical_device = physical_devices[1];

		vkGetPhysicalDeviceProperties(ctx->vk.physical_device, &ctx->vk.physical_device_properties);
		vkGetPhysicalDeviceMemoryProperties(ctx->vk.physical_device, &ctx->vk.physical_device_memory_properties);
	}

	// VulkanGetDeviceExtensions
	{
		uint32_t count;

		VK_CHECK(vkEnumerateDeviceExtensionProperties(ctx->vk.physical_device, NULL, &count, NULL));
		size_t layer_idx;
		for (layer_idx = 0, ctx->vk.num_device_extensions = (size_t)count; 
			layer_idx < ctx->vk.num_instance_layers;
			++layer_idx, ctx->vk.num_device_extensions += (size_t)count) {
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
			++layer_idx, extension_idx += (size_t)count) {
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
	}

	// VulkanCreateSurface
	{
		SDL_CHECK(SDL_Vulkan_CreateSurface(ctx->window, ctx->vk.instance, NULL, &ctx->vk.surface));
		VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->vk.physical_device, ctx->vk.surface, &ctx->vk.surface_capabilities));
	}

	// VulkanGetPhysicalDeviceSurfaceFormats
	{
		uint32_t count;
		VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->vk.physical_device, ctx->vk.surface, &count, NULL));
		ctx->vk.num_surface_formats = (size_t)count;
		ctx->vk.surface_formats = ArenaAlloc(&ctx->arena, ctx->vk.num_surface_formats, VkSurfaceFormatKHR);
		VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->vk.physical_device, ctx->vk.surface, &count, ctx->vk.surface_formats));
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

		VkPhysicalDeviceFeatures2 physical_device_features = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
		};
		vkGetPhysicalDeviceFeatures2(ctx->vk.physical_device, &physical_device_features);

		VkDeviceQueueCreateInfo* queue_infos = SDL_malloc(ctx->vk.num_queue_family_properties * sizeof(VkDeviceQueueCreateInfo)); SDL_CHECK(queue_infos);
		size_t num_queue_infos = 0;
		size_t num_queues = 0;
		for (size_t queue_family_idx = 0; queue_family_idx < ctx->vk.num_queue_family_properties; queue_family_idx += 1) {
			if (ctx->vk.queue_family_properties[queue_family_idx].queueCount == 0) continue;
			queue_infos[num_queue_infos] = (VkDeviceQueueCreateInfo){
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.queueFamilyIndex = (uint32_t)queue_family_idx,
				.queueCount = ctx->vk.queue_family_properties[queue_family_idx].queueCount,
				.pQueuePriorities = g_queue_priorities,
			};
			num_queue_infos += 1;
			num_queues += (size_t)ctx->vk.queue_family_properties[queue_family_idx].queueCount;
		}

		VkDeviceCreateInfo device_info = { 
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pNext = &physical_device_features,
			.queueCreateInfoCount = (uint32_t)num_queue_infos,
			.pQueueCreateInfos = queue_infos,
			.enabledExtensionCount = SDL_arraysize(g_vk_device_extensions),
			.ppEnabledExtensionNames = g_vk_device_extensions,
		};
		VK_CHECK(vkCreateDevice(ctx->vk.physical_device, &device_info, NULL, &ctx->vk.device));
		volkLoadDevice(ctx->vk.device);

		ctx->vk.queues = ArenaAlloc(&ctx->arena, num_queues, VkQueue);
		size_t queue_array_idx = 0;
		for (size_t queue_family_idx = 0; queue_family_idx < ctx->vk.num_queue_family_properties; queue_family_idx += 1) {
			for (uint32_t queue_idx = 0; queue_idx < ctx->vk.queue_family_properties[queue_family_idx].queueCount; queue_idx += 1) {
				vkGetDeviceQueue(ctx->vk.device, (uint32_t)queue_family_idx, queue_idx, &ctx->vk.queues[queue_array_idx]);
				++queue_array_idx;
			}
		}

		ctx->vk.graphics_queue = ctx->vk.queues[0]; // TODO

		SDL_free(queue_infos);
	}

	// VulkanCreateSwapchain
	{
		ctx->vk.swapchain_info = (VkSwapchainCreateInfoKHR){ 
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.surface = ctx->vk.surface,
			.minImageCount = SDL_min(ctx->vk.surface_capabilities.minImageCount + 1, ctx->vk.surface_capabilities.maxImageCount),
			.imageFormat = VK_FORMAT_R8G8B8A8_UNORM, // TODO
			.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR, // TODO
			.imageExtent = ctx->vk.surface_capabilities.currentExtent,
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			.preTransform = ctx->vk.surface_capabilities.currentTransform,
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode = VK_PRESENT_MODE_FIFO_KHR,
			.clipped = VK_TRUE,
		};
		
		VK_CHECK(vkCreateSwapchainKHR(ctx->vk.device, &ctx->vk.swapchain_info, NULL, &ctx->vk.swapchain));
	}

	// VulkanGetSwapchainImages
	{
		uint32_t count;
		VK_CHECK(vkGetSwapchainImagesKHR(ctx->vk.device, ctx->vk.swapchain, &count, NULL));
		ctx->vk.num_swapchain_images = (size_t)count;
		ctx->vk.swapchain_images = ArenaAlloc(&ctx->arena, ctx->vk.num_swapchain_images, VkImage);
		VK_CHECK(vkGetSwapchainImagesKHR(ctx->vk.device, ctx->vk.swapchain, &count, ctx->vk.swapchain_images));
	}

	// VulkanCreateSwapchainImageView
	{
		ctx->vk.swapchain_image_views = ArenaAlloc(&ctx->arena, ctx->vk.num_swapchain_images, VkImageView);
		for (size_t swapchain_image_idx = 0; swapchain_image_idx < ctx->vk.num_swapchain_images; swapchain_image_idx += 1) {
			VkImageViewCreateInfo info = { 
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = ctx->vk.swapchain_images[swapchain_image_idx],
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = ctx->vk.swapchain_info.imageFormat,
				.subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1},
			};
			VK_CHECK(vkCreateImageView(ctx->vk.device, &info, NULL, &ctx->vk.swapchain_image_views[swapchain_image_idx]));
		}
	}

	// VulkanCreateDepthStencilImage
	{
		VkImageCreateInfo info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = VK_FORMAT_D32_SFLOAT_S8_UINT, // TODO
			.extent = {.width = ctx->vk.swapchain_info.imageExtent.width, .height = ctx->vk.swapchain_info.imageExtent.height, .depth = 1 },
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		};
		VK_CHECK(vkCreateImage(ctx->vk.device, &info, NULL, &ctx->vk.depth_stencil_image));
	}

	// VulkanCreateDepthStencilImageMemory
	{
		VkImageMemoryRequirementsInfo2 info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2,
			.image = ctx->vk.depth_stencil_image,
		};
		VkMemoryDedicatedRequirements dedicated_mem_req = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS,
		};
		VkMemoryRequirements2 mem_req = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
			.pNext = &dedicated_mem_req,
		};
		vkGetImageMemoryRequirements2(ctx->vk.device, &info, &mem_req);

		uint32_t memory_type_idx = VulkanGetMemoryTypeIdx(&ctx->vk, &mem_req.memoryRequirements);		
		VkMemoryAllocateInfo mem_info = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = mem_req.memoryRequirements.size,
			.memoryTypeIndex = memory_type_idx,
		};
		VK_CHECK(vkAllocateMemory(ctx->vk.device, &mem_info, NULL, &ctx->vk.depth_stencil_image_memory));

		VkBindImageMemoryInfo bind_info = {
			.sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO,
			.image = ctx->vk.depth_stencil_image,
			.memory = ctx->vk.depth_stencil_image_memory,
		};
		VK_CHECK(vkBindImageMemory2(ctx->vk.device, 1, &bind_info));
	}

	// VulkanCreateDepthStencilImageView
	{
		VkImageViewCreateInfo info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = ctx->vk.depth_stencil_image,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = VK_FORMAT_D32_SFLOAT_S8_UINT, // TODO
			.subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, .levelCount = 1, .layerCount = 1 },
		};

		VK_CHECK(vkCreateImageView(ctx->vk.device, &info, NULL, &ctx->vk.depth_stencil_image_view));
	}

	// VulkanAllocateFrames
	{
		ctx->vk.num_frames = ctx->vk.num_swapchain_images;
		ctx->vk.frames = ArenaAlloc(&ctx->arena, ctx->vk.num_frames, VulkanFrame);
	}

	// VulkanCreateCommandPool
	{
		VkCommandPoolCreateInfo info = { 
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		};
	
		// TODO: Search for graphics queue family index.
		VK_CHECK(vkCreateCommandPool(ctx->vk.device, &info, NULL, &ctx->vk.command_pool));
	}

	// VulkanAllocateCommandBuffers
	{
		VkCommandBufferAllocateInfo info = { 
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = ctx->vk.command_pool,
			.commandBufferCount = (uint32_t)ctx->vk.num_frames,
		};

		VkCommandBuffer* command_buffers = SDL_malloc(ctx->vk.num_frames * sizeof(VkCommandBuffer)); SDL_CHECK(command_buffers);
		VK_CHECK(vkAllocateCommandBuffers(ctx->vk.device, &info, command_buffers));

		for (size_t i = 0; i < ctx->vk.num_frames; i += 1) {
			ctx->vk.frames[i].command_buffer = command_buffers[i];			
		}

		SDL_free(command_buffers);
	}

	// VulkanCreateFences
	{
		VkFenceCreateInfo info = { 
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT,
		};

		for (size_t i = 0; i < ctx->vk.num_frames; i += 1) {
			VK_CHECK(vkCreateFence(ctx->vk.device, &info, NULL, &ctx->vk.frames[i].fence_in_flight));
		}
	}

	// VulkanCreateSemaphores
	{
		VkSemaphoreCreateInfo info = { 
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		};

		for (size_t i = 0; i < ctx->vk.num_frames; i += 1) {
			VK_CHECK(vkCreateSemaphore(ctx->vk.device, &info, NULL, &ctx->vk.frames[i].sem_image_available));
			VK_CHECK(vkCreateSemaphore(ctx->vk.device, &info, NULL, &ctx->vk.frames[i].sem_render_finished));
		}
	}

	// VulkanCreateDescriptorSetLayout
	{
		VkDescriptorSetLayoutBinding binding = {
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		};

		VkDescriptorSetLayoutCreateInfo info = { 
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = 1,
			.pBindings = &binding,
		};
		
		VK_CHECK(vkCreateDescriptorSetLayout(ctx->vk.device, &info, NULL, &ctx->vk.descriptor_set_layout));
	}

	// VulkanCreatePipelineLayout
	{
		VkPipelineLayoutCreateInfo pipeline_layout_info = { 
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = 1,
			.pSetLayouts = &ctx->vk.descriptor_set_layout,
		};

		VK_CHECK(vkCreatePipelineLayout(ctx->vk.device, &pipeline_layout_info, NULL, &ctx->vk.pipeline_layout));
	}

	// VulkanCreateRenderPass
	{
		VkAttachmentDescription color_attachment = {
			.format = ctx->vk.swapchain_info.imageFormat,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		};

		VkAttachmentReference color_attachment_ref = {
			.attachment = 0, // index in attachments array
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		};

		VkAttachmentDescription depth_attachment = {
			.format = VK_FORMAT_D32_SFLOAT_S8_UINT, // TODO
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		};

		VkAttachmentReference depth_attachment_ref = {
			.attachment = 1,
			.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		};

		VkSubpassDescription subpass = {
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = &color_attachment_ref,
			.pDepthStencilAttachment = &depth_attachment_ref,
		};

		VkSubpassDependency subpass_dependency = {
			.srcSubpass = VK_SUBPASS_EXTERNAL, // implicit subpass before and after
			.dstSubpass = 0, // subpass index
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		};

		VkAttachmentDescription attachments[] = { color_attachment, depth_attachment };

		VkRenderPassCreateInfo info = { 
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = SDL_arraysize(attachments),
			.pAttachments = attachments,
			.subpassCount = 1,
			.pSubpasses = &subpass,
			.dependencyCount = 1,
			.pDependencies = &subpass_dependency,
		};

		VK_CHECK(vkCreateRenderPass(ctx->vk.device, &info, NULL, &ctx->vk.render_pass));
	}

	// VulkanCreateFramebuffers
	{
		ctx->vk.framebuffers = ArenaAlloc(&ctx->arena, ctx->vk.num_swapchain_images, VkFramebuffer);
		for (size_t i = 0; i < ctx->vk.num_swapchain_images; i += 1) {
			VkImageView attachments[] = {
				ctx->vk.swapchain_image_views[i],
				ctx->vk.depth_stencil_image_view,
			};

			VkFramebufferCreateInfo info = {
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
	}

	// VulkanCreatePipelineCache
	{
		VkPipelineCacheCreateInfo info = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
		};
		VK_CHECK(vkCreatePipelineCache(ctx->vk.device, &info, NULL, &ctx->vk.pipeline_cache));
	}

	// VulkanCreateGraphicsPipelines
	{
		VkDynamicState dynamic_states[] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
		};
		VkPipelineDynamicStateCreateInfo dynamic_state_info = { 
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.dynamicStateCount = SDL_arraysize(dynamic_states),
			.pDynamicStates = dynamic_states,
		};
		VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		};
		ctx->vk.viewport = (VkViewport){
			.width = (float)ctx->vk.swapchain_info.imageExtent.width,
			.height = (float)ctx->vk.swapchain_info.imageExtent.height,
			.maxDepth = 1.0f,
		};
		ctx->vk.scissor.extent = ctx->vk.swapchain_info.imageExtent;
		VkPipelineViewportStateCreateInfo viewport_info = { 
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.viewportCount = 1,
			.scissorCount = 1,
		};
		VkPipelineRasterizationStateCreateInfo rasterization_info = { 
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.cullMode = VK_CULL_MODE_BACK_BIT,
			.frontFace = VK_FRONT_FACE_CLOCKWISE,
			.lineWidth = 1.0f
		};
		VkPipelineMultisampleStateCreateInfo multisample_info = { 
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		};
		VkPipelineColorBlendAttachmentState blend_attachment_info = { 
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		};
		VkPipelineDepthStencilStateCreateInfo depth_stencil_info = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.depthTestEnable = VK_TRUE,
			.depthWriteEnable = VK_TRUE,
			.depthCompareOp = VK_COMPARE_OP_LESS,
		};
		VkPipelineColorBlendStateCreateInfo blend_info = { 
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.attachmentCount = 1,
			.pAttachments = &blend_attachment_info,
		};

		VkGraphicsPipelineCreateInfo graphics_pipline_info = { 
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		#ifdef _DEBUG
			.flags = VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT,
		#endif
			.pInputAssemblyState = &input_assembly_info,
			.pViewportState = &viewport_info,
			.pRasterizationState = &rasterization_info,
			.pMultisampleState = &multisample_info,
			.pDepthStencilState = &depth_stencil_info,
			.pColorBlendState = &blend_info,
			.pDynamicState = &dynamic_state_info,
			.layout = ctx->vk.pipeline_layout,
			.renderPass = ctx->vk.render_pass,
		};

		// VulkanCreateGraphicsPipelineTile
		VkPipelineShaderStageCreateInfo tile_vert = VulkanCreateShaderStage(ctx->vk.device, "build/shaders/tile_vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		VkPipelineShaderStageCreateInfo tile_frag = VulkanCreateShaderStage(ctx->vk.device, "build/shaders/tile_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		VkPipelineShaderStageCreateInfo tile_shader_stages[] = {
			tile_vert,
			tile_frag,
		};
		VkVertexInputBindingDescription tile_vertex_input_bindings[] = 
		{
			{
				.binding = 0,
				.stride = sizeof(Tile),
				.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
			},
		};
		VkVertexInputAttributeDescription tile_vertex_attributes[] = {
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
		VkPipelineVertexInputStateCreateInfo tile_vertex_input_info = { 
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
		VkPipelineShaderStageCreateInfo entity_shader_stages[] = {
			entity_vert,
			entity_frag,
		};
		VkVertexInputBindingDescription entity_vertex_input_bindings[] = 
		{
			{
				.binding = 0,
				.stride = sizeof(EntityVertex),
				.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
			},
		};
		VkVertexInputAttributeDescription entity_vertex_attributes[] = {
			{
				.location = 0,
				.binding = 0,
				.format = VK_FORMAT_R32G32B32A32_SINT,
				.offset = offsetof(EntityVertex, pos),
			},
		};
		VkPipelineVertexInputStateCreateInfo entity_vertex_input_info = {
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

		VkGraphicsPipelineCreateInfo infos[PIPELINE_COUNT] = {
			tile_graphics_pipeline_info,
			entity_graphics_pipeline_info,
		};

		VK_CHECK(vkCreateGraphicsPipelines(ctx->vk.device, ctx->vk.pipeline_cache, SDL_arraysize(infos), infos, NULL, ctx->vk.pipelines));

		vkDestroyShaderModule(ctx->vk.device, tile_vert.module, NULL);
		vkDestroyShaderModule(ctx->vk.device, tile_frag.module, NULL);
		vkDestroyShaderModule(ctx->vk.device, entity_vert.module, NULL);
		vkDestroyShaderModule(ctx->vk.device, entity_frag.module, NULL);
	}

	// VulkanCreateSampler
	{
		VkSamplerCreateInfo info = { 
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.magFilter = VK_FILTER_LINEAR,
			.minFilter = VK_FILTER_LINEAR,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.anisotropyEnable = VK_TRUE,
			.maxAnisotropy = ctx->vk.physical_device_properties.limits.maxSamplerAnisotropy,
			.compareOp = VK_COMPARE_OP_ALWAYS,
			.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		};
		VK_CHECK(vkCreateSampler(ctx->vk.device, &info, NULL, &ctx->vk.sampler));
	}

	// VulkanCreateDescriptorPool
	{
		VkDescriptorPoolSize size = {
			VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			1,
		};

		VkDescriptorPoolCreateInfo info = { 
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.maxSets = 1,
			.poolSizeCount = 1,
			.pPoolSizes = &size,
		};

		VK_CHECK(vkCreateDescriptorPool(ctx->vk.device, &info, NULL, &ctx->vk.descriptor_pool));
	}

	// VulkanCreateDescriptorSets
	{
		VkDescriptorSetAllocateInfo info = { 
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = ctx->vk.descriptor_pool,
			.descriptorSetCount = 1,
			.pSetLayouts = &ctx->vk.descriptor_set_layout,
		};


		VK_CHECK(vkAllocateDescriptorSets(ctx->vk.device, &info, &ctx->vk.descriptor_set));
	}
	
	// LoadSprites
	{
		ctx->vk.static_staging_buffer_stream = SDL_IOFromDynamicMem(); SDL_CHECK(ctx->vk.static_staging_buffer_stream);
		SDL_CHECK(SDL_EnumerateDirectory("assets\\legacy_fantasy_high_forest", EnumerateSpriteDirectory, ctx));
	}

	// SortSpriteCells
	{
		SPALL_BUFFER_BEGIN_NAME("SortSprites");

		for (size_t sprite_idx = 0; sprite_idx < MAX_SPRITES; sprite_idx += 1) {
			SpriteDesc* sd = GetSpriteDesc(ctx, (Sprite){(int32_t)sprite_idx});
			if (sd) {
				for (size_t frame_idx = 0; frame_idx < sd->num_frames; frame_idx += 1) {
					SpriteFrame* sf = &sd->frames[frame_idx];
					SDL_qsort(sf->cells, sf->num_cells, sizeof(SpriteCell), (SDL_CompareCallback)CompareSpriteCells);
				}
			}
		}

		SPALL_BUFFER_END();
	}

	// LoadLevels
	{
		SPALL_BUFFER_BEGIN_NAME("LoadLevels");

		cJSON* head;
		{
			size_t file_len;
			void* file_data = SDL_LoadFile("assets\\levels\\test.ldtk", &file_len); SDL_CHECK(file_data);
			head = cJSON_ParseWithLength((const char*)file_data, file_len);
			SDL_free(file_data);
		}
		SDL_assert(HAS_FLAG(head->type, cJSON_Object));

		cJSON* level_nodes = cJSON_GetObjectItem(head, "levels");
		cJSON* level_node; cJSON_ArrayForEach(level_node, level_nodes) {
			++ctx->num_levels;
		}
		ctx->levels = ArenaAlloc(&ctx->arena, ctx->num_levels, Level);
		size_t level_idx = 0;

		// LoadLevel
		cJSON_ArrayForEach(level_node, level_nodes) {
			Level level = {0};

			cJSON* w = cJSON_GetObjectItem(level_node, "pxWid");
			level.size.x = (int32_t)cJSON_GetNumberValue(w);

			cJSON* h = cJSON_GetObjectItem(level_node, "pxHei");
			level.size.y = (int32_t)cJSON_GetNumberValue(h);

			size_t num_tiles = (size_t)(level.size.x*level.size.y/TILE_SIZE);
			level.tiles = ArenaAlloc(&ctx->arena, num_tiles, bool);

			// TODO
			level.num_tile_layers = 3;
			level.tile_layers = ArenaAlloc(&ctx->arena, level.num_tile_layers, TileLayer);
		
			const char* layer_player = "Player";
			const char* layer_enemies = "Enemies";
			const char* layer_tiles = "Tiles";
			const char* layer_props = "Props";
			const char* layer_grass = "Grass";

			cJSON* layer_instances = cJSON_GetObjectItem(level_node, "layerInstances");
			cJSON* layer_instance; cJSON_ArrayForEach(layer_instance, layer_instances) {
				cJSON* node_type = cJSON_GetObjectItem(layer_instance, "__type");
				char* type = cJSON_GetStringValue(node_type); SDL_assert(type);
				cJSON* node_ident = cJSON_GetObjectItem(layer_instance, "__identifier");
				char* ident = cJSON_GetStringValue(node_ident); SDL_assert(ident);

				if (SDL_strcmp(type, "Tiles") == 0) {
					TileLayer* tile_layer = NULL;
					// TODO
	 				if (SDL_strcmp(ident, layer_tiles) == 0) {
						tile_layer = &level.tile_layers[0];
					} else if (SDL_strcmp(ident, layer_props) == 0) {
						tile_layer = &level.tile_layers[1];
					} else if (SDL_strcmp(ident, layer_grass) == 0) {
						tile_layer = &level.tile_layers[2];
					} else {
						SDL_assert(!"Invalid layer!");
					}

					cJSON* grid_tiles = cJSON_GetObjectItem(layer_instance, "gridTiles");
					cJSON* grid_tile; cJSON_ArrayForEach(grid_tile, grid_tiles) {
						++tile_layer->num_tiles;
					}
				} else if (SDL_strcmp(ident, layer_enemies) == 0) {
					cJSON* entity_instances = cJSON_GetObjectItem(layer_instance, "entityInstances");
					cJSON* entity_instance; cJSON_ArrayForEach(entity_instance, entity_instances) {
						++level.num_enemies;
					}
				}
			}

			for (size_t tile_layer_idx = 0; tile_layer_idx < level.num_tile_layers; tile_layer_idx += 1) {
				TileLayer* tile_layer = &level.tile_layers[tile_layer_idx];
				tile_layer->tiles = ArenaAlloc(&ctx->arena, tile_layer->num_tiles, Tile);
				for (size_t i = 0; i < tile_layer->num_tiles; i += 1) {
					tile_layer->tiles[i].src.x = -1;
					tile_layer->tiles[i].src.x = -1;
					tile_layer->tiles[i].dst.x = -1;
					tile_layer->tiles[i].dst.y = -1;
				}
			}
			level.enemies = ArenaAlloc(&ctx->arena, level.num_enemies, Entity);
			Entity* enemy = level.enemies;
			cJSON_ArrayForEach(layer_instance, layer_instances) {
				cJSON* node_type = cJSON_GetObjectItem(layer_instance, "__type");
				char* type = cJSON_GetStringValue(node_type); SDL_assert(type);
				cJSON* node_ident = cJSON_GetObjectItem(layer_instance, "__identifier");
				char* ident = cJSON_GetStringValue(node_ident); SDL_assert(ident);
				if (SDL_strcmp(type, "Entities") == 0) {
					cJSON* entity_instances = cJSON_GetObjectItem(layer_instance, "entityInstances");

					if (SDL_strcmp(ident, layer_player) == 0) {
						cJSON* entity_instance = entity_instances->child;
						cJSON* world_x = cJSON_GetObjectItem(entity_instance, "__worldX");
						cJSON* world_y = cJSON_GetObjectItem(entity_instance, "__worldY");
						level.player.start_pos = (ivec2s){(int32_t)cJSON_GetNumberValue(world_x), (int32_t)cJSON_GetNumberValue(world_y)};
					} else if (SDL_strcmp(ident, layer_enemies) == 0) {
						cJSON* entity_instance; cJSON_ArrayForEach(entity_instance, entity_instances) {
							cJSON* identifier_node = cJSON_GetObjectItem(entity_instance, "__identifier");
							char* identifier = cJSON_GetStringValue(identifier_node);
							cJSON* world_x = cJSON_GetObjectItem(entity_instance, "__worldX");
							cJSON* world_y = cJSON_GetObjectItem(entity_instance, "__worldY");
							enemy->start_pos = (ivec2s){(int32_t)cJSON_GetNumberValue(world_x), (int32_t)cJSON_GetNumberValue(world_y)};
							if (SDL_strcmp(identifier, "Boar") == 0) {
								enemy->type = EntityType_Boar;
							} // else if (SDL_strcmp(identifier, "") == 0) {}
							++enemy;
						}
					}
				} else if (SDL_strcmp(type, "Tiles") == 0) {
					cJSON* grid_tiles = cJSON_GetObjectItem(layer_instance, "gridTiles");

					// TODO
					TileLayer* tile_layer = NULL;
	 				if (SDL_strcmp(ident, layer_tiles) == 0) {
						tile_layer = &level.tile_layers[0];
					} else if (SDL_strcmp(ident, layer_props) == 0) {
						tile_layer = &level.tile_layers[1];
					} else if (SDL_strcmp(ident, layer_grass) == 0) {
						tile_layer = &level.tile_layers[2];
					} else {
						SDL_assert(!"Invalid layer!");
					}

					size_t i = 0;
					cJSON* grid_tile; cJSON_ArrayForEach(grid_tile, grid_tiles) {
						cJSON* src_node = cJSON_GetObjectItem(grid_tile, "src");
						ivec2s src = {
							(int32_t)cJSON_GetNumberValue(src_node->child),
							(int32_t)cJSON_GetNumberValue(src_node->child->next),
						};

						cJSON* dst_node = cJSON_GetObjectItem(grid_tile, "px");
						ivec2s dst = {
							(int32_t)cJSON_GetNumberValue(dst_node->child),
							(int32_t)cJSON_GetNumberValue(dst_node->child->next),
						};

						SDL_assert(i < tile_layer->num_tiles);
						tile_layer->tiles[i++] = (Tile){src, dst};
					}

					//SDL_qsort(tile_layer->tiles, tile_layer->num_tiles, sizeof(Tile), (SDL_CompareCallback)CompareTileSrc);
				}
 				
			}
			ctx->levels[level_idx++] = level;
		}

		cJSON* defs = cJSON_GetObjectItem(head, "defs");
		cJSON* tilesets = cJSON_GetObjectItem(defs, "tilesets");
		bool break_all = false;
		cJSON* tileset; cJSON_ArrayForEach(tileset, tilesets) {
			cJSON* enum_tags = cJSON_GetObjectItem(tileset, "enumTags");
			cJSON* enum_tag; cJSON_ArrayForEach(enum_tag, enum_tags) {
				cJSON* id_node = cJSON_GetObjectItem(enum_tag, "enumValueId");
				char* id = cJSON_GetStringValue(id_node);
				if (SDL_strcmp(id, "Collide") != 0) continue;
				
				cJSON* tile_ids = cJSON_GetObjectItem(enum_tag, "tileIds");
				cJSON* tile_id; cJSON_ArrayForEach(tile_id, tile_ids) {
					size_t src_idx = (int32_t)cJSON_GetNumberValue(tile_id);
					TileLayer* tile_layer = &ctx->levels[0].tile_layers[0]; // TODO
					ivec2s dst = tile_layer->tiles[src_idx].dst;
					if (dst.x == -1 || dst.y == -1) continue;
					Level* level = GetCurrentLevel(ctx); // TODO
					size_t tile_idx = (size_t)((dst.x + dst.y*level->size.x)/TILE_SIZE);
					level->tiles[tile_idx] = true;
				}
				break_all = true;
				break;
			}
			if (break_all) break;
		}
		break_all = false;

		cJSON_Delete(head);

		SPALL_BUFFER_END();
	}

	// VulkanCreateStaticStagingBuffer
	{
		void* stream_ptr; size_t stream_size;
		GetDynamicMemProperties(ctx->vk.static_staging_buffer_stream, &stream_ptr, &stream_size);

		ctx->vk.static_staging_buffer.size = stream_size;
		for (size_t level_idx = 0; level_idx < ctx->num_levels; level_idx += 1) {
			Level* level = &ctx->levels[level_idx];
			for (size_t tile_layer_idx = 0; tile_layer_idx < level->num_tile_layers; tile_layer_idx += 1) {
				TileLayer* tile_layer = &level->tile_layers[tile_layer_idx];
				ctx->vk.static_staging_buffer.size += tile_layer->num_tiles * sizeof(Tile);
			}
		}
		{
			uint32_t queue_family_idx = 0;
			VkBufferCreateInfo buffer_info = {
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.size = ctx->vk.static_staging_buffer.size,
				.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,

				// TODO
				.queueFamilyIndexCount = 1,
				.pQueueFamilyIndices = &queue_family_idx,
			};
			VK_CHECK(vkCreateBuffer(ctx->vk.device, &buffer_info, NULL, &ctx->vk.static_staging_buffer.handle));
		}
		{
			VkMemoryRequirements mem_req;
			vkGetBufferMemoryRequirements(ctx->vk.device, ctx->vk.static_staging_buffer.handle, &mem_req);
			uint32_t memory_type_idx = VulkanGetMemoryTypeIdxWithProperties(&ctx->vk, &mem_req, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			VkMemoryAllocateInfo mem_info = {
				.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
				.allocationSize = mem_req.size,
				.memoryTypeIndex = memory_type_idx,
			};
			VK_CHECK(vkAllocateMemory(ctx->vk.device, &mem_info, NULL, &ctx->vk.static_staging_buffer.memory));
		}

		void* data;
		VK_CHECK(vkMapMemory(ctx->vk.device, ctx->vk.static_staging_buffer.memory, 0, ctx->vk.static_staging_buffer.size, 0, &data));
		uint8_t* cur = data; // for convenience
		
		SDL_memcpy(cur, stream_ptr, stream_size);
		ctx->vk.static_staging_buffer.offset += stream_size;

		for (size_t level_idx = 0; level_idx < ctx->num_levels; level_idx += 1) {
			Level* level = &ctx->levels[level_idx];
			for (size_t tile_layer_idx = 0; tile_layer_idx < level->num_tile_layers; tile_layer_idx += 1) {
				TileLayer* tile_layer = &level->tile_layers[tile_layer_idx];
				SDL_memcpy(cur + ctx->vk.static_staging_buffer.offset, tile_layer->tiles, sizeof(Tile)*tile_layer->num_tiles);
				ctx->vk.static_staging_buffer.offset += sizeof(Tile)*tile_layer->num_tiles;
			}
		}
		vkUnmapMemory(ctx->vk.device, ctx->vk.static_staging_buffer.memory);

		{
			VkDeviceSize memory_offset = 0;
			VK_CHECK(vkBindBufferMemory(ctx->vk.device, ctx->vk.static_staging_buffer.handle, ctx->vk.static_staging_buffer.memory, memory_offset));
		}

		SDL_CHECK(SDL_CloseIO(ctx->vk.static_staging_buffer_stream));
		ctx->vk.static_staging_buffer_stream = NULL;
	}

	{
		// The reason I commented it out is because it causes a segmentation fault.
		//SDL_qsort((void*)mem_req, ctx->num_sprite_cells, sizeof(VkImageMemoryRequirements), (SDL_CompareCallback)VulkanCompareImageMemoryRequirements);

		VkImage* images = VulkanGetSpriteCellsImages(ctx);

		// VulkanAllocateAndBindImageMemory
		{
			VkMemoryRequirements* mem_reqs = VulkanGetSpriteCellsMemoryRequirements(ctx);
			VkMemoryAllocateInfo allocate_info = {
				.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
				.memoryTypeIndex = VulkanGetMemoryTypeIdx(&ctx->vk, &mem_reqs[0]),
			};
			VkBindImageMemoryInfo* bind_infos = SDL_calloc(ctx->num_sprite_cells, sizeof(VkBindImageMemoryInfo)); SDL_CHECK(bind_infos);

			for (size_t i = 0; i < ctx->num_sprite_cells; i += 1) {
				allocate_info.allocationSize = AlignForward(allocate_info.allocationSize, mem_reqs[i].alignment);
				bind_infos[i] = (VkBindImageMemoryInfo){
					.sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO,
					.image = images[i],
					.memoryOffset = allocate_info.allocationSize,
				};
				allocate_info.allocationSize += mem_reqs[i].size;
			}
			VK_CHECK(vkAllocateMemory(ctx->vk.device, &allocate_info, NULL, &ctx->vk.image_memory));

			for (size_t i = 0; i < ctx->num_sprite_cells; i += 1) {
				bind_infos[i].memory = ctx->vk.image_memory;
			}
			VK_CHECK(vkBindImageMemory2(ctx->vk.device, (uint32_t)ctx->num_sprite_cells, bind_infos));

			SDL_free(bind_infos);
			SDL_free(mem_reqs);
		}

		// VulkanCreateImageMemoryBarriers
		ctx->vk.image_memory_barriers_before = SDL_malloc(ctx->num_sprite_cells*sizeof(VkImageMemoryBarrier)); SDL_CHECK(ctx->vk.image_memory_barriers_before);
		ctx->vk.image_memory_barriers_after = SDL_malloc(ctx->num_sprite_cells*sizeof(VkImageMemoryBarrier)); SDL_CHECK(ctx->vk.image_memory_barriers_after);
		ctx->vk.image_memory_barriers = ArenaAlloc(&ctx->arena, ctx->num_sprite_cells, VkImageMemoryBarrier);
		for (size_t i = 0; i < ctx->num_sprite_cells; i += 1) {
			VkImageSubresourceRange subresource_range = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			};

			ctx->vk.image_memory_barriers_before[i] = (VkImageMemoryBarrier){
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.srcAccessMask = 0,
				.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.image = images[i],
				.subresourceRange = subresource_range,
			};

			ctx->vk.image_memory_barriers_after[i] = (VkImageMemoryBarrier){
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				.image = images[i],
				.subresourceRange = subresource_range,
			};

			ctx->vk.image_memory_barriers[i] = (VkImageMemoryBarrier){
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.srcAccessMask = 0,
				.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				.image = images[i],
				.subresourceRange = subresource_range,
			};
		}

		SDL_free(images);
	}

	// VulkanCreateDynamicStagingBuffer
	{
		ctx->vk.dynamic_staging_buffer.size = sizeof(EntityVertex); // the player
		for (size_t level_idx = 0; level_idx < ctx->num_levels; level_idx += 1) {
			Level* level = &ctx->levels[level_idx];
			ctx->vk.dynamic_staging_buffer.size += sizeof(EntityVertex)*level->num_enemies;
		}
		{
			uint32_t queue_family_idx = 0;
			VkBufferCreateInfo buffer_info = {
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.size = ctx->vk.dynamic_staging_buffer.size,
				.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,

				// TODO
				.queueFamilyIndexCount = 1,
				.pQueueFamilyIndices = &queue_family_idx,
			};
			VK_CHECK(vkCreateBuffer(ctx->vk.device, &buffer_info, NULL, &ctx->vk.dynamic_staging_buffer.handle));
		}
		{
			VkMemoryRequirements mem_req;
			vkGetBufferMemoryRequirements(ctx->vk.device, ctx->vk.dynamic_staging_buffer.handle, &mem_req);

			uint32_t memory_type_idx = VulkanGetMemoryTypeIdxWithProperties(&ctx->vk, &mem_req, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			VkMemoryAllocateInfo mem_info = {
				.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
				.allocationSize = mem_req.size,
				.memoryTypeIndex = memory_type_idx,
			};
			VK_CHECK(vkAllocateMemory(ctx->vk.device, &mem_info, NULL, &ctx->vk.dynamic_staging_buffer.memory));

			VK_CHECK(vkMapMemory(ctx->vk.device, ctx->vk.dynamic_staging_buffer.memory, 0, ctx->vk.dynamic_staging_buffer.size, 0, &ctx->vk.dynamic_staging_buffer.mapped));
		}
		{
			VkDeviceSize memory_offset = 0;
			VK_CHECK(vkBindBufferMemory(ctx->vk.device, ctx->vk.dynamic_staging_buffer.handle, ctx->vk.dynamic_staging_buffer.memory, memory_offset));
		}
	}

	// VulkanCreateVertexBuffer
	{
		ctx->vk.vertex_buffer.size = ctx->vk.dynamic_staging_buffer.size;
		for (size_t level_idx = 0; level_idx < ctx->num_levels; level_idx += 1) {
			Level* level = &ctx->levels[level_idx];
			for (size_t tile_layer_idx = 0; tile_layer_idx < level->num_tile_layers; tile_layer_idx += 1) {
				TileLayer* tile_layer = &level->tile_layers[tile_layer_idx];
				ctx->vk.vertex_buffer.size += tile_layer->num_tiles*sizeof(Tile);
			}
		}

		VkBufferCreateInfo buffer_info = { 
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = ctx->vk.vertex_buffer.size,
			.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		};
		
		VK_CHECK(vkCreateBuffer(ctx->vk.device, &buffer_info, NULL, &ctx->vk.vertex_buffer.handle));

		VkMemoryRequirements mem_req;
		vkGetBufferMemoryRequirements(ctx->vk.device, ctx->vk.vertex_buffer.handle, &mem_req);

		VkMemoryAllocateInfo mem_info = { 
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, 
			.allocationSize = mem_req.size,
			.memoryTypeIndex = VulkanGetMemoryTypeIdx(&ctx->vk, &mem_req),
		};
		
		VK_CHECK(vkAllocateMemory(ctx->vk.device, &mem_info, NULL, &ctx->vk.vertex_buffer.memory));
		VK_CHECK(vkBindBufferMemory(ctx->vk.device, ctx->vk.vertex_buffer.handle, ctx->vk.vertex_buffer.memory, 0));
	}

	// InitReplayFrames
	{
		ctx->c_replay_frames = 1024;
		ctx->replay_frames = SDL_malloc(ctx->c_replay_frames * sizeof(ReplayFrame)); SDL_CHECK(ctx->replay_frames);
	}

	ResetGame(ctx);

	ctx->running = true;
	while (ctx->running) {	
		// GetInput
		{
			SPALL_BUFFER_BEGIN_NAME("GetInput");

			if (!ctx->gamepad) {
				int32_t joystick_count = 0;
				SDL_JoystickID* joysticks = SDL_GetGamepads(&joystick_count);
				if (joystick_count != 0) {
					ctx->gamepad = SDL_OpenGamepad(joysticks[0]);
				}
			}

			ctx->button_jump = false;
			ctx->button_jump_released = false;
			ctx->button_attack = false;
			ctx->left_mouse_pressed = false;

			SDL_Event event;
			while (SDL_PollEvent(&event)) {
				switch (event.type) {
				case SDL_EVENT_KEY_DOWN:
					if (event.key.key == SDLK_ESCAPE) {
						ctx->running = false;
					}
					if (!ctx->gamepad) {
						switch (event.key.key) {
						case SDLK_SPACE:
							ctx->paused = !ctx->paused;
							break;
						case SDLK_0:
							break;
						case SDLK_X:
							ctx->button_attack = true;
							break;
						case SDLK_LEFT:
							if (!event.key.repeat) {
								ctx->button_left = 1;
							}
							if (ctx->paused) {
								//SetReplayFrame(ctx, SDL_max(ctx->replay_frame_idx - 1, 0));
							}
							break;
						case SDLK_RIGHT:
							if (!event.key.repeat) {
								ctx->button_right = 1;
							}
							if (ctx->paused) {
								//SetReplayFrame(ctx, SDL_min(ctx->replay_frame_idx + 1, ctx->replay_frame_idx_max - 1));
							}
							break;
						case SDLK_UP:
							if (!event.key.repeat) {
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
					if (event.button.button == SDL_BUTTON_LEFT) {
						ctx->left_mouse_pressed = true;
					}
					break;
				case SDL_EVENT_KEY_UP:
					if (!ctx->gamepad) {
						switch (event.key.key) {
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
					switch (event.gaxis.axis) {
					case SDL_GAMEPAD_AXIS_LEFTX:
						ctx->gamepad_left_stick.x = NormInt16(event.gaxis.value);
						break;
					case SDL_GAMEPAD_AXIS_LEFTY:
						ctx->gamepad_left_stick.y = NormInt16(event.gaxis.value);
						break;
					}
					break;
				case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
					switch (event.gbutton.button) {
					case SDL_GAMEPAD_BUTTON_SOUTH:
						ctx->button_jump = true;
						break;
					case SDL_GAMEPAD_BUTTON_WEST:
						ctx->button_attack = true;
						break;
					}
					break;
				case SDL_EVENT_GAMEPAD_BUTTON_UP:
					switch (event.gbutton.button) {
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

			if (SDL_fabs(ctx->gamepad_left_stick.x) > GAMEPAD_THRESHOLD) {
				ctx->gamepad_left_stick.x = 0.0f;
			}

			SPALL_BUFFER_END();
		}
		if (!ctx->paused) {
			// UpdateGame
			{
				SPALL_BUFFER_BEGIN_NAME("UpdateGame");

				UpdatePlayer(ctx);
				size_t num_enemies; Entity* enemies = GetEnemies(ctx, &num_enemies);
				for (size_t enemy_idx = 0; enemy_idx < num_enemies; enemy_idx += 1) {
					Entity* enemy = &enemies[enemy_idx];
					if (enemy->type == EntityType_Boar) {
						UpdateBoar(ctx, enemy);
					}
				}

				SPALL_BUFFER_END();
			}

		#if 0
			// RecordReplayFrame
			{
				SPALL_BUFFER_BEGIN_NAME("RecordReplayFrame");

				ReplayFrame replay_frame = {0};
				
				Level* level = GetCurrentLevel(ctx);
				replay_frame.player = level->player;
				replay_frame.enemies = SDL_malloc(level->num_enemies * sizeof(Entity)); SDL_CHECK(replay_frame.enemies);
				SDL_memcpy(replay_frame.enemies, level->enemies, level->num_enemies * sizeof(Entity));
				replay_frame.num_enemies = level->num_enemies;
					
				ctx->replay_frames[ctx->replay_frame_idx++] = replay_frame;
				if (ctx->replay_frame_idx >= ctx->c_replay_frames - 1) {
					ctx->c_replay_frames *= 8;
					ctx->replay_frames = SDL_realloc(ctx->replay_frames, ctx->c_replay_frames * sizeof(ReplayFrame)); SDL_CHECK(ctx->replay_frames);
				}
				ctx->replay_frame_idx_max = SDL_max(ctx->replay_frame_idx_max, ctx->replay_frame_idx);

				SPALL_BUFFER_END();
			}
		#endif
		}
		
		// CopyVerticesToDynamicStagingBuffer
		VkDeviceSize vertex_buffer_offset;
		{
			size_t num_entity_vertices;
			EntityVertex* entity_vertices = GetEntityVertices(GetCurrentLevel(ctx), &num_entity_vertices);
			SDL_memcpy(ctx->vk.dynamic_staging_buffer.mapped, entity_vertices, num_entity_vertices * sizeof(EntityVertex));
			vertex_buffer_offset = num_entity_vertices * sizeof(EntityVertex); // TODO: Is there a clearer way to do this?
			SDL_free(entity_vertices);
		}

		uint32_t image_idx;
		VkCommandBuffer cb;

		// DrawBegin
		{
			SPALL_BUFFER_BEGIN_NAME("DrawBegin");

			VK_CHECK(vkWaitForFences(ctx->vk.device, 1, &ctx->vk.frames[ctx->vk.current_frame].fence_in_flight, VK_TRUE, UINT64_MAX));
			VK_CHECK(vkResetFences(ctx->vk.device, 1, &ctx->vk.frames[ctx->vk.current_frame].fence_in_flight));

			VK_CHECK(vkAcquireNextImageKHR(ctx->vk.device, ctx->vk.swapchain, UINT64_MAX, ctx->vk.frames[ctx->vk.current_frame].sem_image_available, VK_NULL_HANDLE, &image_idx));

			cb = ctx->vk.frames[ctx->vk.current_frame].command_buffer;

			// VulkanBeginCommandBuffer
			{
				VkCommandBufferBeginInfo info = {
					.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
					.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
				};
				VK_CHECK(vkBeginCommandBuffer(cb, &info));
			}

			// VulkanUploadStaticStagingBuffer
			if (!ctx->vk.staged) {
				ctx->vk.staged = true;

				VkBufferMemoryBarrier buffer_memory_barriers[] = {
					{
						.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
						.srcAccessMask = 0,
						.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
						.buffer = ctx->vk.static_staging_buffer.handle,
						.size = ctx->vk.static_staging_buffer.size,
					},
				};
				vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, SDL_arraysize(buffer_memory_barriers), buffer_memory_barriers, (uint32_t)ctx->num_sprite_cells, ctx->vk.image_memory_barriers_before);

				VkBufferImageCopy region = {
					.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
				};
				EnumerateSpriteCells(ctx, VulkanCopyBufferToImageCallback, &region);

				// TODO
				VkBufferCopy region2 = {
					// TODO: Is there a way a more self-documenting way to write this?
					.srcOffset = region.bufferOffset,
					.dstOffset = vertex_buffer_offset,
					.size = ctx->vk.vertex_buffer.size - ctx->vk.dynamic_staging_buffer.size,
				};
				vkCmdCopyBuffer(cb, ctx->vk.static_staging_buffer.handle, ctx->vk.vertex_buffer.handle, 1, &region2);

				vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, (uint32_t)ctx->num_sprite_cells, ctx->vk.image_memory_barriers_after);


			} else {
				vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, (uint32_t)ctx->num_sprite_cells, ctx->vk.image_memory_barriers);
			}

			// VulkanUploadDynamicStagingBuffer
			{
				VkBufferMemoryBarrier buffer_memory_barriers_before[] = {
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
				};
				vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, SDL_arraysize(buffer_memory_barriers_before), buffer_memory_barriers_before, 0, NULL);

				VkBufferCopy region = {
					.size = ctx->vk.dynamic_staging_buffer.size,
				};
				vkCmdCopyBuffer(cb, ctx->vk.dynamic_staging_buffer.handle, ctx->vk.vertex_buffer.handle, 1, &region);

				VkBufferMemoryBarrier buffer_memory_barriers_after[] = {
					{
						.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
						.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
						.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
						.buffer = ctx->vk.vertex_buffer.handle,
						.size = ctx->vk.vertex_buffer.size,
					},
				};
				vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, NULL, SDL_arraysize(buffer_memory_barriers_after), buffer_memory_barriers_after, 0, NULL);
			}

			// VulkanBeginRenderPass
			{
				VkClearValue clear_values[2];
				clear_values[0].color = (VkClearColorValue){0.0f, 0.0f, 0.0f, 1.0f };
				clear_values[1].depthStencil = (VkClearDepthStencilValue){1.0f, 0};

				VkRenderPassBeginInfo info = { 
					.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
					.renderPass = ctx->vk.render_pass,
					.framebuffer = ctx->vk.framebuffers[image_idx],
					.renderArea = { .extent = ctx->vk.swapchain_info.imageExtent },
					.clearValueCount = SDL_arraysize(clear_values),
					.pClearValues = clear_values,
				};
				vkCmdBeginRenderPass(cb, &info, VK_SUBPASS_CONTENTS_INLINE);
			}

			SPALL_BUFFER_END();
		}

		// Draw
		{
			{
				uint32_t first_viewport = 0;
				uint32_t num_viewports = 1;
				vkCmdSetViewport(cb, first_viewport, num_viewports, &ctx->vk.viewport);
			}
			{
				uint32_t first_scissor = 0;
				uint32_t num_scissors = 1;
				vkCmdSetScissor(cb, first_scissor, num_scissors, &ctx->vk.scissor);
			}
			{
				uint32_t first_binding = 0;
				uint32_t binding_count = 1;
				VkDeviceSize offset = 0;
				vkCmdBindVertexBuffers(cb, first_binding, binding_count, &ctx->vk.vertex_buffer.handle, &offset);
			}
			{
				uint32_t first_set = 0;
				uint32_t num_descriptor_sets = 1;
				uint32_t num_dynamic_offsets = 0;
				const uint32_t* dynamic_offsets = NULL;
				vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->vk.pipeline_layout, first_set, num_descriptor_sets, &ctx->vk.descriptor_set, num_dynamic_offsets, dynamic_offsets);
			}


			vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->vk.pipelines[0]);
			// TODO: Draw tiles.

			vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->vk.pipelines[1]);
			// TODO: Draw entities.


		}

		// DrawEnd
		{
			vkCmdEndRenderPass(cb);

			VK_CHECK(vkEndCommandBuffer(cb));

			VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

			VkSubmitInfo submit_info = { 
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

			VkPresentInfoKHR present_info = { 
				.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
				.waitSemaphoreCount = 1,
				.pWaitSemaphores = &ctx->vk.frames[ctx->vk.current_frame].sem_render_finished,
				.swapchainCount = 1,
				.pSwapchains = &ctx->vk.swapchain,
				.pImageIndices = &image_idx,
			};
			VK_CHECK(vkQueuePresentKHR(ctx->vk.graphics_queue, &present_info));

			ctx->vk.current_frame = (ctx->vk.current_frame + 1) % ctx->vk.num_frames;
		}

		static bool blah = false;
		if (!blah) {
			blah = true;
		} else {
			raddbg_break();
		}

	#if 0
		// DrawEntities
		{
			SPALL_BUFFER_BEGIN_NAME("DrawEntities");

			size_t num_enemies; Entity* enemies = GetEnemies(ctx, &num_enemies);
			for (size_t enemy_idx = 0; enemy_idx < num_enemies; enemy_idx += 1) {
				Entity* enemy = &enemies[enemy_idx];
				DrawEntity(ctx, enemy);
			}
			DrawEntity(ctx, GetPlayer(ctx));

			SPALL_BUFFER_END();
		}

		// DrawLevel
		{
			SPALL_BUFFER_BEGIN_NAME("DrawLevel");

			SpriteDesc* sd = GetSpriteDesc(ctx, spr_tiles);
			SDL_assert(sd->num_layers == 1);
			SDL_assert(sd->num_frames == 1);
			SDL_assert(sd->frames[0].num_cells == 1);

			Level* level = GetCurrentLevel(ctx);

			SDL_Texture* texture = sd->frames[0].cells[0].texture;

			size_t draw_calls = 0;

			SDL_FRect* tiles_to_draw = ArenaAlloc(&ctx->temp, level->size.x*level->size.y/TILE_SIZE, SDL_FRect);
			size_t num_tiles_to_draw = 0;

			for (size_t tile_layer_idx = 0; tile_layer_idx < level->num_tile_layers; tile_layer_idx += 1) {
				size_t num_tiles;
				Tile* tiles = GetLayerTiles(level, tile_layer_idx, &num_tiles);
				
				for (size_t i = 0; i < num_tiles; i += 1) {
					if (!TileIsValid(tiles[i])) continue;

					tiles_to_draw[num_tiles_to_draw++] = (SDL_FRect){
						(float)tiles[i].dst.x,
						(float)tile[i].dst.y,
						(float)TILE_SIZE,
						(float)TILE_SIZE,
					};

					size_t j;
					for (j = i + 1; j < num_tiles; j += 1) {
						SDL_assert(TileIsValid(tiles[j]));				
						if (tiles[j].src.x != tiles[i].src.x || tiles[j].src.y != tiles[i].src.y) break;
						tiles_to_draw[num_tiles_to_draw++] = (SDL_FRect){
							(float)tiles[j].dst.x,
							(float)tile[j].dst.y,
							(float)TILE_SIZE,
							(float)TILE_SIZE,
						};
					}

					SDL_FRect srcrect = {
						(float)tiles[i].src.x,
						(float)tiles[i].src.y,
						(float)TILE_SIZE,
						(float)TILE_SIZE,
					};
					
					SDL_CHECK(SDL_RenderTexture(ctx->renderer, texture, &srcrect, &dstrect));

					i = j;
					num_tiles_to_draw = 0;

					++draw_calls;
				}
			}

			//SDL_Log("Draw calls: %llu", draw_calls);

			SPALL_BUFFER_END();
		}

		// DrawHitbox
		{
			SPALL_BUFFER_BEGIN_NAME("DrawHitbox");

			Entity* player = GetPlayer(ctx);
			Rect hitbox = GetEntityHitbox(ctx, player);
			SDL_CHECK(SDL_SetRenderDrawColor(ctx->renderer, 255, 0, 0, 128));
			SDL_FRect rect = {(float)hitbox.min.x, (float)hitbox.min.y, (float)(hitbox.max.x-hitbox.min.x+1), (float)(hitbox.max.y-hitbox.min.y+1)};
			SDL_CHECK(SDL_RenderFillRect(ctx->renderer, &rect));

			SPALL_BUFFER_END();
		}

		// DrawCollisions
		{
			SPALL_BUFFER_BEGIN_NAME("DrawCollisions");

			Level* level = &ctx->levels[0]; // TODO
			ivec2s dst = {0, 0};
			for (size_t i = 0; i < level->size.x*level->size.y/TILE_SIZE; i += 1) {
				if (level->tiles[i]) {
					SDL_FRect rect = {
						(float)dst.x,
						(float)dst.y, 
						(float)TILE_SIZE, 
						(float)TILE_SIZE,
					};
					SDL_CHECK(SDL_RenderFillRect(ctx->renderer, &rect));
				}
				dst.x += TILE_SIZE;
				if (dst.x >= level->size.x) {
					dst.x = 0;
					dst.y += TILE_SIZE;
				}
			}

			SPALL_BUFFER_END();
		}

		// DrawEnd
		{
			SPALL_BUFFER_BEGIN_NAME("DrawEnd");

			SDL_RenderPresent(ctx->renderer);

			SPALL_BUFFER_END();
		}
	#endif
	}

	// NOTE: If we don't do this, we might not get the last few events.
#if ENABLE_PROFILING
	spall_buffer_quit(&ctx->spall_ctx, &ctx->spall_buffer);
	spall_quit(&ctx->spall_ctx);
#endif

	// NOTE: If we don't do this, SDL might not reverse certain operations,
	// like changing the resolution of the monitor.
	SDL_Quit();

	return 0;
}