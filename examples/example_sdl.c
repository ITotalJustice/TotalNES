// this is a small-ish example of how you would use my NES_core
// and how to write a basic "frontend".
#include <nes.h>
#include <palette.h>

#include <stdbool.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <SDL.h>


enum
{
    WIDTH = NES_SCREEN_WIDTH,
    HEIGHT = NES_SCREEN_HEIGHT,

    VOLUME = SDL_MIX_MAXVOLUME / 2,
    SAMPLES = 2048,
    SDL_AUDIO_FREQ = 96000,
};

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))


static struct NES_Core nes;

static const char* rom_path = NULL;
static uint8_t rom_data[NES_ROM_SIZE_MAX] = {0};
static size_t rom_size = 0;
static bool has_rom = false;

static int sram_fd = -1;
static uint8_t* sram_data = NULL;
static size_t sram_size = 0;

static uint8_t chr_ram[1024 * 256] = {0};

static uint32_t core_pixels[NES_SCREEN_HEIGHT][NES_SCREEN_WIDTH];

static bool running = true;
static int scale = 2;
static int speed = 1;
static int frameskip_counter = 0;

static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static SDL_Texture* texture = NULL;
static SDL_AudioDeviceID audio_device = 0;
static SDL_Rect rect = {0};
static SDL_PixelFormat* pixel_format = NULL;
static SDL_GameController* game_controller = NULL;


static int get_scale(int w, int h)
{
    const int scale_w = w / WIDTH;
    const int scale_h = h / HEIGHT;

    return scale_w < scale_h ? scale_w : scale_h;
}

static void run()
{
    for (int i = 0; i < speed; ++i)
    {
        NES_run_frame(&nes);
    }
}

static bool is_fullscreen()
{
    const int flags = SDL_GetWindowFlags(window);

    // check if we are already in fullscreen mode
    if (flags & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP))
    {
        return true;
    }
    else
    {
        return false;
    }
}

static void setup_rect(int w, int h)
{
    const int min_scale = get_scale(w, h);

    rect.w = WIDTH * min_scale;
    rect.h = HEIGHT * min_scale;
    rect.x = (w - rect.w);
    rect.y = (h - rect.h);

    // don't divide by zero!
    if (rect.x > 0) rect.x /= 2;
    if (rect.y > 0) rect.y /= 2;
}

static void scale_screen()
{
    SDL_SetWindowSize(window, WIDTH * scale, HEIGHT * scale);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

static void toggle_fullscreen()
{
    // check if we are already in fullscreen mode
    if (is_fullscreen())
    {
        SDL_SetWindowFullscreen(window, 0);
    }
    else
    {
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }
}

static void on_ctrl_key_event(const SDL_KeyboardEvent* e, bool down)
{
    if (down)
    {
        switch (e->keysym.scancode)
        {
            case SDL_SCANCODE_EQUALS:
            case SDL_SCANCODE_KP_PLUS:
                ++scale;
                scale_screen();
                break;

            case SDL_SCANCODE_MINUS:
            case SDL_SCANCODE_KP_PLUSMINUS:
            case SDL_SCANCODE_KP_MINUS:
                scale = scale > 0 ? scale - 1 : 1;
                scale_screen();
                break;

            case SDL_SCANCODE_1:
            case SDL_SCANCODE_2:
            case SDL_SCANCODE_3:
            case SDL_SCANCODE_4:
            case SDL_SCANCODE_5:
            case SDL_SCANCODE_6:
            case SDL_SCANCODE_7:
            case SDL_SCANCODE_8:
            case SDL_SCANCODE_9:
                speed = (e->keysym.scancode - SDL_SCANCODE_1) + 1;
                break;

            case SDL_SCANCODE_F:
                toggle_fullscreen();
                break;

            default: break; // silence enum warning
        }
    }
}

static void on_key_event(const SDL_KeyboardEvent* e)
{
    const bool down = e->type == SDL_KEYDOWN;
    const bool ctrl = (e->keysym.mod & KMOD_CTRL) > 0;

    if (ctrl)
    {
        on_ctrl_key_event(e, down);

        return;
    }

    if (!has_rom)
    {
        return;
    }

    switch (e->keysym.scancode)
    {
        case SDL_SCANCODE_X:        NES_set_button(&nes, NES_BUTTON_A, down);       break;
        case SDL_SCANCODE_Z:        NES_set_button(&nes, NES_BUTTON_B, down);       break;
        case SDL_SCANCODE_UP:       NES_set_button(&nes, NES_BUTTON_UP, down);      break;
        case SDL_SCANCODE_DOWN:     NES_set_button(&nes, NES_BUTTON_DOWN, down);    break;
        case SDL_SCANCODE_LEFT:     NES_set_button(&nes, NES_BUTTON_LEFT, down);    break;
        case SDL_SCANCODE_RIGHT:    NES_set_button(&nes, NES_BUTTON_RIGHT, down);   break;
        case SDL_SCANCODE_RETURN:   NES_set_button(&nes, NES_BUTTON_START, down);   break;
        case SDL_SCANCODE_SPACE:    NES_set_button(&nes, NES_BUTTON_SELECT, down);  break;
    
        case SDL_SCANCODE_ESCAPE:
            running = false;
            break;

        default: break; // silence enum warning
    }
}

static void on_controller_axis_event(const SDL_ControllerAxisEvent* e)
{
    enum
    {
        deadzone = 8000,
        left     = -deadzone,
        right    = +deadzone,
        up       = -deadzone,
        down     = +deadzone,
    };

    switch (e->axis)
    {
        case SDL_CONTROLLER_AXIS_LEFTX: case SDL_CONTROLLER_AXIS_RIGHTX:
            if (e->value < left)
            {
                NES_set_button(&nes, NES_BUTTON_LEFT, true);
                NES_set_button(&nes, NES_BUTTON_RIGHT, false);
            }
            else if (e->value > right)
            {
                NES_set_button(&nes, NES_BUTTON_LEFT, false);
                NES_set_button(&nes, NES_BUTTON_RIGHT, true);
            }
            else
            {
                NES_set_button(&nes, NES_BUTTON_LEFT, false);
                NES_set_button(&nes, NES_BUTTON_RIGHT, false);
            }
            break;

        case SDL_CONTROLLER_AXIS_LEFTY: case SDL_CONTROLLER_AXIS_RIGHTY:
            if (e->value < up)
            {
                NES_set_button(&nes, NES_BUTTON_UP, true);
                NES_set_button(&nes, NES_BUTTON_DOWN, false);
            }
            else if (e->value > down)
            {
                NES_set_button(&nes, NES_BUTTON_UP, false);
                NES_set_button(&nes, NES_BUTTON_DOWN, true);
            }
            else
            {
                NES_set_button(&nes, NES_BUTTON_UP, false);
                NES_set_button(&nes, NES_BUTTON_DOWN, false);
            }
            break;
    }
}

static void on_controller_event(const SDL_ControllerButtonEvent* e)
{
    const bool down = e->type == SDL_CONTROLLERBUTTONDOWN;

    switch (e->button)
    {
        case SDL_CONTROLLER_BUTTON_A:               NES_set_button(&nes, NES_BUTTON_A, down);       break;
        case SDL_CONTROLLER_BUTTON_B:               NES_set_button(&nes, NES_BUTTON_B, down);       break;
        case SDL_CONTROLLER_BUTTON_X:               break;
        case SDL_CONTROLLER_BUTTON_Y:               break;
        case SDL_CONTROLLER_BUTTON_START:           NES_set_button(&nes, NES_BUTTON_START, down);   break;
        case SDL_CONTROLLER_BUTTON_BACK:            NES_set_button(&nes, NES_BUTTON_SELECT, down);  break;
        case SDL_CONTROLLER_BUTTON_GUIDE:           break;
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:    break;
        case SDL_CONTROLLER_BUTTON_LEFTSTICK:       break;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:   break;
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK:      break;
        case SDL_CONTROLLER_BUTTON_DPAD_UP:         NES_set_button(&nes, NES_BUTTON_UP, down);      break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:       NES_set_button(&nes, NES_BUTTON_DOWN, down);    break;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:       NES_set_button(&nes, NES_BUTTON_LEFT, down);    break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:      NES_set_button(&nes, NES_BUTTON_RIGHT, down);   break;
    }
}

static void on_controller_device_event(const SDL_ControllerDeviceEvent* e)
{
    switch (e->type)
    {
        case SDL_CONTROLLERDEVICEADDED:
            if (game_controller)
            {
                SDL_GameControllerClose(game_controller);
            }
            game_controller = SDL_GameControllerOpen(e->which);
            break;

        case SDL_CONTROLLERDEVICEREMOVED:
            break;
    }
}

static void on_window_event(const SDL_WindowEvent* e)
{
    switch (e->event)
    {
        case SDL_WINDOWEVENT_SIZE_CHANGED:
        {
            // use this rather than window size because iirc i had issues with
            // hi-dpi screens.
            int w = 0, h = 0;
            SDL_GetRendererOutputSize(renderer, &w, &h);

            setup_rect(w, h);
        }   break;
    }
}

static void events()
{
    SDL_Event e;

    while (SDL_PollEvent(&e))
    {
        switch (e.type)
        {
            case SDL_QUIT:
                running = false;
                return;
        
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                on_key_event(&e.key);
                break;

            case SDL_CONTROLLERBUTTONDOWN:
            case SDL_CONTROLLERBUTTONUP:
                on_controller_event(&e.cbutton);
                break;

            case SDL_CONTROLLERDEVICEADDED:
            case SDL_CONTROLLERDEVICEREMOVED:
                on_controller_device_event(&e.cdevice);
                break;
            
            case SDL_CONTROLLERAXISMOTION:
                on_controller_axis_event(&e.caxis);
                break;

            case SDL_WINDOWEVENT:
                on_window_event(&e.window);
                break;
        }
    } 
}

static void core_on_apu(void* user, struct NES_ApuCallbackData* data)
{
    (void)user;

    // using buffers because pushing 1 sample at a time seems to
    // cause popping sounds (on my chromebook).
    static int8_t buffer[SAMPLES] = {0};

    static size_t buffer_count = 0;

    // if speedup is enabled, skip x many samples in order to not fill the audio buffer!
    if (speed > 1)
    {
        static int skipped_samples = 0;

        if (skipped_samples < speed - 1)
        {
            ++skipped_samples;
            return;
        }     

        skipped_samples = 0;   
    }

    buffer[buffer_count++] = data->square1 + data->square2 + data->triangle + data->noise;

    if (buffer_count == sizeof(buffer))
    {
        buffer_count = 0;

        uint8_t samples[sizeof(buffer)] = {0};

        SDL_MixAudioFormat(samples, (const uint8_t*)buffer, AUDIO_S8, sizeof(buffer), VOLUME);

        while (SDL_GetQueuedAudioSize(audio_device) > (sizeof(buffer) * 4))
        {
            SDL_Delay(8);
        }

        SDL_QueueAudio(audio_device, samples, sizeof(samples));
    }
}

static void core_on_vblank(void* user)
{
    (void)user;

    ++frameskip_counter;

    if (frameskip_counter >= speed)
    {
        void* pixels; int pitch;

        SDL_LockTexture(texture, NULL, &pixels, &pitch);
        memcpy(pixels, core_pixels, sizeof(core_pixels));
        SDL_UnlockTexture(texture);

        frameskip_counter = 0;
    }
}

static void render()
{
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_RenderPresent(renderer);
}

static void cleanup()
{
    if (sram_data)      { munmap(sram_data, sram_size); }
    if (sram_fd != -1)  { close(sram_fd); }
    if (pixel_format)   { SDL_free(pixel_format); }
    if (audio_device)   { SDL_CloseAudioDevice(audio_device); }
    if (texture)        { SDL_DestroyTexture(texture); }
    if (renderer)       { SDL_DestroyRenderer(renderer); }
    if (window)         { SDL_DestroyWindow(window); }

    SDL_Quit();
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
    if (!NES_init(&nes))
    {
        goto fail;
    }

    if (argc < 2)
    {
        goto fail;
    }

    rom_path = argv[1];

    FILE* f = fopen(rom_path, "rb");

    if (!f)
    {
        goto fail;
    }

    rom_size = fread(rom_data, 1, sizeof(rom_data), f);

    fclose(f);

    if (!rom_size)
    {
        goto fail;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER))
    {
        goto fail;
    }

    if (SDL_GameControllerAddMappingsFromFile("res/controller_mapping/gamecontrollerdb.txt"))
    {
        printf("failed to open controllerdb file! %s\n", SDL_GetError());
    }

    window = SDL_CreateWindow("TotalNES", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH * scale, HEIGHT * scale, SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);

    if (!window)
    {
        goto fail;
    }

    // this doesn't seem to work on chromebook...
    SDL_SetWindowMinimumSize(window, WIDTH, HEIGHT);

    const uint32_t pixel_format_enum = SDL_GetWindowPixelFormat(window);

    pixel_format = SDL_AllocFormat(pixel_format_enum);

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (!renderer)
    {
        goto fail;
    }

    texture = SDL_CreateTexture(renderer, pixel_format_enum, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

    if (!texture)
    {
        goto fail;
    }

    {
        int w = 0, h = 0;
        SDL_GetRendererOutputSize(renderer, &w, &h);

        setup_rect(w, h);
    }

    const SDL_AudioSpec wanted =
    {
        .freq = SDL_AUDIO_FREQ,
        .format = AUDIO_S8,
        .channels = 1,
        .silence = 0,
        .samples = SAMPLES,
        .padding = 0,
        .size = 0,
        .callback = NULL,
        .userdata = NULL,
    };

    SDL_AudioSpec aspec_got = {0};

    audio_device = SDL_OpenAudioDevice(NULL, 0, &wanted, &aspec_got, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);

    if (audio_device == 0)
    {
        goto fail;
    }

    printf("[SDL-AUDIO] freq: %d\n", aspec_got.freq);
    printf("[SDL-AUDIO] channels: %d\n", aspec_got.channels);
    printf("[SDL-AUDIO] samples: %d\n", aspec_got.samples);
    printf("[SDL-AUDIO] size: %d\n", aspec_got.size);

    SDL_PauseAudioDevice(audio_device, 0);

    setup_palette();

    NES_set_chr_ram(&nes, chr_ram, sizeof(chr_ram));
    NES_set_pixels(&nes, core_pixels, NES_SCREEN_WIDTH, 32);
    NES_set_apu_callback(&nes, core_on_apu, NULL, aspec_got.freq + 512);
    NES_set_vblank_callback(&nes, core_on_vblank, NULL);

    struct NES_RomInfo rom_info = {0};

    if (!NES_get_rom_info(rom_data, rom_size, &rom_info))
    {
        goto fail;
    }

    if (rom_info.prg_ram_size > 0)
    {
        int flags = 0;

        if (rom_info.prg_battery)
        {
            char sram_path[0x304] = {0};

            const char* ext = strrchr(argv[1], '.');

            if (!ext)
            {
                goto fail;
            }

            strncat(sram_path, argv[1], ext - argv[1]);
            strcat(sram_path, ".sav");

            flags = MAP_SHARED;

            sram_fd = open(sram_path, O_RDWR | O_CREAT, 0644);

            if (sram_fd == -1)
            {
                perror("failed to open sram");
                goto fail;
            }

            struct stat s = {0};

            if (fstat(sram_fd, &s) == -1)
            {
                perror("failed to stat sram");
                goto fail;
            }

            if (s.st_size < rom_info.prg_ram_size)
            {
                char page[1024] = {0};
                
                for (size_t i = 0; i < rom_info.prg_ram_size; i += sizeof(page))
                {
                    int size = sizeof(page) > rom_info.prg_ram_size-i ? rom_info.prg_ram_size-i : sizeof(page);
                    write(sram_fd, page, size);
                }
            }  
        }
        else
        {
            flags = MAP_PRIVATE | MAP_ANONYMOUS;
        }

        sram_data = (uint8_t*)mmap(NULL, rom_info.prg_ram_size, PROT_READ | PROT_WRITE, flags, sram_fd, 0);
    
        if (sram_data == MAP_FAILED)
        {
            perror("failed to mmap sram");
            goto fail;
        }

        sram_size = rom_info.prg_ram_size;

        NES_set_prg_ram(&nes, sram_data, rom_info.prg_ram_size);
    }

    if (!NES_loadrom(&nes, rom_data, rom_size))
    {
        printf("failed to loadrom\n");
        goto fail;
    }

    has_rom = true;

    while (running)
    {
        events();
        run();
        render();
    }

    cleanup();

    return 0;

fail:
    printf("fail %s\n", SDL_GetError());
    cleanup();

    return -1;
}
