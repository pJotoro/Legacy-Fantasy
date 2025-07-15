// TODO: opacity
// TODO: blend modes

typedef struct SpriteLayer {
	char* name;
} SpriteLayer;

typedef struct SpriteCell {
	size_t layer_idx;
	size_t frame_idx;
	ssize_t x_offset;
	ssize_t y_offset;
	ssize_t z_idx;
	size_t w; // width relative to x offset
	size_t h; // height relative to y offset
	SDL_Texture* texture;
	
	SDL_Surface* surf;
	void* surf_backing;
} SpriteCell;

typedef struct SpriteFrame {
	size_t dur;
	SpriteCell* cells; size_t n_cells;
} SpriteFrame;

typedef struct SpriteDesc {
	char* path;
	size_t w, h;
	SpriteLayer* layers; size_t n_layers;
	SpriteFrame* frames; size_t n_frames;
} SpriteDesc;

typedef struct Sprite {
	size_t idx;
} Sprite;

int32_t CompareSpriteCells(SpriteCell* a, SpriteCell* b);