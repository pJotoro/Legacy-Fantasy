// https://github.com/aseprite/aseprite/blob/main/docs/ase-file-specs.md#color-profile-chunk-0x2007

#pragma pack(push, 1)

typedef struct ASE_Fixed {
	uint16_t val[2];
} ASE_Fixed;

typedef struct ASE_String {
	uint16_t len;
	uint8_t buf[];
} ASE_String;

typedef struct ASE_Point {
	int32_t x;
	int32_t y;
} ASE_Point;

typedef struct ASE_Size {
	int32_t w;
	int32_t h;
} ASE_Size;

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
	ASE_FLAG_LAYER_OPACITY_VALID = 1u,
	ASE_FLAG_LAYER_OPACITY_VALID_FOR_GROUPS = 2u,
	ASE_FLAG_LAYERS_HAVE_UUID = 4u,
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
	int16_t grid_x_pos;
	int16_t grid_y_pos;
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
	ASE_CHUNK_TYPE_OLD_PALETTE = 0x0004u,
	ASE_CHUNK_TYPE_OLD_PALETTE2 = 0x0011u,
	ASE_CHUNK_TYPE_LAYER = 0x2004u,
	ASE_CHUNK_TYPE_CELL = 0x2005u,
	ASE_CHUNK_TYPE_CELL_EXTRA = 0x2006u,
	ASE_CHUNK_TYPE_COLOR_PROFILE = 0x2007u,
	ASE_CHUNK_TYPE_EXTERNAL_FILES = 0x2008u,
	ASE_CHUNK_TYPE_DEPRECATED_MASK = 0x2016u,
	ASE_CHUNK_TYPE_PATH = 0x2017u,
	ASE_CHUNK_TYPE_TAGS = 0x2018u,
	ASE_CHUNK_TYPE_PALETTE = 0x2019u,
	ASE_CHUNK_TYPE_USER_DATA = 0x2020u,
	ASE_CHUNK_TYPE_SLICE = 0x2022u,
	ASE_CHUNK_TYPE_TILESET = 0x2023u,
};
typedef uint16_t ASE_ChunkType;

typedef struct ASE_ChunkHeader {
	uint32_t size;
	ASE_ChunkType type;
} ASE_ChunkHeader;

#if 0
typedef struct ASE_OldPaletteChunk {
	uint16_t n_packets;
	struct {
		uint8_t n_skip_entries;
		uint8_t n_colors;
		struct {
			uint8_t red;
			uint8_t green;
			uint8_t blue;
		} colors[];
	} packets[];
} ASE_OldPaletteChunk;
#endif

enum {
	ASE_LAYER_CHUNK_FLAG_VISIBLE = 1u,
	ASE_LAYER_CHUNK_FLAG_EDITABLE = 2u,
	ASE_LAYER_CHUNK_FLAG_LOCK_MOVEMENT = 4u,
	ASE_LAYER_CHUNK_FLAG_BACKGROUND = 8u,
	ASE_LAYER_CHUNK_FLAG_PREFER_LINKED_CELLS = 16u,
	ASE_LAYER_CHUNK_FLAG_DISPLAY_COLLAPSED = 32u,
	ASE_LAYER_CHUNK_FLAG_REFERENCE_LAYER = 64u,
};
typedef uint16_t ASE_LayerChunkFlags;

enum {
	ASE_LAYER_TYPE_NORMAL = 0u,
	ASE_LAYER_TYPE_GROUP = 1u,
	ASE_LAYER_TYPE_TILEMAP = 2u,
};
typedef uint16_t ASE_LayerType;

enum {
	ASE_BLEND_MODE_NORMAL = 0u,
	ASE_BLEND_MODE_MULTIPLY = 1u,
	ASE_BLEND_MODE_SCREEN = 2u,
	ASE_BLEND_MODE_OVERLAY = 3u,
	ASE_BLEND_MODE_DARKEN = 4u,
	ASE_BLEND_MODE_LIGHTEN = 5u,
	ASE_BLEND_MODE_COLOR_DODGE = 6u,
	ASE_BLEND_MODE_COLOR_BURN = 7u,
	ASE_BLEND_MODE_HARD_LIGHT = 8u,
	ASE_BLEND_MODE_SOFT_LIGHT = 9u,
	ASE_BLEND_MODE_DIFFERENCE = 10u,
	ASE_BLEND_MODE_EXCLUSION = 11u,
	ASE_BLEND_MODE_HUE = 12u,
	ASE_BLEND_MODE_SATURATION = 13u,
	ASE_BLEND_MODE_COLOR = 14u,
	ASE_BLEND_MODE_LUMINOSITY = 15u,
	ASE_BLEND_MODE_ADDITION = 16u,
	ASE_BLEND_MODE_SUBTRACT = 17u,
	ASE_BLEND_MODE_DIVIDE = 18u,
};
typedef uint16_t ASE_BlendMode;

typedef struct ASE_LayerChunk {
	ASE_LayerChunkFlags flags;
	ASE_LayerType layer_type;
	uint16_t layer_child_level;
	uint16_t ignored_default_layer_w;
	uint16_t ignored_default_layer_h;
	ASE_BlendMode blend_mode;
	uint8_t opacity;
	uint8_t reserved0[3];
	ASE_String layer_name;
	#if 0
	uint32_t tileset_idx;
	uint8_t uuid[16];
	#endif
} ASE_LayerChunk;

enum {
	ASE_CELL_TYPE_RAW = 0u,
	ASE_CELL_TYPE_LINKED = 1u,
	ASE_CELL_TYPE_COMPRESSED_IMAGE = 2u,
	ASE_CELL_TYPE_COMPRESSED_TILEMAP = 3u,
};
typedef uint16_t ASE_CellType;

typedef struct ASE_CellChunk {
	uint16_t layer_idx;
	int16_t x;
	int16_t y;
	uint8_t opacity;
	ASE_CellType cell_type;
	int16_t z_idx;
	uint8_t reserved0[5];
	union {
		struct {
			uint16_t w;
			uint16_t h;
			ASE_Pixel pixels[];
		} raw;
		struct {
			uint16_t frame_pos;
		} linked;
		struct {
			uint16_t w;
			uint16_t h;
			ASE_Pixel pixels[];
		} compressed_image;
		struct {
			uint16_t w;
			uint16_t h;
			uint16_t bits_per_tile;
			uint32_t tile_id_bitmask;
			uint32_t x_flip_bitmask;
			uint32_t y_flip_bitmask;
			uint32_t diagonal_flip_bitmask;
			uint8_t reserved1[10];
			uint32_t tiles[];
		} compressed_tilemap;
	};
} ASE_CellChunk;

enum {
	ASE_CELL_CHUNK_EXTRA_FLAG_PRECISE_BOUNDS = 1u,
};
typedef uint32_t ASE_CellChunkExtraFlags;

typedef struct ASE_CellChunkExtra {
	ASE_CellChunkExtraFlags flags;
	ASE_Fixed x;
	ASE_Fixed y;
	ASE_Fixed w;
	ASE_Fixed h;
	uint8_t reserved0[16];
} ASE_CellChunkExtra;

enum {
	ASE_COLOR_PROFILE_TYPE_NONE = 0u,
	ASE_COLOR_PROFILE_TYPE_SRGB = 1u,
	ASE_COLOR_PROFILE_TYPE_EMBEDDED_ICC = 2u,
};
typedef uint16_t ASE_ColorProfileType;

enum {
	COLOR_PROFILE_FLAG_SPECIAL_FIXED_GAMMA = 1u,
};
typedef uint16_t ASE_ColorProfileFlags;

typedef struct ASE_ColorProfileChunk {
	ASE_ColorProfileType type;
	ASE_ColorProfileFlags flags;
	ASE_Fixed fixed_gamma;
	uint8_t reserved0[8];
	uint32_t icc_profile_data_len;
	uint8_t icc_profile_data[];
} ASE_ColorProfileChunk;

#pragma pack(pop)