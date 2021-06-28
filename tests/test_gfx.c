#include <nes.h>
#include <palette.h>

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
static uint32_t core_pixels[NES_SCREEN_HEIGHT][NES_SCREEN_WIDTH];

static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static SDL_Texture* texture = NULL;
static SDL_PixelFormat* pixel_format = NULL;


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

static void core_on_vblank(void* user)
{
    void* pixels; int pitch;

    SDL_LockTexture(texture, NULL, &pixels, &pitch);
    memcpy(pixels, nes.ppu.pixels, sizeof(nes.ppu.pixels));
    SDL_UnlockTexture(texture);
}

static void setup_palette()
{
    struct NES_Palette palette = {0};

    for (size_t i = 0; i < 64; ++i)
    {
        const struct RgbTriple rgb = NES_RGB888_PALETTE[i];

        palette.colour[i] = SDL_MapRGB(pixel_format, rgb.r, rgb.g, rgb.b);
    }

    NES_set_palette(&nes, &palette);
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

    window = SDL_CreateWindow("TotalNES", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH * scale, HEIGHT * scale, SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);

    if (!window)
    {
        exit(-1);
    }

    // this doesn't seem to work on chromebook...
    SDL_SetWindowMinimumSize(window, WIDTH, HEIGHT);

    const uint32_t pixel_format_enum = SDL_GetWindowPixelFormat(window);

    pixel_format = SDL_AllocFormat(pixel_format_enum);

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (!renderer)
    {
        exit(-1);
    }

    texture = SDL_CreateTexture(renderer, pixel_format_enum, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

    if (!texture)
    {
        exit(-1);
    }

    NES_set_pixels(&nes, core_pixels, NES_SCREEN_WIDTH, 32);
    NES_set_vblank_callback(&nes, core_on_vblank, NULL);

    setup_palette();

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
