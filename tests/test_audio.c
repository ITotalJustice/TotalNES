#include <nes.h>
#include <stdio.h>
#include <SDL.h>


enum
{
    SAMPLES = 1024,
    VOLUME = SDL_MIX_MAXVOLUME / 2,
    SDL_AUDIO_FREQ = 96000,
};


static struct NES_Core nes = {0};
static uint8_t ROM[NES_ROM_SIZE_MAX] = {0};

static SDL_AudioDeviceID audio_device = 0;


static bool read_file(const char* path, uint8_t* out_buf, size_t* out_size)
{
    FILE* f = fopen(path, "rb");
    if (!f)
    {
        return false;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size <= 0)
    {
        return false;
    }

    fread(out_buf, 1, size, f);
    *out_size = (size_t)size;
    fclose(f);

    return true;
}

static void core_on_apu(void* user, struct NES_ApuCallbackData* data)
{
    (void)user;
    
    // using buffers because pushing 1 sample at a time seems to
    // cause popping sounds (on my chromebook).
    static int8_t buffer[SAMPLES] = {0};

    static size_t buffer_count = 0;

    buffer[buffer_count++] = data->square1 + data->square2 + data->triangle + data->noise;

    if (buffer_count == sizeof(buffer))
    {
        buffer_count = 0;

        uint8_t samples[sizeof(buffer)] = {0};

        SDL_MixAudioFormat(samples, (const uint8_t*)buffer, AUDIO_S8, sizeof(buffer), VOLUME);

        while (SDL_GetQueuedAudioSize(audio_device) > (sizeof(buffer) * 4))
        {
            SDL_Delay(4);
        }

        SDL_QueueAudio(audio_device, samples, sizeof(samples));
    }
}

static void cleanup()
{
    if (audio_device) { SDL_CloseAudioDevice(audio_device); }

    SDL_Quit();
}

int main(int argc, char const *argv[])
{
    if (argc < 2)
    {
        printf("missing args\n");
        goto fail;
    }

    size_t rom_size = 0;

    if (!read_file(argv[1], ROM, &rom_size))
    {
        printf("failed to read file %s\n", argv[1]);
        goto fail;
    }

    // enable to record audio
    #if 0
        SDL_setenv("SDL_AUDIODRIVER", "disk", 1);
    #endif

    if (SDL_Init(SDL_INIT_AUDIO))
    {
        goto fail;
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

    SDL_PauseAudioDevice(audio_device, 0);

    printf("[SDL-AUDIO] freq: %d\n", aspec_got.freq);
    printf("[SDL-AUDIO] channels: %d\n", aspec_got.channels);
    printf("[SDL-AUDIO] samples: %d\n", aspec_got.samples);
    printf("[SDL-AUDIO] size: %d\n", aspec_got.size);
    
    if (!NES_init(&nes))
    {
        goto fail;
    }

    if (!NES_loadrom(&nes, ROM, rom_size))
    {
        printf("failed to load rom %s\n", argv[1]);
        goto fail;
    }

    NES_set_apu_callback(&nes, core_on_apu, NULL, aspec_got.freq);

    for (;;)
    {
        NES_run_frame(&nes);
    }

    cleanup();
    return 0;

fail:
    cleanup();
    return -1;
}
