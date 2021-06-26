#include <nes.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <SDL.h>


static uint8_t ROM_BUFFER[0x400000] = {0}; // 4MiB
static struct NES_Core nes = {0};
static int scale = 2;

static bool read_file(const char* path, uint8_t* out_buf, size_t* out_size)
{
    FILE* f = fopen(path, "rb");
    
    if (!f)
    {
        goto fail;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size <= 0)
    {
        goto fail;
    }

    fread(out_buf, 1, size, f);
    *out_size = (size_t)size;
    fclose(f);

    return true;

fail:
    if (f)
    {
        fclose(f);
    }

    return false;
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "missing args\n");
        exit(-1);
    }

    size_t rom_size = 0;

    if (!read_file(argv[1], ROM_BUFFER, &rom_size))
    {
        exit(-1);
    }

    if (!NES_loadrom(&nes, ROM_BUFFER, rom_size))
    {
        fprintf(stderr, "failed to open file: %s\n", argv[1]);
        exit(-1);
    }

    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow(
        "TotalNES",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        NES_SCREEN_WIDTH * scale, NES_SCREEN_HEIGHT * scale,
        0
    );

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    SDL_Texture* texture = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING,
        NES_SCREEN_WIDTH, NES_SCREEN_HEIGHT
    );

    bool quit = false;

    while (!quit)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT) quit = true;

            if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
            {
                bool down = e.type == SDL_KEYDOWN;

                switch (e.key.keysym.sym)
                {
                    case SDLK_x: NES_set_button(&nes, NES_BUTTON_A, down); break;
                    case SDLK_z: NES_set_button(&nes, NES_BUTTON_B, down); break;
                    case SDLK_RETURN: NES_set_button(&nes, NES_BUTTON_START, down); break;
                    case SDLK_SPACE: NES_set_button(&nes, NES_BUTTON_SELECT, down); break;
                    case SDLK_DOWN: NES_set_button(&nes, NES_BUTTON_DOWN, down); break;
                    case SDLK_UP: NES_set_button(&nes, NES_BUTTON_UP, down); break;
                    case SDLK_LEFT: NES_set_button(&nes, NES_BUTTON_LEFT, down); break;
                    case SDLK_RIGHT: NES_set_button(&nes, NES_BUTTON_RIGHT, down); break;
                
                    case SDLK_ESCAPE: quit = true; break;
                }
            }
        }

        NES_run_frame(&nes);

        void* pixels; int pitch;

        SDL_LockTexture(texture, NULL, &pixels, &pitch);
        memcpy(pixels, nes.ppu.pixels, sizeof(nes.ppu.pixels));
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
