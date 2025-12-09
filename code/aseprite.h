#pragma pack(push, 1)

typedef struct ASE_Fixed {
	uint16_t val[2];
} ASE_Fixed;

typedef struct ASE_String {
	uint16_t len;
#if 0
	uint8_t buf[];
#endif
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
	uint16_t num_frames;
	uint16_t w;
	uint16_t h;
	uint16_t color_depth;
	ASE_Flags flags;
	uint16_t reserved0;
	uint32_t reserved1;
	uint32_t reserved2;
	uint8_t transparent_color_entry;
	uint8_t reserved3[3];
	uint16_t num_colors;
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
	uint32_t num_bytes;
	uint16_t magic_number;
	uint16_t reserved0;
	uint16_t frame_dur;
	uint8_t reserved1[2];
	uint32_t num_chunks;
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
	uint16_t w;
	uint16_t h;
#if 0
	ASE_Pixel pixels[];
#endif
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
#if 0
	uint8_t icc_profile_data[];
#endif
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
	uint32_t num_entries;
	uint8_t reserved0[8];
#if 0
	ASE_ExternalFilesEntry entries[];
#endif
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
	uint16_t num_tags;
#if 0
	ASE_Tag tags[];
#endif
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
#if 0
	ASE_String color_name;
#endif
} ASE_PaletteEntry;

typedef struct ASE_PaletteChunk {
	uint32_t num_entries;
	uint32_t first_color_idx_to_change;
	uint32_t last_color_idx_to_change;
	uint8_t reserved0[8];
#if 0
	ASE_PaletteEntry entries[];
#endif
} ASE_PaletteChunk;

#pragma pack(pop)