#include "audio.h"

#include <stdio.h>
#include <string.h>

#include <audsrv.h>
#include <loadfile.h>
#include <sifrpc.h>

#define AUDIO_CHUNK_SIZE 2048

typedef struct
{
    char riff[4];
    unsigned int size;
    char wave[4];
} WavRiffHeader;

typedef struct
{
    char id[4];
    unsigned int size;
} WavChunkHeader;

typedef struct
{
    unsigned short format;
    unsigned short channels;
    unsigned int sample_rate;
    unsigned int byte_rate;
    unsigned short block_align;
    unsigned short bits_per_sample;
} WavFormatData;

static FILE *audio_file;
static long audio_data_offset;
static int audio_ready;
static int audio_playing;
static char audio_chunk[AUDIO_CHUNK_SIZE];

static const char *audio_irx_path = "cdrom0:\\AUDSRV.IRX;1";
static const char *audio_wav_path = "cdrom0:\\PIXEL.WAV;1";

static int load_audio_modules(void)
{
    int result;

    sceSifInitRpc(0);

    result = SifLoadModule("rom0:LIBSD", 0, 0);
    if (result < 0) {
        return 0;
    }

    result = SifLoadModule(audio_irx_path, 0, 0);
    if (result < 0) {
        return 0;
    }

    return 1;
}

static int read_wav_header(FILE *file, audsrv_fmt_t *format, long *data_offset)
{
    WavRiffHeader riff;
    WavChunkHeader chunk;
    WavFormatData wav_format;
    int found_format = 0;
    int found_data = 0;

    if (fread(&riff, sizeof(riff), 1, file) != 1) {
        return 0;
    }

    if (memcmp(riff.riff, "RIFF", 4) != 0 || memcmp(riff.wave, "WAVE", 4) != 0) {
        return 0;
    }

    while (fread(&chunk, sizeof(chunk), 1, file) == 1) {
        if (memcmp(chunk.id, "fmt ", 4) == 0) {
            if (chunk.size < sizeof(wav_format)) {
                return 0;
            }

            if (fread(&wav_format, sizeof(wav_format), 1, file) != 1) {
                return 0;
            }

            if (chunk.size > sizeof(wav_format)) {
                fseek(file, (long)(chunk.size - sizeof(wav_format)), SEEK_CUR);
            }

            if (wav_format.format != 1) {
                return 0;
            }

            format->freq = (int)wav_format.sample_rate;
            format->bits = (int)wav_format.bits_per_sample;
            format->channels = (int)wav_format.channels;
            found_format = 1;
        } else if (memcmp(chunk.id, "data", 4) == 0) {
            *data_offset = ftell(file);
            fseek(file, (long)chunk.size, SEEK_CUR);
            found_data = 1;
        } else {
            fseek(file, (long)chunk.size, SEEK_CUR);
        }

        if (found_format && found_data) {
            break;
        }
    }

    if (!found_format || !found_data) {
        return 0;
    }

    if ((format->bits != 8 && format->bits != 16) ||
        (format->channels != 1 && format->channels != 2)) {
        return 0;
    }

    fseek(file, *data_offset, SEEK_SET);
    return 1;
}

int audio_init(void)
{
    audsrv_fmt_t format;

    audio_file = 0;
    audio_data_offset = 0;
    audio_ready = 0;
    audio_playing = 0;

    if (!load_audio_modules()) {
        return 0;
    }

    if (audsrv_init() != 0) {
        return 0;
    }

    audsrv_set_volume(MAX_VOLUME);

    audio_file = fopen(audio_wav_path, "rb");
    if (audio_file == 0) {
        return 0;
    }

    if (!read_wav_header(audio_file, &format, &audio_data_offset)) {
        fclose(audio_file);
        audio_file = 0;
        return 0;
    }

    if (audsrv_set_format(&format) != 0) {
        fclose(audio_file);
        audio_file = 0;
        return 0;
    }

    audio_ready = 1;
    return 1;
}

void audio_update(int enabled)
{
    int bytes_read;

    if (!audio_ready || audio_file == 0) {
        return;
    }

    if (!enabled) {
        if (audio_playing) {
            audsrv_stop_audio();
            fseek(audio_file, audio_data_offset, SEEK_SET);
            audio_playing = 0;
        }
        return;
    }

    bytes_read = (int)fread(audio_chunk, 1, sizeof(audio_chunk), audio_file);
    if (bytes_read <= 0) {
        fseek(audio_file, audio_data_offset, SEEK_SET);
        bytes_read = (int)fread(audio_chunk, 1, sizeof(audio_chunk), audio_file);
        if (bytes_read <= 0) {
            return;
        }
    }

    audsrv_wait_audio(bytes_read);
    audsrv_play_audio(audio_chunk, bytes_read);
    audio_playing = 1;
}

void audio_shutdown(void)
{
    if (audio_file != 0) {
        fclose(audio_file);
        audio_file = 0;
    }

    if (audio_ready) {
        audsrv_quit();
        audio_ready = 0;
    }
}
