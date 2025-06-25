#pragma pack(push, 1)

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

// https://github.com/aseprite/aseprite/blob/main/docs/ase-file-specs.md#header
typedef struct ASE_Header {
	uint32_t file_size;
	uint16_t magic_number;
	uint16_t n_frames;
	uint16_t w;
	uint16_t h;
	uint16_t color_depth;
	ASE_Flags flags;
	uint16_t deprecated_speed;
	uint32_t __ignore0;
	uint32_t __ignore1;
	uint8_t transparent_color_entry;
	uint8_t __ignore2[3];
	uint16_t n_colors;
	uint8_t pixel_w;
	uint8_t pixel_h;
	int16_t grid_x_pos;
	int16_t grid_y_pos;
	uint16_t grid_w;
	uint16_t grid_h;
	uint8_t __ignore3[84];

} ASE_Header;
static_assert(sizeof(ASE_Header) == 128);

// https://github.com/aseprite/aseprite/blob/main/docs/ase-file-specs.md#frames
typedef struct ASE_Frame {
	uint32_t bytes;
	uint16_t magic_number;
	uint16_t old_n_chunks;
	uint16_t frame_dur;
	uint8_t __ignore0[2];
	uint32_t n_chunks;
} ASE_Frame;
static_assert(sizeof(ASE_Frame) == 16);

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

// https://github.com/aseprite/aseprite/blob/main/docs/ase-file-specs.md#layer-chunk-0x2004
typedef struct ASE_LayerChunk {
	ASE_LayerChunkFlags flags;
	ASE_LayerType layer_type;
	uint16_t layer_child_level;
	uint16_t ignored_default_layer_w;
	uint16_t ignored_default_layer_h;
	ASE_BlendMode blend_mode;
	uint8_t opacity;
	uint8_t __ignored0[3];
	ASE_String layer_name;
	#if 0
	uint32_t tileset_idx;
	uint8_t uuid[16];
	#endif
} ASE_LayerChunk;

// https://github.com/aseprite/aseprite/blob/main/docs/ase-file-specs.md#cel-chunk-0x2005
// typedef struct ASE_CellChunk {
	
// } ASE_CellChunk;

#pragma pack(pop)