float NK_TextWidthCallback(nk_handle handle, float height, const char* text, int32_t len) {
	(void)height;
	TTF_Font* font = handle.ptr;
	const int32_t MAX_WIDTH = 256;
	int32_t measured_width;
	size_t measured_length;
	float res = 0.0f;
	if (TTF_MeasureString(font, text, (size_t)len, MAX_WIDTH, &measured_width, &measured_length)) {
		res = (float)measured_width;
	}
	return res;
}

void NK_Render(Context* ctx) {
	const struct nk_command* cmd = NULL;
	nk_foreach(cmd, &ctx->nk.ctx) {
		switch (cmd->type) {
	        case NK_COMMAND_NOP: {
	        	SDL_assert(false);
	        } break;
		    case NK_COMMAND_SCISSOR: {
	        	const struct nk_command_scissor* c = (const struct nk_command_scissor*)cmd;
	        	const SDL_Rect clip = {c->x, c->y, c->w, c->h};
	        	SDL_CHECK(SDL_SetRenderClipRect(ctx->renderer, &clip));
		    } break;
		    case NK_COMMAND_LINE: {
	        	SDL_assert(false);
		    } break;
		    case NK_COMMAND_CURVE: {
	        	SDL_assert(false);
		    } break;
		    case NK_COMMAND_RECT: {
	        	const struct nk_command_rect* c = (const struct nk_command_rect*)cmd;
	        	SDL_FRect rect = {(float)c->x, (float)c->y, (float)c->w, (float)c->h};
	        	SDL_CHECK(SDL_SetRenderDrawColor(ctx->renderer, c->color.r, c->color.g, c->color.b, c->color.a));
	        	SDL_CHECK(SDL_RenderRect(ctx->renderer, &rect));
		    } break;
		    case NK_COMMAND_RECT_FILLED: {
	        	const struct nk_command_rect_filled* c = (const struct nk_command_rect_filled*)cmd;
	        	SDL_FRect rect = {(float)c->x, (float)c->y, (float)c->w, (float)c->h};
	        	SDL_CHECK(SDL_SetRenderDrawColor(ctx->renderer, c->color.r, c->color.g, c->color.b, c->color.a));
	        	SDL_CHECK(SDL_RenderFillRect(ctx->renderer, &rect));
		    } break;
		    case NK_COMMAND_RECT_MULTI_COLOR: {
	        	SDL_assert(false);
		    } break;
		    case NK_COMMAND_CIRCLE: {
	        	const struct nk_command_circle* c = (const struct nk_command_circle*)cmd;
	        	SDL_CHECK(SDL_SetRenderDrawColor(ctx->renderer, c->color.r, c->color.g, c->color.b, c->color.a));
	        	DrawCircle(ctx->renderer, (ivec2s){c->x + c->w/2, c->y + c->h/2}, c->w/2);
		    } break;
		    case NK_COMMAND_CIRCLE_FILLED: {
	        	const struct nk_command_circle_filled* c = (const struct nk_command_circle_filled*)cmd;
	        	SDL_CHECK(SDL_SetRenderDrawColor(ctx->renderer, c->color.r, c->color.g, c->color.b, c->color.a));
	        	DrawCircleFilled(ctx->renderer, (ivec2s){c->x + c->w/2, c->y + c->h/2}, c->w/2);	        	
		    } break;
		    case NK_COMMAND_ARC: {
	        	SDL_assert(false);
		    } break;
		    case NK_COMMAND_ARC_FILLED: {
	        	SDL_assert(false);
		    } break;
		    case NK_COMMAND_TRIANGLE: {
	        	const struct nk_command_triangle* c = (const struct nk_command_triangle*)cmd;
	        	SDL_Vertex vertices[] = {
	        		{
	        			.position = {c->a.x, c->a.y},
	        			.color = {c->color.r, c->color.g, c->color.b, c->color.a},
	        		},
	        		{
	        			.position = {c->b.x, c->b.y},
	        			.color = {c->color.r, c->color.g, c->color.b, c->color.a},
	        		},
	        		{
	        			.position = {c->c.x, c->c.y},
	        			.color = {c->color.r, c->color.g, c->color.b, c->color.a},
	        		},
	        	};
	        	SDL_CHECK(SDL_RenderGeometry(ctx->renderer, NULL, vertices, 3, NULL, 0));

		    } break;
		    case NK_COMMAND_TRIANGLE_FILLED: {
	        	const struct nk_command_triangle_filled* c = (const struct nk_command_triangle_filled*)cmd;
	        	SDL_Vertex vertices[] = {
	        		{
	        			.position = {c->a.x, c->a.y},
	        			.color = {c->color.r, c->color.g, c->color.b, c->color.a},
	        		},
	        		{
	        			.position = {c->b.x, c->b.y},
	        			.color = {c->color.r, c->color.g, c->color.b, c->color.a},
	        		},
	        		{
	        			.position = {c->c.x, c->c.y},
	        			.color = {c->color.r, c->color.g, c->color.b, c->color.a},
	        		},
	        	};
	        	SDL_CHECK(SDL_RenderGeometry(ctx->renderer, NULL, vertices, 3, NULL, 0));
		    } break;
		    case NK_COMMAND_POLYGON: {
	        	SDL_assert(false);
		    } break;
		    case NK_COMMAND_POLYGON_FILLED: {
	        	SDL_assert(false);
		    } break;
		    case NK_COMMAND_POLYLINE: {
	        	SDL_assert(false);
		    } break;
		    case NK_COMMAND_TEXT: {
	        	const struct nk_command_text* c = (const struct nk_command_text*)cmd;
	        	TTF_Text* text = TTF_CreateText(ctx->text_engine, ctx->font_roboto_regular, c->string, (size_t)c->length); SDL_CHECK(text);
	        	// TODO: text color
	        	SDL_CHECK(TTF_DrawRendererText(text, (float)c->x, (float)c->y));
	        	TTF_DestroyText(text);
		    } break;
		    case NK_COMMAND_IMAGE: {
	        	SDL_assert(false);
		    } break;
		    case NK_COMMAND_CUSTOM: {
	        	SDL_assert(false);
		    } break;
		}
	}
}

void NK_HandleEvent(Context* ctx, SDL_Event* event)
{
    int ctrl_down = SDL_GetModState() & (SDL_KMOD_LCTRL | SDL_KMOD_RCTRL);

    switch(event->type)
    {
        case SDL_EVENT_KEY_UP: /* KEYUP & KEYDOWN share same routine */
        case SDL_EVENT_KEY_DOWN:
            {
                bool down = event->type == SDL_EVENT_KEY_DOWN;
                switch(event->key.key)
                {
                    case SDLK_RSHIFT: /* RSHIFT & LSHIFT share same routine */
                    case SDLK_LSHIFT:    nk_input_key(&ctx->nk.ctx, NK_KEY_SHIFT, down); break;
                    case SDLK_DELETE:    nk_input_key(&ctx->nk.ctx, NK_KEY_DEL, down); break;

                    case SDLK_KP_ENTER:
                    case SDLK_RETURN:    nk_input_key(&ctx->nk.ctx, NK_KEY_ENTER, down); break;

                    case SDLK_TAB:       nk_input_key(&ctx->nk.ctx, NK_KEY_TAB, down); break;
                    case SDLK_BACKSPACE: nk_input_key(&ctx->nk.ctx, NK_KEY_BACKSPACE, down); break;
                    case SDLK_HOME:      nk_input_key(&ctx->nk.ctx, NK_KEY_TEXT_START, down);
                                         nk_input_key(&ctx->nk.ctx, NK_KEY_SCROLL_START, down); break;
                    case SDLK_END:       nk_input_key(&ctx->nk.ctx, NK_KEY_TEXT_END, down);
                                         nk_input_key(&ctx->nk.ctx, NK_KEY_SCROLL_END, down); break;
                    case SDLK_PAGEDOWN:  nk_input_key(&ctx->nk.ctx, NK_KEY_SCROLL_DOWN, down); break;
                    case SDLK_PAGEUP:    nk_input_key(&ctx->nk.ctx, NK_KEY_SCROLL_UP, down); break;
                    case SDLK_Z:         nk_input_key(&ctx->nk.ctx, NK_KEY_TEXT_UNDO, down && ctrl_down); break;
                    case SDLK_R:         nk_input_key(&ctx->nk.ctx, NK_KEY_TEXT_REDO, down && ctrl_down); break;
                    case SDLK_C:         nk_input_key(&ctx->nk.ctx, NK_KEY_COPY, down && ctrl_down); break;
                    case SDLK_V:         nk_input_key(&ctx->nk.ctx, NK_KEY_PASTE, down && ctrl_down); break;
                    case SDLK_X:         nk_input_key(&ctx->nk.ctx, NK_KEY_CUT, down && ctrl_down); break;
                    case SDLK_B:         nk_input_key(&ctx->nk.ctx, NK_KEY_TEXT_LINE_START, down && ctrl_down); break;
                    case SDLK_E:         nk_input_key(&ctx->nk.ctx, NK_KEY_TEXT_LINE_END, down && ctrl_down); break;
                    case SDLK_UP:        nk_input_key(&ctx->nk.ctx, NK_KEY_UP, down); break;
                    case SDLK_DOWN:      nk_input_key(&ctx->nk.ctx, NK_KEY_DOWN, down); break;
                    case SDLK_A:
                        if (ctrl_down)
                            nk_input_key(&ctx->nk.ctx,NK_KEY_TEXT_SELECT_ALL, down);
                        break;
                    case SDLK_LEFT:
                        if (ctrl_down)
                            nk_input_key(&ctx->nk.ctx, NK_KEY_TEXT_WORD_LEFT, down);
                        else nk_input_key(&ctx->nk.ctx, NK_KEY_LEFT, down);
                        break;
                    case SDLK_RIGHT:
                        if (ctrl_down)
                            nk_input_key(&ctx->nk.ctx, NK_KEY_TEXT_WORD_RIGHT, down);
                        else nk_input_key(&ctx->nk.ctx, NK_KEY_RIGHT, down);
                        break;
                    default:
                    	break;
                }
            }
            break;

        case SDL_EVENT_MOUSE_BUTTON_UP: /* MOUSEBUTTONUP & MOUSEBUTTONDOWN share same routine */
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            {
                bool down = event->type == SDL_EVENT_MOUSE_BUTTON_DOWN;
                const int32_t x = (int32_t)event->button.x, y = (int32_t)event->button.y;
                switch(event->button.button)
                {
                    case SDL_BUTTON_LEFT:
                        if (event->button.clicks > 1)
                            nk_input_button(&ctx->nk.ctx, NK_BUTTON_DOUBLE, x, y, down);
                        nk_input_button(&ctx->nk.ctx, NK_BUTTON_LEFT, x, y, down); break;
                    case SDL_BUTTON_MIDDLE: nk_input_button(&ctx->nk.ctx, NK_BUTTON_MIDDLE, x, y, down); break;
                    case SDL_BUTTON_RIGHT:  nk_input_button(&ctx->nk.ctx, NK_BUTTON_RIGHT, x, y, down); break;
                }
            }
            break;

        case SDL_EVENT_MOUSE_MOTION:
            nk_input_motion(&ctx->nk.ctx, (int32_t)event->motion.x, (int32_t)event->motion.y);
            break;

        case SDL_EVENT_TEXT_INPUT:
            {
                nk_glyph glyph;
                memcpy(glyph, event->text.text, NK_UTF_SIZE);
                nk_input_glyph(&ctx->nk.ctx, glyph);
            }
            break;

        case SDL_EVENT_MOUSE_WHEEL:
            nk_input_scroll(&ctx->nk.ctx,nk_vec2(event->wheel.x, event->wheel.y));
            break;

        default:
        	break;
    }  
}

typedef struct NK_DrawTileCallbackData {
	Context* ctx;
	Sprite sprite;
	ivec2s tile;
	vec2s pos;
} NK_DrawTileCallbackData;

void NK_DrawTileCallback(void* canvas, int16_t x, int16_t y, uint16_t w, uint16_t h, nk_handle callback_data) {
	(void)canvas; (void)x; (void)y; (void)w; (void)h;

	NK_DrawTileCallbackData* data = (NK_DrawTileCallbackData*)callback_data.ptr;
	DrawSpriteTile(data->ctx, data->sprite, data->tile, data->pos);

	SDL_Log("Doing this");

	SDL_free(data);
}

void NK_UpdateUI(Context* gctx) {
	struct nk_context* ctx = &gctx->nk.ctx;

	if (nk_begin(ctx, "UI", nk_rect(5, 5, 600, 1000),
	    NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|NK_WINDOW_MINIMIZABLE)) {
		float height = 250.0f;
		int cols = 1;
	    nk_layout_row_dynamic(ctx, height, cols);

		if (nk_group_begin(ctx, "Variables", NK_WINDOW_TITLE|NK_WINDOW_BORDER)) {
			float height = 20.0f;
			int cols = 2;
		    nk_layout_row_dynamic(ctx, height, cols);

			#define nk_varf(ctx, var, min, max, incr) STMT( \
				nk_labelf(ctx, NK_TEXT_LEFT, STRINGIFY(var: %f), var); \
				nk_slider_float(ctx, min, &var, max, incr); \
			)
			#define nk_vari(ctx, var, min, max, incr) STMT( \
				nk_labelf(ctx, NK_TEXT_LEFT, STRINGIFY(var: %d), var); \
				nk_slider_int(ctx, min, &var, max, incr); \
			)
			nk_varf(ctx, GRAVITY, 0.0f, 1.0f, 0.01f);
			nk_varf(ctx, PLAYER_ACC, 0.0f, 1.0f, 0.01f);
			nk_varf(ctx, PLAYER_FRIC, 0.0f, 1.0f, 0.01f);
			nk_varf(ctx, PLAYER_MAX_VEL, 0.0f, 10.0f, 0.1f);
			nk_varf(ctx, PLAYER_JUMP, 0.0f, 30.0f, 0.5f);
			nk_vari(ctx, TILE_SIZE, 0, 256, 2);
			nk_vari(ctx, PLAYER_JUMP_PERIOD, 0, 10, 1);
			
			nk_group_end(ctx);
		}

		height = 432.0f;
		nk_layout_row_static(ctx, height, 432, cols);

		if (nk_group_begin(ctx, "Tiles", NK_WINDOW_TITLE|NK_WINDOW_BORDER)) {
			nk_layout_space_begin(ctx, NK_LAYOUT_STATIC, 16, 25*25);
			// static bool printed = false;
			static Sprite tiles;
			static bool initialized_tiles = false;
			if (!initialized_tiles) {
				initialized_tiles = true;
				tiles = GetSprite("assets\\legacy_fantasy_high_forest\\Assets\\Tiles.aseprite");
			}
			for (ivec2s tile = {0, 0}; tile.y < 25; ++tile.y) {
				for (tile.x = 0; tile.x < 25; ++tile.x) {
					nk_layout_space_push(ctx, (struct nk_rect){(float)tile.x, (float)tile.y, 16.0f, 16.0f});

					struct nk_vec2 current_pos = nk_layout_space_to_screen(ctx, (struct nk_vec2){(float)tile.x*16.0f, (float)tile.y*16.0f});

					struct nk_command_buffer* cb = nk_window_get_canvas(ctx);
					NK_DrawTileCallbackData* data = SDL_malloc(sizeof(NK_DrawTileCallbackData)); // TODO: This is really bad.
					*data = (NK_DrawTileCallbackData){gctx, tiles, tile, (vec2s){current_pos.x, current_pos.y}};

					nk_push_custom(cb, (struct nk_rect){0}, NK_DrawTileCallback, nk_handle_ptr((void*)data));

					

				}
			}
			// printed = true;
			nk_layout_space_end(ctx);

			nk_group_end(ctx);
		}

		// if (nk_group_begin(ctx, "Sprites", NK_WINDOW_TITLE|NK_WINDOW_BORDER)) {
		// 	float height = 70.0f;
		// 	int cols = 1;
		//     nk_layout_row_dynamic(ctx, height, cols);

		// 	for (size_t sprite_idx = 0; sprite_idx < MAX_SPRITES; sprite_idx += 1) {
		// 		SpriteDesc* sprite = &gctx->sprites[sprite_idx];
		// 		if (sprite->path) {
		// 			if (nk_group_begin(ctx, sprite->path, NK_WINDOW_TITLE|NK_WINDOW_NO_SCROLLBAR)) {
		// 				float height = 20.0f;
		// 				int item_width = 80;
		// 				int cols = 3;
		// 				nk_layout_row_static(ctx, height, item_width, cols);

		// 				nk_value_int(ctx, "w", sprite->size.x);
		// 				nk_value_int(ctx, "h", sprite->size.y);
		// 				nk_value_uint(ctx, "frames", (uint32_t)sprite->n_frames);

						

		// 				// cols = 1;
		// 				// nk_layout_row_static(ctx, height, item_width, cols);

		// 				// nk_label(ctx, "Layers:", NK_TEXT_LEFT);
		// 				// for (size_t layer_idx = 0; layer_idx < sprite->n_layers; layer_idx += 1) {
		// 				// 	nk_label(ctx, sprite->layers[layer_idx].name, NK_TEXT_CENTERED);
		// 				// }

		// 				nk_group_end(ctx);
		// 			}
		// 		}
		// 	}

		// 	nk_group_end(ctx);
		// }
	}
	nk_end(ctx);
}

struct nk_image NK_GetImage(const SpriteDesc* sprite, size_t frame_idx) {
	return (struct nk_image){
		// .handle = {.ptr = (void*)sprite->texture},
		.w = (nk_ushort)sprite->size.x,
		.h = (nk_ushort)sprite->size.y,
		.region = {0, 0, (nk_ushort)((size_t)sprite->size.x*frame_idx), (nk_ushort)sprite->size.y},
	};
}