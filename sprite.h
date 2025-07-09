typedef struct SpriteLayer {
	char* name;
	SDL_Texture* texture;
} SpriteLayer;

typedef struct SpriteFrame {
	size_t dur;
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

void LoadSprite(SDL_IOStream* fs, SpriteDesc* sd);