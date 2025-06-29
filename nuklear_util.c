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
	        	DrawCircle(ctx->renderer, c->x + c->w/2, c->y + c->h/2, c->w/2);
		    } break;
		    case NK_COMMAND_CIRCLE_FILLED: {
	        	const struct nk_command_circle_filled* c = (const struct nk_command_circle_filled*)cmd;
	        	SDL_CHECK(SDL_SetRenderDrawColor(ctx->renderer, c->color.r, c->color.g, c->color.b, c->color.a));
	        	DrawCircleFilled(ctx->renderer, c->x + c->w/2, c->y + c->h/2, c->w/2);	        	
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