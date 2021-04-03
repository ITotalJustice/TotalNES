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
	ssize_t size = ftell(f);
	fseek(f, 0, SEEK_SET);

	if (size <= 0) {
		return false;
	}

	fread(out_buf, 1, size, f);
	*out_size = (size_t)size;
	fclose(f);

	return true;
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

	SDL_Window* window = SDL_CreateWindow("TotalNES", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, NES_SCREEN_WIDTH, NES_SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR555, SDL_TEXTUREACCESS_STREAMING, NES_SCREEN_WIDTH, NES_SCREEN_HEIGHT);
	SDL_SetWindowMinimumSize(window, NES_SCREEN_WIDTH, NES_SCREEN_HEIGHT);
    SDL_ShowCursor(SDL_DISABLE);

    bool quit = false;

    while (!quit) {
        SDL_Event e;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) quit = true;
		}

		NES_run_frame(&nes_core);

		void* pixles; int pitch;
        SDL_LockTexture(texture, NULL, &pixles, &pitch);
		memcpy(pixles, nes_core.ppu.pixels, sizeof(nes_core.ppu.pixels));
		SDL_UnlockTexture(texture);

		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();


    return 0;
}
