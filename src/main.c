#include "core/nes.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <SDL2/SDL.h>


static uint8_t ROM_BUFFER[0x400000] = {0}; // 4MiB
static struct NES_Core nes_core = {0};
static SDL_AudioDeviceID AUDIO_DEVICE_ID = 0;


static bool read_file(const char* path, uint8_t* out_buf, size_t* out_size) {
	FILE* f = fopen(path, "rb");
	
	if (!f) {
		goto fail;
	}

	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);

	if (size <= 0) {
		goto fail;
	}

	fread(out_buf, 1, size, f);
	*out_size = (size_t)size;
	fclose(f);

	return true;

fail:
	if (f) {
		fclose(f);
	}

	return false;
}

static void core_apu_callback(struct NES_Core* nes, void* user, struct NES_ApuCallbackData* data) {
    (void)nes; (void)user;

    while ((SDL_GetQueuedAudioSize(AUDIO_DEVICE_ID)) > (1024 * 4)) {
        SDL_Delay(1);
    }

    SDL_QueueAudio(AUDIO_DEVICE_ID, data->samples, sizeof(data->samples));
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "missing args\n");
        exit(-1);
    }

	size_t rom_size = 0;
	if (!read_file(argv[1], ROM_BUFFER, &rom_size)) {
		exit(-1);
	}

    if (!NES_loadrom(&nes_core, ROM_BUFFER, rom_size)) {
        fprintf(stderr, "failed to open file: %s\n", argv[1]);
        exit(-1);
    }

    // SDL_setenv("SDL_AUDIODRIVER", "disk", 1);
    
    SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS);

	const SDL_AudioSpec wanted = {
        .freq = 48000,
        .format = AUDIO_S8,
        .channels = 1,
        .silence = 0, // calculated
        .samples = 512,
        .padding = 0,
        .size = 0, // calculated
        .callback = NULL,
        .userdata = NULL
    };

    AUDIO_DEVICE_ID = SDL_OpenAudioDevice(NULL, 0, &wanted, NULL, 0);

    if (AUDIO_DEVICE_ID == 0) {
        printf("failed to find valid audio device\n");
        return -1;
    }
    
    SDL_PauseAudioDevice(AUDIO_DEVICE_ID, 0);

	// set the audio callback
	NES_set_apu_callback(&nes_core, core_apu_callback, NULL);

	SDL_Window* window = SDL_CreateWindow(
		"TotalNES",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		NES_SCREEN_WIDTH, NES_SCREEN_HEIGHT,
		SDL_WINDOW_SHOWN
	);

	SDL_Renderer* renderer = SDL_CreateRenderer(
		window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
	);
	
	SDL_Texture* texture = SDL_CreateTexture(
		renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING,
		NES_SCREEN_WIDTH, NES_SCREEN_HEIGHT
	);

    bool quit = false;

    while (!quit) {
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) quit = true;

			if (e.type == SDL_KEYDOWN) {
				bool down = e.type == SDL_KEYDOWN;

				switch (e.key.keysym.sym) {
					case SDLK_x: NES_set_button(&nes_core, NES_BUTTON_A, down); break;
					case SDLK_z: NES_set_button(&nes_core, NES_BUTTON_B, down); break;
					case SDLK_RETURN: NES_set_button(&nes_core, NES_BUTTON_START, down); break;
					case SDLK_SPACE: NES_set_button(&nes_core, NES_BUTTON_SELECT, down); break;
					case SDLK_DOWN: NES_set_button(&nes_core, NES_BUTTON_DOWN, down); break;
					case SDLK_UP: NES_set_button(&nes_core, NES_BUTTON_UP, down); break;
					case SDLK_LEFT: NES_set_button(&nes_core, NES_BUTTON_LEFT, down); break;
					case SDLK_RIGHT: NES_set_button(&nes_core, NES_BUTTON_RIGHT, down); break;
				}
			}
		}

		NES_run_frame(&nes_core);

		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);
	}


	SDL_CloseAudioDevice(AUDIO_DEVICE_ID);
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();

    return 0;
}
