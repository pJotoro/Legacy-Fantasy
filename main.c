// libraries.h
	#include <SDL.h>
	#include <SDL_main.h>

	typedef int64_t ssize_t;

	#include <cglm/struct.h>

	#define XXH_STATIC_LINKING_ONLY /* access advanced declarations */
	#define XXH_IMPLEMENTATION      /* access definitions */
	#include <xxhash.h>

	#include "infl.h"

	#define DBL_EPSILON 2.2204460492503131e-016
	#include "json.h"

	#include <raddbg_markup.h>

// main.h
	// Aseprite
		#pragma pack(push, 1)

		typedef struct ASE_Fixed {
			uint16_t val[2];
		} ASE_Fixed;

		typedef struct ASE_String {
			uint16_t len;
			// uint8_t buf[];
		} ASE_String;

		typedef ivec2s ASE_Point;
		typedef ivec2s ASE_Size;

		typedef struct ASE_Rect {
			ASE_Point origin;
			ASE_Size size;
		} ASE_Rect;

		typedef union ASE_Pixel {
			uint8_t rgba[4];
			uint8_t grayscale[2];
			uint8_t index;
		} ASE_Pixel;

		enum {
			ASE_Flags_LayerOpacityValid = 1u,
			ASE_Flags_LayerOpacityValidForGroups = 2u,
			ASE_Flags_LayersHaveUUID = 4u,
		};
		typedef uint32_t ASE_Flags;

		typedef struct ASE_Header {
			uint32_t file_size;
			uint16_t magic_number;
			uint16_t n_frames;
			uint16_t w;
			uint16_t h;
			uint16_t color_depth;
			ASE_Flags flags;
			uint16_t reserved0;
			uint32_t reserved1;
			uint32_t reserved2;
			uint8_t transparent_color_entry;
			uint8_t reserved3[3];
			uint16_t n_colors;
			uint8_t pixel_w;
			uint8_t pixel_h;
			int16_t grid_x;
			int16_t grid_y;
			uint16_t grid_w;
			uint16_t grid_h;
			uint8_t reserved4[84];

		} ASE_Header;
		static_assert(sizeof(ASE_Header) == 128);

		typedef struct ASE_Frame {
			uint32_t n_bytes;
			uint16_t magic_number;
			uint16_t reserved0;
			uint16_t frame_dur;
			uint8_t reserved1[2];
			uint32_t n_chunks;
		} ASE_Frame;
		static_assert(sizeof(ASE_Frame) == 16);

		enum {
			ASE_ChunkType_OldPalette = 0x0004u,
			ASE_ChunkType_OldPalette2 = 0x0011u,
			ASE_ChunkType_Layer = 0x2004u,
			ASE_ChunkType_Cell = 0x2005u,
			ASE_ChunkType_CellExtra = 0x2006u,
			ASE_ChunkType_ColorProfile = 0x2007u,
			ASE_ChunkType_ExternalFiles = 0x2008u,
			ASE_ChunkType_DeprecatedMask = 0x2016u,
			ASE_ChunkType_Path = 0x2017u,
			ASE_ChunkType_Tags = 0x2018u,
			ASE_ChunkType_Palette = 0x2019u,
			ASE_ChunkType_UserData = 0x2020u,
			ASE_ChunkType_Slice = 0x2022u,
			ASE_ChunkType_Tileset = 0x2023u,
		};
		typedef uint16_t ASE_ChunkType;

		typedef struct ASE_ChunkHeader {
			uint32_t size;
			ASE_ChunkType type;
		} ASE_ChunkHeader;

		enum {
			ASE_LayerChunkFlags_Visible = 1u,
			ASE_LayerChunkFlags_Editable = 2u,
			ASE_LayerChunkFlags_LockMovement = 4u,
			ASE_LayerChunkFlags_Background = 8u,
			ASE_LayerChunkFlags_PreferLinkedCells = 16u,
			ASE_LayerChunkFlags_DisplayCollapsed = 32u,
			ASE_LayerChunkFlags_ReferenceLayer = 64u,
		};
		typedef uint16_t ASE_LayerChunkFlags;

		enum {
			ASE_LayerType_Normal = 0u,
			ASE_LayerType_Group = 1u,
			ASE_LayerType_Tilemap = 2u,
		};
		typedef uint16_t ASE_LayerType;

		enum {
			ASE_BlendMode_Normal = 0u,
			ASE_BlendMode_Multiply = 1u,
			ASE_BlendMode_Screen = 2u,
			ASE_BlendMode_Overlay = 3u,
			ASE_BlendMode_Darken = 4u,
			ASE_BlendMode_Lighten = 5u,
			ASE_BlendMode_ColorDodge = 6u,
			ASE_BlendMode_ColorBurn = 7u,
			ASE_BlendMode_HardLight = 8u,
			ASE_BlendMode_SoftLight = 9u,
			ASE_BlendMode_Difference = 10u,
			ASE_BlendMode_Exclusion = 11u,
			ASE_BlendMode_Hue = 12u,
			ASE_BlendMode_Saturation = 13u,
			ASE_BlendMode_Color = 14u,
			ASE_BlendMode_Luminosity = 15u,
			ASE_BlendMode_Addition = 16u,
			ASE_BlendMode_Subtract = 17u,
			ASE_BlendMode_Divide = 18u,
		};
		typedef uint16_t ASE_BlendMode;

		typedef struct ASE_LayerChunk {
			ASE_LayerChunkFlags flags;
			ASE_LayerType layer_type;
			uint16_t layer_child_level;
			uint16_t reserved0;
			uint16_t reserved1;
			ASE_BlendMode blend_mode;
			uint8_t opacity;
			uint8_t reserved2[3];
			ASE_String layer_name;
			#if 0
			uint32_t tileset_idx;
			uint8_t uuid[16];
			#endif
		} ASE_LayerChunk;

		enum {
			ASE_CellType_Raw = 0u,
			ASE_CellType_Linked = 1u,
			ASE_CellType_CompressedImage = 2u,
			ASE_CellType_CompressedTilemap = 3u,
		};
		typedef uint16_t ASE_CellType;

		typedef struct ASE_CellChunk {
			uint16_t layer_idx;
			int16_t x;
			int16_t y;
			uint8_t opacity;
			ASE_CellType type;
			int16_t z_idx;
			uint8_t reserved0[5];
			struct {
				uint16_t w;
				uint16_t h;
				// ASE_Pixel pixels[];
			} compressed_image;
		} ASE_CellChunk;

		enum {
			ASE_CellExtraChunkFlags_PreciseBounds = 1u,
		};
		typedef uint32_t ASE_CellExtraChunkFlags;

		typedef struct ASE_CellExtraChunk {
			ASE_CellExtraChunkFlags flags;
			ASE_Fixed x;
			ASE_Fixed y;
			ASE_Fixed w;
			ASE_Fixed h;
			uint8_t reserved0[16];
		} ASE_CellExtraChunk;

		enum {
			ASE_ColorProfileType_None = 0u,
			ASE_ColorProfileType_SRGB = 1u,
			ASE_ColorProfileType_EmbeddedICC = 2u,
		};
		typedef uint16_t ASE_ColorProfileType;

		enum {
			ASE_ColorProfileFlags_SpecialFixedGamma = 1u,
		};
		typedef uint16_t ASE_ColorProfileFlags;

		typedef struct ASE_ColorProfileChunk {
			ASE_ColorProfileType type;
			ASE_ColorProfileFlags flags;
			ASE_Fixed fixed_gamma;
			uint8_t reserved0[8];
			uint32_t icc_profile_data_len;
			// uint8_t icc_profile_data[];
		} ASE_ColorProfileChunk;

		enum {
			ASE_ExternalFilesEntryType_ExternalPalette = 0u,
			ASE_ExternalFilesEntryType_ExternalTileset = 1u,
			ASE_ExternalFilesEntryType_ExtensionNameForProperties = 2u,
			ASE_ExternalFilesEntryType_ExtensionNameForTileManagement = 3u,
		};
		typedef uint8_t ASE_ExternalFilesEntryType;

		typedef struct ASE_ExternalFilesEntry {
			uint32_t id;
			ASE_ExternalFilesEntryType type;
			uint8_t reserved0[7];
			union {
				ASE_String external_file_name;
				ASE_String extension_id;
			};
		} ASE_ExternalFilesEntry;

		typedef struct ASE_ExternalFilesChunk {
			uint32_t n_entries;
			uint8_t reserved0[8];
			// ASE_ExternalFilesEntry entries[];
		} ASE_ExternalFilesChunk;

		enum {
			ASE_LoopAnimDir_Forward = 0u,
			ASE_LoopAnimDir_Reverse = 1u,
			ASE_LoopAnimDir_PingPong = 2u,
			ASE_LoopAnimDir_PingPongReverse = 3u,
		};
		typedef uint8_t ASE_LoopAnimDir;

		typedef struct ASE_Tag {
			uint16_t from_frame;
			uint16_t to_frame;
			ASE_LoopAnimDir loop_anim_dir;
			uint16_t repeat;
			uint8_t reserved0[6];
			uint8_t reserved1[3];
			uint8_t reserved2;
			ASE_String name;
		} ASE_Tag;

		typedef struct ASE_TagsChunk {
			uint16_t n_tags;
			// ASE_Tag tags[];
		} ASE_TagsChunk;

		enum {
			ASE_PaletteEntryFlags_HasName = 1u,
		};
		typedef uint16_t ASE_PaletteEntryFlags;

		typedef struct ASE_PaletteEntry {
			ASE_PaletteEntryFlags flags;
			uint8_t r;
			uint8_t g;
			uint8_t b;
			uint8_t a;
			// ASE_String color_name;
		} ASE_PaletteEntry;

		typedef struct ASE_PaletteChunk {
			uint32_t n_entries;
			uint32_t first_color_idx_to_change;
			uint32_t last_color_idx_to_change;
			uint8_t reserved0[8];
			// ASE_PaletteEntry entries[];
		} ASE_PaletteChunk;

		#pragma pack(pop)

	
	// Macros
		#define FORCEINLINE SDL_FORCE_INLINE

		#define KILOBYTE(X) ((X)*1024LL)
		#define MEGABYTE(X) (KILOBYTE(X)*1024LL)
		#define GIGABYTE(X) (MEGABYTE(X)*1024LL)
		#define TERABYTE(X) (GIGABYTE(X)*1024LL)

		#define STMT(X) do {X} while (false)

		#define STRINGIFY(X) #X
		#define CONCAT(A,B) A##B

		#define UNUSED(X) (void)X

		#define HAS_FLAG(FLAGS, FLAG) ((FLAGS) & (FLAG)) // TODO: Figure out why I can't have multiple flags set in the second argument.
		#define FLAG(X) (1u << X##u)

		#define SDL_CHECK(E) STMT( \
			if (!(E)) { \
				SDL_LogMessage(SDL_LOG_CATEGORY_ASSERT, SDL_LOG_PRIORITY_CRITICAL, "%s(%d): %s", __FILE__, __LINE__, SDL_GetError()); \
				SDL_TriggerBreakpoint(); \
			} \
		)

		#define SDL_ReadStruct(S, STRUCT) SDL_ReadIO(S, (STRUCT), sizeof(*(STRUCT)))

		#define SDL_ReadIOChecked(S, P, SZ) SDL_CHECK(SDL_ReadIO(S, P, SZ) == SZ)
		#define SDL_ReadStructChecked(S, STRUCT) SDL_CHECK(SDL_ReadStruct(S, STRUCT) == sizeof(*(STRUCT)))

		#define GetSprite(path) ((Sprite){HashString(path, 0) & (MAX_SPRITES - 1)})

	
	// Constants
		#define FULLSCREEN 1

		#define GAME_WIDTH 960
		#define GAME_HEIGHT 540

		#define GAMEPAD_THRESHOLD 0.5f

		#define PLAYER_ACC 0.015f
		#define PLAYER_FRIC 0.005f
		#define PLAYER_MAX_VEL 0.25f
		#define PLAYER_JUMP 1.0f
		#define PLAYER_JUMP_REMAINDER 0.2f

		#define BOAR_ACC 0.01f
		#define BOAR_FRIC 0.005f
		#define BOAR_MAX_VEL 0.15f

		#define TILE_SIZE 16
		#define GRAVITY 0.005f

		#define MAX_SPRITES 256

		#define MIN_FPS 15

		// 1/60/8
		// So basically, if we are running at perfect 60 fps, then the physics will update 8 times per second.
		#define dt 0.00208333333333333333

	
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
		SDL_Texture* texture;
		uint32_t layer_idx;
		uint32_t frame_idx;
		int32_t z_idx;
		SpriteCellType type;
	} SpriteCell;

	typedef struct SpriteFrame {
		double dur;
		SpriteCell* cells; size_t n_cells;
	} SpriteFrame;

	typedef struct SpriteDesc {
		char* path;
		ivec2s size;
		ivec2s grid_offset;
		ivec2s grid_size;
		SpriteLayer* layers; size_t n_layers;
		SpriteFrame* frames; size_t n_frames;
	} SpriteDesc;

	typedef struct Sprite {
		int32_t idx;
	} Sprite;

	typedef struct Anim {
		Sprite sprite;
		double dt_accumulator;
		uint32_t frame_idx;
		bool ended;
	} Anim;

	bool SpritesEqual(Sprite a, Sprite b);
	void ResetAnim(Anim* anim);
	SDL_EnumerationResult EnumerateSpriteDirectory(void *userdata, const char *dirname, const char *fname);
	int32_t CompareSpriteCells(const SpriteCell* a, const SpriteCell* b);


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

	enum {
		TileType_Empty, // no tile
		TileType_Decor, // cannot collide with
		TileType_Level, // can collide with
	};
	typedef uint32_t TileType;

	typedef struct Tile {
		struct Tile* next;
		ivec2s src;
		TileType type;
	} Tile;

	typedef struct Level {
		ivec2s size;
		Entity player;
		Entity* enemies; size_t n_enemies;
		Tile* tiles; // n_tiles = size.x*size.y
	} Level;

	typedef struct Rect {
		ivec2s min;
		ivec2s max;
	} Rect;

	typedef struct ReplayFrame {
		Entity player;
		Entity* enemies; size_t n_enemies;
	} ReplayFrame;

	typedef struct Context {
		SDL_Window* window;

		SDL_Renderer* renderer;
		bool vsync;

		SDL_Gamepad* gamepad;
		vec2s gamepad_left_stick;

		bool button_left;
		bool button_right;
		bool button_jump;
		bool button_jump_released;
		bool button_attack;

		bool left_mouse_pressed;
		vec2s mouse_pos;

		SDL_Time time;
		double dt_accumulator;
		float refresh_rate;
		
		Level* levels; size_t n_levels;
		size_t level_idx;

		bool running;

		SpriteDesc sprites[MAX_SPRITES];

		ReplayFrame* replay_frames; 
		size_t replay_frame_idx; 
		size_t replay_frame_idx_max;
		size_t c_replay_frames;
		bool paused;
	} Context;

	void ResetGame(Context* ctx);

	ivec2s GetSpriteOrigin(Context* ctx, Sprite sprite, int32_t dir);
	bool GetSpriteHitbox(Context* ctx, Sprite sprite, size_t frame_idx, int32_t dir, Rect* hitbox);
	void DrawSprite(Context* ctx, Sprite sprite, size_t frame_idx, vec2s pos, int32_t dir);
	void DrawSpriteTile(Context* ctx, Sprite tileset, Tile tile, ivec2s pos);
	ivec2s GetTilesetDimensions(Context* ctx, Sprite tileset);

	void UpdateAnim(Context* ctx, Anim* anim, bool loop);
	void DrawAnim(Context* ctx, Anim* anim, vec2s pos, int32_t dir);

	Rect GetEntityHitbox(Context* ctx, Entity* entity);
	void GetEntityHitboxes(Context* ctx, Entity* entity, Rect* h, Rect* lh, Rect* rh, Rect* uh, Rect* dh);
	bool EntitiesIntersect(Context* ctx, Entity* a, Entity* b);
	void DrawEntity(Context* ctx, Entity* entity);

	void UpdatePlayer(Context* ctx);
	void UpdateBoar(Context* ctx, Entity* boar);

	void SetReplayFrame(Context* ctx, size_t replay_frame_idx);

	// This returns a new EntityState instead of setting the 
	// entity state directly because depending on the entity,
	// certain states might not make sense.
	EntityState EntityMoveAndCollide(Context* ctx, Entity* entity, vec2s acc, float fric, float max_vel);

// util.c
	FORCEINLINE vec2s vec2_from_ivec2(ivec2s v) {
		return (vec2s){(float)v.x, (float)v.y};
	}

	FORCEINLINE ivec2s ivec2_from_vec2(vec2s v) {
		return (ivec2s){(int32_t)v.x, (int32_t)v.y};
	}

	FORCEINLINE vec2s glms_vec2_round(vec2s v) {
	    v.x = SDL_roundf(v.x);
	    v.y = SDL_roundf(v.y);
	    return v;
	}

	FORCEINLINE bool RectsIntersect(Rect a, Rect b) {
		bool d0 = b.max.x < a.min.x;
	    bool d1 = a.max.x < b.min.x;
	    bool d2 = b.max.y < a.min.y;
	    bool d3 = a.max.y < b.min.y;
	    return !(d0 | d1 | d2 | d3);
	}

	#if 0
	void DrawCircle(SDL_Renderer* renderer, ivec2s center, int32_t radius) {
		int32_t x = radius;
		int32_t y = 0;
	    
	    SDL_CHECK(SDL_RenderPoint(renderer, (float)(center.x + x), (float)(center.y + y)));
	    SDL_CHECK(SDL_RenderPoint(renderer, (float)(center.x - x), (float)(center.y + y)));
	    SDL_CHECK(SDL_RenderPoint(renderer, (float)(center.x + x), (float)(center.y - y)));
	    SDL_CHECK(SDL_RenderPoint(renderer, (float)(center.x - x), (float)(center.y - y)));

	    SDL_CHECK(SDL_RenderPoint(renderer, (float)(center.x + y), (float)(center.y + x)));
	    SDL_CHECK(SDL_RenderPoint(renderer, (float)(center.x - y), (float)(center.y + x)));
	    SDL_CHECK(SDL_RenderPoint(renderer, (float)(center.x + y), (float)(center.y - x)));
	    SDL_CHECK(SDL_RenderPoint(renderer, (float)(center.x - y), (float)(center.y - x)));

	    int32_t point = 1 - radius;
	    while (x > y)
	    { 
	        ++y;
	        
	        if (point <= 0) {
				point = point + 2*y + 1;
	        }
	        else
	        {
	            --x;
	            point = point + 2*y - 2*x + 1;
	        }
	        
	        if (x < y) {
	            break;
	        }

	        SDL_CHECK(SDL_RenderPoint(renderer, (float)(center.x + x), (float)(center.y + y)));
	        SDL_CHECK(SDL_RenderPoint(renderer, (float)(center.x - x), (float)(center.y + y)));
	        SDL_CHECK(SDL_RenderPoint(renderer, (float)(center.x + x), (float)(center.y - y)));
	        SDL_CHECK(SDL_RenderPoint(renderer, (float)(center.x - x), (float)(center.y - y)));
	        
	        if (x != y)
	        {
		        SDL_CHECK(SDL_RenderPoint(renderer, (float)(center.x + y), (float)(center.y + x)));
		        SDL_CHECK(SDL_RenderPoint(renderer, (float)(center.x - y), (float)(center.y + x)));
		        SDL_CHECK(SDL_RenderPoint(renderer, (float)(center.x + y), (float)(center.y - x)));
		        SDL_CHECK(SDL_RenderPoint(renderer, (float)(center.x - y), (float)(center.y - x)));   
	        }
	    }	
	}

	void DrawCircleFilled(SDL_Renderer* renderer, ivec2s center, int32_t radius) {
		int32_t x = radius;
		int32_t y = 0;
	    
	    SDL_CHECK(SDL_RenderLine(renderer, (float)center.x, (float)center.y, (float)(center.x + x), (float)(center.y + y)));
	    SDL_CHECK(SDL_RenderLine(renderer, (float)center.x, (float)center.y, (float)(center.x - x), (float)(center.y + y)));
	    SDL_CHECK(SDL_RenderLine(renderer, (float)center.x, (float)center.y, (float)(center.x + x), (float)(center.y - y)));
	    SDL_CHECK(SDL_RenderLine(renderer, (float)center.x, (float)center.y, (float)(center.x - x), (float)(center.y - y)));

	    SDL_CHECK(SDL_RenderLine(renderer, (float)center.x, (float)center.y, (float)(center.x + y), (float)(center.y + x)));
	    SDL_CHECK(SDL_RenderLine(renderer, (float)center.x, (float)center.y, (float)(center.x - y), (float)(center.y + x)));
	    SDL_CHECK(SDL_RenderLine(renderer, (float)center.x, (float)center.y, (float)(center.x + y), (float)(center.y - x)));
	    SDL_CHECK(SDL_RenderLine(renderer, (float)center.x, (float)center.y, (float)(center.x - y), (float)(center.y - x)));

	    int32_t point = 1 - radius;
	    while (x > y) { 
	        ++y;
	        
	        if (point <= 0) {
				point = point + 2*y + 1;
	        } else {
	            --x;
	            point = point + 2*y - 2*x + 1;
	        }
	        
	        if (x < y) {
	            break;
	        }

	        SDL_CHECK(SDL_RenderLine(renderer, (float)center.x, (float)center.y, (float)(center.x + x), (float)(center.y + y)));
	        SDL_CHECK(SDL_RenderLine(renderer, (float)center.x, (float)center.y, (float)(center.x - x), (float)(center.y + y)));
	        SDL_CHECK(SDL_RenderLine(renderer, (float)center.x, (float)center.y, (float)(center.x + x), (float)(center.y - y)));
	        SDL_CHECK(SDL_RenderLine(renderer, (float)center.x, (float)center.y, (float)(center.x - x), (float)(center.y - y)));
	        
	        if (x != y) {
		        SDL_CHECK(SDL_RenderLine(renderer, (float)center.x, (float)center.y, (float)(center.x + y), (float)(center.y + x)));
		        SDL_CHECK(SDL_RenderLine(renderer, (float)center.x, (float)center.y, (float)(center.x - y), (float)(center.y + x)));
		        SDL_CHECK(SDL_RenderLine(renderer, (float)center.x, (float)center.y, (float)(center.x + y), (float)(center.y - x)));
		        SDL_CHECK(SDL_RenderLine(renderer, (float)center.x, (float)center.y, (float)(center.x - y), (float)(center.y - x)));   
	        }
	    }
	}
	#endif

	size_t HashString(char* key, size_t len) {
	    if (len == 0) {
	        len = SDL_strlen(key);
	    }
	    return XXH3_64bits((const void*)key, len);
	}

	Entity* GetPlayer(Context* ctx) {
	    return &ctx->levels[ctx->level_idx].player;
	}

	Entity* GetEnemies(Context* ctx, size_t* n_enemies) {
	    SDL_assert(n_enemies);
	    *n_enemies = ctx->levels[ctx->level_idx].n_enemies;
	    return ctx->levels[ctx->level_idx].enemies;
	}

	Tile* GetTiles(Context* ctx, size_t* n_tiles) {
	    SDL_assert(n_tiles);
	    ivec2s size = ctx->levels[ctx->level_idx].size;
	    *n_tiles = (size_t)(size.x * size.y);
	    return ctx->levels[ctx->level_idx].tiles;
	}

	Level* GetCurrentLevel(Context* ctx) {
	    return &ctx->levels[ctx->level_idx];
	}

	SpriteDesc* GetSpriteDesc(Context* ctx, Sprite sprite) {
	    SDL_assert(sprite.idx >= 0 && sprite.idx < MAX_SPRITES);
	    return &ctx->sprites[sprite.idx];
	}

	bool SpritesEqual(Sprite a, Sprite b) {
	    return a.idx == b.idx;
	}

	// Why not just divide by 32768.0f? It's because there is one more negative value than positive value.
	FORCEINLINE float NormInt16(int16_t i16) {
	    int32_t i32 = (int32_t)i16;
	    i32 += 32768;
	    float res = ((float)i32 / 32768.0f) - 1.0f;
	    return res;
	}

	Tile* GetLevelTiles(Level* level, size_t* n_tiles) {
	    SDL_assert(n_tiles);
	    ivec2s size = level->size;
	    *n_tiles = (size_t)(size.x * size.y);
	    return level->tiles;
	}

// sprite variables
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

// sprite.c
	void DrawSprite(Context* ctx, Sprite sprite, size_t frame, vec2s pos, int32_t dir) {
		SpriteDesc* sd = GetSpriteDesc(ctx, sprite);
		SDL_assert(sd->frames && "invalid sprite");
		SpriteFrame* sf = &sd->frames[frame];
		ivec2s origin = GetSpriteOrigin(ctx, sprite, dir);
		for (size_t cell_idx = 0; cell_idx < sf->n_cells; ++cell_idx) {
			SpriteCell* cell = &sf->cells[cell_idx];
			if (cell->texture) {
				SDL_FRect srcrect = {
					0.0f,
					0.0f,
					(float)(cell->size.x),
					(float)(cell->size.y),
				};

				SDL_FRect dstrect = {
					pos.x + (float)(cell->offset.x*dir - origin.x),
					pos.y + (float)(cell->offset.y - origin.y),
					(float)(cell->size.x*dir),
					(float)(cell->size.y),
				};

				if (dir == -1) {
					dstrect.x += (float)sd->size.x;
				}

				SDL_CHECK(SDL_RenderTexture(ctx->renderer, cell->texture, &srcrect, &dstrect));
			}
		}
	}

	ivec2s GetTilesetDimensions(Context* ctx, Sprite tileset) {
		SpriteDesc* sd = GetSpriteDesc(ctx, tileset);
		SDL_assert(sd->n_layers == 1);
		SDL_assert(sd->n_frames == 1);
		SDL_assert(sd->frames[0].n_cells == 1);

		SDL_Texture* texture = sd->frames[0].cells[0].texture;

		return (ivec2s){texture->w/TILE_SIZE, texture->h/TILE_SIZE};
	}

	void DrawSpriteTile(Context* ctx, Sprite tileset, Tile tile, ivec2s pos) {
		SpriteDesc* sd = GetSpriteDesc(ctx, tileset);
		SDL_assert(sd->n_layers == 1);
		SDL_assert(sd->n_frames == 1);
		SDL_assert(sd->frames[0].n_cells == 1);

		SDL_Texture* texture = sd->frames[0].cells[0].texture;

		ivec2s src = tile.src;

		SDL_FRect srcrect = {
			(float)src.x,
			(float)src.y,
			(float)sd->grid_size.x,
			(float)sd->grid_size.y,
		};
		SDL_FRect dstrect = {
			(float)pos.x,
			(float)pos.y,
			(float)sd->grid_size.x,
			(float)sd->grid_size.y,
		};

		SDL_CHECK(SDL_RenderTexture(ctx->renderer, texture, &srcrect, &dstrect));
	}

	bool GetSpriteHitbox(Context* ctx, Sprite sprite, size_t frame_idx, int32_t dir, Rect* hitbox) {
		SDL_assert(hitbox);
		SDL_assert(dir == 1 || dir == -1);
		SpriteDesc* sd = GetSpriteDesc(ctx, sprite);
		SDL_assert(frame_idx < sd->n_frames);
		SpriteFrame* frame = &sd->frames[frame_idx];
		for (size_t cell_idx = 0; cell_idx < frame->n_cells; ++cell_idx) {
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
				--hitbox->max.x;
				--hitbox->max.y;

				ivec2s origin = GetSpriteOrigin(ctx, sprite, dir);
				hitbox->min = glms_ivec2_sub(hitbox->min, origin);
				hitbox->max = glms_ivec2_sub(hitbox->max, origin);

				return true;
			}
		}
		return false;
	}

	ivec2s GetSpriteOrigin(Context* ctx, Sprite sprite, int32_t dir) {
		ivec2s res = {0};

		// Find origin
		SpriteDesc* sd = GetSpriteDesc(ctx, sprite);
		SpriteFrame* frame = &sd->frames[0];
		for (size_t cell_idx = 0; cell_idx < frame->n_cells; ++cell_idx) {
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

		return res;
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
	    SpriteDesc* sd = GetSpriteDesc(ctx, anim->sprite);
		double dur = sd->frames[anim->frame_idx].dur;
		size_t n_frames = sd->n_frames;

	    if (loop || !anim->ended) {
	        anim->dt_accumulator += dt;
	        if (anim->dt_accumulator >= dur) {
	            anim->dt_accumulator = 0.0;
	            ++anim->frame_idx;
	            if (anim->frame_idx >= n_frames) {
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
	    if (entity->anim.sprite.idx != sprite.idx) {
	        sprite_changed = true;
	        ResetAnim(&entity->anim);

	        entity->anim.sprite = sprite;

	    }
	    return sprite_changed;
	}

	bool SetSpriteFromPath(Entity* entity, const char* path) {
	    Sprite sprite = GetSprite((char*)path);
	    return SetSprite(entity, sprite);
	}

	void ResetAnim(Anim* anim) {
	    anim->frame_idx = 0;
	    anim->dt_accumulator = 0.0;
	    anim->ended = false;
	}

	SDL_EnumerationResult EnumerateSpriteDirectory(void *userdata, const char *dirname, const char *fname) {
		Context* ctx = userdata;

		// dirname\fname\file

		char dir_path[1024];
		SDL_CHECK(SDL_snprintf(dir_path, 1024, "%s%s", dirname, fname) >= 0);

		int32_t n_files;
		char** files = SDL_GlobDirectory(dir_path, "*.aseprite", 0, &n_files); SDL_CHECK(files);
		if (n_files > 0) {
			for (size_t file_idx = 0; file_idx < (size_t)n_files; ++file_idx) {
				char* file = files[file_idx];

				char sprite_path[2048];
				SDL_CHECK(SDL_snprintf(sprite_path, sizeof(sprite_path), "%s\\%s", dir_path, file) >= 0);
				SDL_CHECK(SDL_GetPathInfo(sprite_path, NULL));

				Sprite sprite = GetSprite(sprite_path);
				SpriteDesc* sd = &ctx->sprites[sprite.idx];
				SDL_assert(!sd->path && "Collision");

				{
					size_t buf_len = SDL_strlen(sprite_path) + 1;
					sd->path = SDL_malloc(buf_len); SDL_CHECK(sd->path);
					SDL_strlcpy(sd->path, sprite_path, buf_len);
				}
				
				SDL_IOStream* fs = SDL_IOFromFile(sprite_path, "r"); SDL_CHECK(fs);
				
				// LoadSprite
				{
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

						sd->frames[frame_idx].dur = ((double)frame.frame_dur)/1000.0;

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
									.layer_idx = (uint32_t)layer_idx,
									.frame_idx = (uint32_t)frame_idx,
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

										if (SDL_strcmp(sd->layers[chunk->layer_idx].name, "Hitbox") == 0) {
											cell.type = SpriteCellType_Hitbox;
										} else if (SDL_strcmp(sd->layers[chunk->layer_idx].name, "Origin") == 0) {
											cell.type = SpriteCellType_Origin;
										} else {
											// It's the zero-sized array at the end of ASE_CellChunk.
											size_t src_buf_size = chunk_size - sizeof(ASE_CellChunk) - 2; 
											void* src_buf = (void*)((&chunk->compressed_image.h)+1);

											size_t dst_buf_size = sizeof(uint32_t)*cell.size.x*cell.size.y;
											void* dst_buf = SDL_malloc(dst_buf_size); SDL_CHECK(dst_buf);

											size_t res = INFL_ZInflate(dst_buf, dst_buf_size, src_buf, src_buf_size); SDL_assert(res > 0);

											SDL_Surface* surf = SDL_CreateSurfaceFrom(cell.size.x, cell.size.y, SDL_PIXELFORMAT_RGBA32, dst_buf, sizeof(uint32_t)*cell.size.x); SDL_CHECK(surf);
											cell.texture = SDL_CreateTextureFromSurface(ctx->renderer, surf); SDL_CHECK(cell.texture);
											SDL_DestroySurface(surf);
											SDL_free(dst_buf);
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

				SDL_CloseIO(fs);
			}
		} else if (n_files == 0) {
			SDL_CHECK(SDL_EnumerateDirectory(dir_path, EnumerateSpriteDirectory, ctx));
		}
		
		SDL_free(files);
		return SDL_ENUM_CONTINUE;
	}

	void DrawEntity(Context* ctx, Entity* entity) {
	    if (entity->state != EntityState_Inactive) {
	        DrawSprite(ctx, entity->anim.sprite, entity->anim.frame_idx, vec2_from_ivec2(entity->pos), entity->dir);
	    }
	}

	void DrawAnim(Context* ctx, Anim* anim, vec2s pos, int32_t dir) {
	    DrawSprite(ctx, anim->sprite, anim->frame_idx, pos, dir);
	}

// entity.c
	void UpdatePlayer(Context* ctx) {
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

			size_t n_enemies; Entity* enemies = GetEnemies(ctx, &n_enemies);
			for (size_t enemy_idx = 0; enemy_idx < n_enemies; ++enemy_idx) {
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
	}

	void UpdateBoar(Context* ctx, Entity* boar) {
		switch (boar->state) {
		case EntityState_Inactive:
			return;

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
	}

	Rect GetEntityHitbox(Context* ctx, Entity* entity) {
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
			for (res = false, frame_idx = entity->anim.frame_idx; !res && frame_idx >= 0; --frame_idx) {
				res = GetSpriteHitbox(ctx, entity->anim.sprite, (size_t)frame_idx, entity->dir, &hitbox); 
			}
			if (!res && entity->anim.frame_idx == 0) {
				for (frame_idx = 1; !res && frame_idx < (ssize_t)sd->n_frames; ++frame_idx) {
					res = GetSpriteHitbox(ctx, entity->anim.sprite, (size_t)frame_idx, entity->dir, &hitbox);
				}
			}
			SDL_assert(res);
		}

		hitbox.min = glms_ivec2_add(hitbox.min, entity->pos);
		hitbox.max = glms_ivec2_add(hitbox.max, entity->pos);

		return hitbox;
	}

	bool EntitiesIntersect(Context* ctx, Entity* a, Entity* b) {
	    if (a->state == EntityState_Inactive || b->state == EntityState_Inactive) return false;
	    Rect ha = GetEntityHitbox(ctx, a);
	    Rect hb = GetEntityHitbox(ctx, b);
	    return RectsIntersect(ha, hb);
	}

	FORCEINLINE ssize_t GetTileIdx(Level* level, ivec2s pos) {
		return (ssize_t)((pos.x + pos.y*level->size.x)/TILE_SIZE);
	}

	EntityState EntityMoveAndCollide(Context* ctx, Entity* entity, vec2s acc, float fric, float max_vel) {
		Level* level = GetCurrentLevel(ctx);
		size_t n_tiles; Tile* tiles = GetLevelTiles(level, &n_tiles);
		Rect prev_hitbox = GetEntityHitbox(ctx, entity);

		entity->vel = glms_vec2_add(entity->vel, acc);

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
			for (grid_pos.y = prev_hitbox.min.y/TILE_SIZE; grid_pos.y <= prev_hitbox.max.y/TILE_SIZE; ++grid_pos.y) {
				size_t tile_idx = (size_t)(grid_pos.x + grid_pos.y*level->size.x);
				if (tile_idx >= n_tiles) continue;
				Tile tile = tiles[tile_idx];
				if (tile.type == TileType_Level) {
					vel.x = 0.0f;
					horizontal_collision_happened = true;
					break;
				}
			}
		} else if (entity->vel.x > 0.0f && (prev_hitbox.max.x+1) % TILE_SIZE == 0) {
			ivec2s grid_pos;
			grid_pos.x = (prev_hitbox.max.x+1)/TILE_SIZE;
			for (grid_pos.y = prev_hitbox.min.y/TILE_SIZE; grid_pos.y <= prev_hitbox.max.y/TILE_SIZE; ++grid_pos.y) {
				size_t tile_idx = (size_t)(grid_pos.x + grid_pos.y*level->size.x);
				if (tile_idx >= n_tiles) continue;
				Tile tile = tiles[tile_idx];
				if (tile.type == TileType_Level) {
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
			for (grid_pos.x = prev_hitbox.min.x/TILE_SIZE; grid_pos.x <= prev_hitbox.max.x/TILE_SIZE; ++grid_pos.x) {
				size_t tile_idx = (size_t)(grid_pos.x + grid_pos.y*level->size.x);
				if (tile_idx >= n_tiles) continue;
				Tile tile = tiles[tile_idx];
				if (tile.type == TileType_Level) {
					vel.y = 0.0f;
					vertical_collision_happened = true;
					break;
				}
			}
		} else if (entity->vel.y > 0.0f && (prev_hitbox.max.y+1) % TILE_SIZE == 0) {
			ivec2s grid_pos;
			grid_pos.y = (prev_hitbox.max.y+1)/TILE_SIZE;
			for (grid_pos.x = prev_hitbox.min.x/TILE_SIZE; grid_pos.x <= prev_hitbox.max.x/TILE_SIZE; ++grid_pos.x) {
				size_t tile_idx = (size_t)(grid_pos.x + grid_pos.y*level->size.x);
				if (tile_idx >= n_tiles) continue;
				Tile tile = tiles[tile_idx];
				if (tile.type == TileType_Level) {
					vel.y = 0.0f;
					entity->vel.y = 0.0f;
					entity->state = EntityState_Free;
					touching_floor = true;
					vertical_collision_happened = true;
					break;
				}
			}
		}

	    entity->pos_remainder = glms_vec2_add(entity->pos_remainder, vel);
	    entity->pos = glms_ivec2_add(entity->pos, ivec2_from_vec2(glms_vec2_round(entity->pos_remainder)));
	    entity->pos_remainder = glms_vec2_sub(entity->pos_remainder, glms_vec2_round(entity->pos_remainder));

	    Rect hitbox = GetEntityHitbox(ctx, entity);

		ivec2s grid_pos;
		for (grid_pos.y = hitbox.min.y/TILE_SIZE; (!horizontal_collision_happened || !vertical_collision_happened) && grid_pos.y <= hitbox.max.y/TILE_SIZE; ++grid_pos.y) {
			for (grid_pos.x = hitbox.min.x/TILE_SIZE; (!horizontal_collision_happened || !vertical_collision_happened) && grid_pos.x <= hitbox.max.x/TILE_SIZE; ++grid_pos.x) {
				size_t tile_idx = (size_t)(grid_pos.x + grid_pos.y*level->size.x);
				if (tile_idx >= n_tiles) continue;
				Tile tile = tiles[tile_idx];
				if (tile.type == TileType_Level) {
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

		if (!touching_floor && entity->vel.y > 0.0f) {
			return EntityState_Fall;
		} else {
			return entity->state;
		}
	}

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
		ctx = SDL_calloc(1, sizeof(Context)); SDL_CHECK(ctx);
		SDL_CHECK(SDL_GetCurrentTime(&ctx->time));
	}

	// CreateWindowAndRenderer
	{
		SDL_DisplayID display = SDL_GetPrimaryDisplay();
		const SDL_DisplayMode* display_mode = SDL_GetDesktopDisplayMode(display); SDL_CHECK(display_mode);
	#if !FULLSCREEN
		SDL_CHECK(SDL_CreateWindowAndRenderer("LegacyFantasy", display_mode->w/2, display_mode->h/2, SDL_WINDOW_HIGH_PIXEL_DENSITY, &ctx->window, &ctx->renderer));
	#else
		SDL_CHECK(SDL_CreateWindowAndRenderer("LegacyFantasy", display_mode->w, display_mode->h, SDL_WINDOW_HIGH_PIXEL_DENSITY|SDL_WINDOW_FULLSCREEN, &ctx->window, &ctx->renderer));
	#endif

		ctx->vsync = SDL_SetRenderVSync(ctx->renderer, SDL_RENDERER_VSYNC_ADAPTIVE);
		if (!ctx->vsync) {
			ctx->vsync = SDL_SetRenderVSync(ctx->renderer, 1); // fixed refresh rate
		}

		ctx->refresh_rate = display_mode->refresh_rate;

		SDL_CHECK(SDL_SetDefaultTextureScaleMode(ctx->renderer, SDL_SCALEMODE_PIXELART));
		SDL_CHECK(SDL_SetRenderScale(ctx->renderer, (float)(display_mode->w/GAME_WIDTH), (float)(display_mode->h/GAME_HEIGHT)));
		SDL_CHECK(SDL_SetRenderDrawBlendMode(ctx->renderer, SDL_BLENDMODE_BLEND));
	}
	
	// LoadSprites
	{
		SDL_CHECK(SDL_EnumerateDirectory("assets\\legacy_fantasy_high_forest", EnumerateSpriteDirectory, ctx));
	}

	// SortSprites
	{
		for (size_t sprite_idx = 0; sprite_idx < MAX_SPRITES; ++sprite_idx) {
			SpriteDesc* sd = &ctx->sprites[sprite_idx];
			if (sd->path) {
				for (size_t frame_idx = 0; frame_idx < sd->n_frames; ++frame_idx) {
					SpriteFrame* sf = &sd->frames[frame_idx];
					SDL_qsort(sf->cells, sf->n_cells, sizeof(SpriteCell), (SDL_CompareCallback)CompareSpriteCells);
				}
			}
		}
	}

	// LoadLevels
	{
		JSON_Node* head;
		{
			size_t file_len;
			void* file_data = SDL_LoadFile("assets\\levels\\test.ldtk", &file_len); SDL_CHECK(file_data);
			head = JSON_ParseWithLength((const char*)file_data, file_len);
			SDL_free(file_data);
		}
		SDL_assert(HAS_FLAG(head->type, JSON_Object));

		JSON_Node* level_nodes = JSON_GetObjectItem(head, "levels", true);
		JSON_Node* level_node; JSON_ArrayForEach(level_node, level_nodes) {
			++ctx->n_levels;
		}
		ctx->levels = SDL_calloc(ctx->n_levels, sizeof(Level)); SDL_CHECK(ctx->levels);
		size_t level_idx = 0;

		// LoadLevel
		JSON_ArrayForEach(level_node, level_nodes) {
			Level level = {0};

			JSON_Node* w = JSON_GetObjectItem(level_node, "pxWid", true);
			level.size.x = JSON_GetIntValue(w);

			JSON_Node* h = JSON_GetObjectItem(level_node, "pxHei", true);
			level.size.y = JSON_GetIntValue(h);

			size_t n_tiles = (size_t)(level.size.x * level.size.y);
			level.tiles = SDL_calloc(n_tiles, sizeof(Tile)); SDL_CHECK(level.tiles);

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
						++level.n_enemies;
					}
				}
			}

			level.enemies = SDL_calloc(level.n_enemies, sizeof(Entity)); SDL_CHECK(level.enemies);
			Entity* enemy = level.enemies;
			JSON_ArrayForEach(layer_instance, layer_instances) {
				JSON_Node* ident_node = JSON_GetObjectItem(layer_instance, "__identifier", true);
				char* ident = JSON_GetStringValue(ident_node); SDL_assert(ident);
				if (SDL_strcmp(ident, layer_player) == 0) {
					JSON_Node* entity_instances = JSON_GetObjectItem(layer_instance, "entityInstances", true);
					JSON_Node* entity_instance = entity_instances->child;
					JSON_Node* world_x = JSON_GetObjectItem(entity_instance, "__worldX", true);
					JSON_Node* world_y = JSON_GetObjectItem(entity_instance, "__worldY", true);
					level.player.start_pos = (ivec2s){JSON_GetIntValue(world_x), JSON_GetIntValue(world_y)};
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

						Tile tile = {
							.src = src,
							.type = TileType_Decor,
						};
						int32_t dst_idx = (dst.x + dst.y*level.size.x)/TILE_SIZE;
						SDL_assert(dst_idx >= 0 && dst_idx < level.size.x*level.size.y);
						if (level.tiles[dst_idx].type != TileType_Empty) {
							// HACK/TODO: Have multiple layers instead of this.
							level.tiles[dst_idx].next = SDL_calloc(1, sizeof(Tile)); SDL_CHECK(level.tiles[dst_idx].next);
							*level.tiles[dst_idx].next = tile;

						} else {
							level.tiles[dst_idx] = tile;
						}
					}
				}
			}
			ctx->levels[level_idx++] = level;
		}

		int32_t* tiles_collide = NULL; size_t n_tiles_collide = 0;
		JSON_Node* defs = JSON_GetObjectItem(head, "defs", true);
		JSON_Node* tilesets = JSON_GetObjectItem(defs, "tilesets", true);
		bool break_all = false;
		JSON_Node* tileset; JSON_ArrayForEach(tileset, tilesets) {
			JSON_Node* enum_tags = JSON_GetObjectItem(tileset, "enumTags", true);
			JSON_Node* enum_tag; JSON_ArrayForEach(enum_tag, enum_tags) {
				JSON_Node* id_node = JSON_GetObjectItem(enum_tag, "enumValueId", true);
				char* id = JSON_GetStringValue(id_node);
				if (SDL_strcmp(id, "Collide") != 0) continue;
				
				JSON_Node* tile_ids = JSON_GetObjectItem(enum_tag, "tileIds", true);
				JSON_Node* tile_id; JSON_ArrayForEach(tile_id, tile_ids) {
					++n_tiles_collide;
				}
				tiles_collide = SDL_calloc(n_tiles_collide, sizeof(int32_t)); SDL_CHECK(tiles_collide);
				size_t i = 0;
				JSON_ArrayForEach(tile_id, tile_ids) {
					tiles_collide[i++] = JSON_GetIntValue(tile_id);
				}
				break_all = true;
				break;
			}
			if (break_all) break;
		}
		break_all = false;

		for (size_t level_idx = 0; level_idx < ctx->n_levels; ++level_idx) {
			Level* level = &ctx->levels[level_idx];
			size_t n_tiles;
			Tile* tiles = GetLevelTiles(level, &n_tiles);
			for (size_t tile_idx = 0; tile_idx < n_tiles; ++tile_idx) {
				Tile* tile = &tiles[tile_idx];
				ivec2s dim = GetTilesetDimensions(ctx, spr_tiles);
				int32_t src_idx = (tile->src.x + tile->src.y*dim.x)/TILE_SIZE;
				for (size_t tiles_collide_idx = 0; tiles_collide_idx < n_tiles_collide; ++tiles_collide_idx) {
					int32_t i = tiles_collide[tiles_collide_idx];
					if (i == src_idx) {
						tile->type = TileType_Level;
						break;
					}
				}
			}
		}

		SDL_free(tiles_collide);
		JSON_Delete(head);
	}

	// InitReplayFrames
	{
		ctx->c_replay_frames = 1024;
		ctx->replay_frames = SDL_malloc(ctx->c_replay_frames * sizeof(ReplayFrame)); SDL_CHECK(ctx->replay_frames);
	}

	ResetGame(ctx);

	ctx->running = true;
	while (ctx->running) {
		while (ctx->dt_accumulator > dt) {
			// GetInput
			{
				if (!ctx->gamepad) {
					int joystick_count = 0;
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
									SetReplayFrame(ctx, SDL_max(ctx->replay_frame_idx - 1, 0));
								}
								break;
							case SDLK_RIGHT:
								if (!event.key.repeat) {
									ctx->button_right = 1;
								}
								if (ctx->paused) {
									SetReplayFrame(ctx, SDL_min(ctx->replay_frame_idx + 1, ctx->replay_frame_idx_max - 1));
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
			}
			if (!ctx->paused) {
				// UpdateGame
				{
					UpdatePlayer(ctx);
					size_t n_enemies; Entity* enemies = GetEnemies(ctx, &n_enemies);
					for (size_t enemy_idx = 0; enemy_idx < n_enemies; ++enemy_idx) {
						Entity* enemy = &enemies[enemy_idx];
						if (enemy->type == EntityType_Boar) {
							UpdateBoar(ctx, enemy);
						}
					}
				}
				
				// RecordReplayFrame
				{
					ReplayFrame replay_frame = {0};
					
					Level* level = GetCurrentLevel(ctx);
					replay_frame.player = level->player;
					replay_frame.enemies = SDL_malloc(level->n_enemies * sizeof(Entity)); SDL_CHECK(replay_frame.enemies);
					SDL_memcpy(replay_frame.enemies, level->enemies, level->n_enemies * sizeof(Entity));
					replay_frame.n_enemies = level->n_enemies;
						
					ctx->replay_frames[ctx->replay_frame_idx++] = replay_frame;
					if (ctx->replay_frame_idx >= ctx->c_replay_frames - 1) {
						ctx->c_replay_frames *= 8;
						ctx->replay_frames = SDL_realloc(ctx->replay_frames, ctx->c_replay_frames * sizeof(ReplayFrame)); SDL_CHECK(ctx->replay_frames);
					}
					ctx->replay_frame_idx_max = SDL_max(ctx->replay_frame_idx_max, ctx->replay_frame_idx);
				}
			}

	 		ctx->dt_accumulator -= dt;
		}

		// RenderBegin
		{
			SDL_CHECK(SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, 0));
			SDL_CHECK(SDL_RenderClear(ctx->renderer));
		}

		// RenderLevel
		{
			size_t n_enemies; Entity* enemies = GetEnemies(ctx, &n_enemies);
			for (size_t enemy_idx = 0; enemy_idx < n_enemies; ++enemy_idx) {
				Entity* enemy = &enemies[enemy_idx];
				DrawEntity(ctx, enemy);
			}
			DrawEntity(ctx, GetPlayer(ctx));
			size_t n_tiles; Tile* tiles = GetTiles(ctx, &n_tiles);
			Level* level = GetCurrentLevel(ctx);
			for (size_t tile_idx = 0; tile_idx < n_tiles; ++tile_idx) {
				Tile tile = tiles[tile_idx];
				if (tile.type != TileType_Empty) {
					ivec2s pos = {(int32_t)tile_idx % level->size.x, (int32_t)tile_idx / level->size.x};
					pos = glms_ivec2_scale(pos, TILE_SIZE);
					DrawSpriteTile(ctx, spr_tiles, tile, pos);
					
					// HACK/TODO: Have multiple layers instead of this.
					if (tile.next) {
						DrawSpriteTile(ctx, spr_tiles, *tile.next, pos);
					}
				}
			}
		}

		// RenderHitbox
		{
			Entity* player = GetPlayer(ctx);
			Rect hitbox = GetEntityHitbox(ctx, player);
			SDL_CHECK(SDL_SetRenderDrawColor(ctx->renderer, 255, 0, 0, 128));
			SDL_FRect rect = {(float)hitbox.min.x, (float)hitbox.min.y, (float)(hitbox.max.x-hitbox.min.x+1), (float)(hitbox.max.y-hitbox.min.y+1)};
			SDL_CHECK(SDL_RenderFillRect(ctx->renderer, &rect));
		}

		// RenderEnd
		{
			SDL_RenderPresent(ctx->renderer);
		}

		// UpdateTime
		{			
			SDL_Time current_time;
			SDL_CHECK(SDL_GetCurrentTime(&current_time));
			SDL_Time dt_int = current_time - ctx->time;
			const double NANOSECONDS_IN_SECOND = 1000000000.0;
			double dt_double = (double)dt_int / NANOSECONDS_IN_SECOND;
			ctx->dt_accumulator = SDL_min(ctx->dt_accumulator + dt_double, 1.0f/((float)MIN_FPS));
			ctx->time = current_time;
		}	
	}
	
	SDL_Quit();
	return 0;
}

void ResetGame(Context* ctx) {
	ctx->level_idx = 0;
	for (size_t level_idx = 0; level_idx < ctx->n_levels; ++level_idx) {
		Level* level = &ctx->levels[level_idx];
		{
			Entity* player = &level->player;
			player->state = EntityState_Free;
			player->pos = player->start_pos;
			player->vel = (vec2s){0.0f};
			player->dir = 1;
			ResetAnim(&player->anim);
			SetSpriteFromPath(player, "assets\\legacy_fantasy_high_forest\\Character\\Idle\\Idle.aseprite");
		}
		for (size_t enemy_idx = 0; enemy_idx < level->n_enemies; ++enemy_idx) {
			Entity* enemy = &level->enemies[enemy_idx];
			enemy->state = EntityState_Free;
			enemy->pos = enemy->start_pos;
			enemy->vel = (vec2s){0.0f};
			enemy->dir = 1;
			ResetAnim(&enemy->anim);
			if (enemy->type == EntityType_Boar) {
				SetSpriteFromPath(enemy, "assets\\legacy_fantasy_high_forest\\Mob\\Boar\\Idle\\Idle.aseprite");
			}
		}
	}
}

void SetReplayFrame(Context* ctx, size_t replay_frame_idx) {
	ctx->replay_frame_idx = replay_frame_idx;
	Level* level = GetCurrentLevel(ctx);
	ReplayFrame* replay_frame = &ctx->replay_frames[ctx->replay_frame_idx];
	level->player = replay_frame->player;
	SDL_memcpy(level->enemies, replay_frame->enemies, replay_frame->n_enemies * sizeof(Entity));
	level->n_enemies = replay_frame->n_enemies;
}