#include "core/nes.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>


static uint8_t ROM_BUFFER[0x400000]; // 4MiB
static struct NES_Core nes_core;


static bool read_file(const char* path, uint8_t* out_buf, size_t* out_size) {
	FILE* f = fopen(path, "rb");
	if (!f) {
		return false;
	}

	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);

	if (size <= 0) {
		return false;
	}

	fread(out_buf, 1, size, f);
	*out_size = (size_t)size;
	fclose(f);

	return true;
}

struct SDL_Ctx {
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* texture;
};

#define SCALE 3
static struct SDL_Ctx create_sdl_ctx(const char* name, int x, int y, int w, int h, uint32_t format) {
	struct SDL_Ctx ctx = {0};
	
	ctx.window = SDL_CreateWindow(name, x, y, w * SCALE, h * SCALE, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	ctx.renderer = SDL_CreateRenderer(ctx.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	ctx.texture = SDL_CreateTexture(ctx.renderer, format, SDL_TEXTUREACCESS_STREAMING, w, h);

	return ctx;
}

static void destroy_sdl_ctx(struct SDL_Ctx* ctx) {
	SDL_DestroyTexture(ctx->texture);
	SDL_DestroyRenderer(ctx->renderer);
	SDL_DestroyWindow(ctx->window);
}

static void update_ctx(struct SDL_Ctx* ctx, const void* new_pixels, size_t size) {
	void* pixles; int pitch;
	SDL_LockTexture(ctx->texture, NULL, &pixles, &pitch);
	memcpy(pixles, new_pixels, size);
	SDL_UnlockTexture(ctx->texture);
}

static void render_ctx(struct SDL_Ctx* ctx) {
	SDL_RenderClear(ctx->renderer);
	SDL_RenderCopy(ctx->renderer, ctx->texture, NULL, NULL);
	SDL_RenderPresent(ctx->renderer);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "missing args\n");
        exit(-1);
    }
	
	size_t rom_size = 0;
	if (!read_file(argv[1], ROM_BUFFER, &rom_size)) {
		return false;
	}

    if (NES_OK != NES_loadrom(&nes_core, ROM_BUFFER, rom_size)) {
        fprintf(stderr, "failed to open file: %s\n", argv[1]);
        exit(-1);
    }

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);


	struct SDL_Ctx core_sdl_ctx = create_sdl_ctx("TotalNES", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, NES_SCREEN_WIDTH, NES_SCREEN_HEIGHT, SDL_PIXELFORMAT_BGR555);
	struct SDL_Ctx pattern0_sdl_ctx = create_sdl_ctx("Pattern Table 0", 0, SDL_WINDOWPOS_CENTERED, 128, 128, SDL_PIXELFORMAT_RGB888);
	struct SDL_Ctx pattern1_sdl_ctx = create_sdl_ctx("Pattern Table 1", 1280 - (1280/10), SDL_WINDOWPOS_CENTERED, 128, 128, SDL_PIXELFORMAT_RGB888);

    bool quit = false;
	uint8_t pallete_num = 0;

    while (!quit) {
        SDL_Event e;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) quit = true;

			if (e.type == SDL_KEYDOWN) {
				// bool down = e.type == SDL_KEYDOWN;

				switch (e.key.keysym.sym) {
					case SDLK_1:
						pallete_num++;
						break;
				}
			}
		}

		NES_run_frame(&nes_core);

		update_ctx(&core_sdl_ctx, nes_core.ppu.pixels, sizeof(nes_core.ppu.pixels));
		render_ctx(&core_sdl_ctx);

		struct NES_PatternTableGfx pattern0 = NES_ppu_get_pattern_table(&nes_core, 0, pallete_num);
		struct NES_PatternTableGfx pattern1 = NES_ppu_get_pattern_table(&nes_core, 1, pallete_num);
    
		update_ctx(&pattern0_sdl_ctx, pattern0.pixels, sizeof(pattern0.pixels));
		render_ctx(&pattern0_sdl_ctx);

		update_ctx(&pattern1_sdl_ctx, pattern1.pixels, sizeof(pattern1.pixels));
		render_ctx(&pattern1_sdl_ctx);
		
	}

	destroy_sdl_ctx(&core_sdl_ctx);
	destroy_sdl_ctx(&pattern0_sdl_ctx);
	destroy_sdl_ctx(&pattern1_sdl_ctx);

	SDL_Quit();


    return 0;
}
