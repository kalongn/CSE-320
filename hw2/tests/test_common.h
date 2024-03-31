#ifndef _TEST_COMMON_H
#define _TEST_COMMON_H

#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "audio.h"
#include "const.h"
#include "dtmf.h"

#define nelem(arr) (sizeof(arr) / sizeof(arr[0]))

struct _dtmf_event {
	uint32_t start_index;
	uint32_t end_index;
	uint8_t symbol;
};

typedef struct test_signal
{
	FILE* input;
	FILE* output;
	size_t out_size;
	char *out_content;
} TEST_SIGNAL;

static const AUDIO_HEADER const_hdr = {
	.magic_number = AUDIO_MAGIC,
	.data_offset = AUDIO_DATA_OFFSET,
	.data_size = 0,
	.encoding = PCM16_ENCODING,
	.sample_rate = AUDIO_FRAME_RATE,
	.channels = AUDIO_CHANNELS
};

/* test_common.c */
int dtmf_event_to_text(struct _dtmf_event *_events, int n, char *text);
int dtmf_event_from_text(struct _dtmf_event *_events, int maxn, FILE *textf);

void generate_dtmf_audio(struct _dtmf_event *events, int nevents, char *audio,
	size_t audiolen, char *noise, size_t noiselen,
	int level);
void validate_dtmf_audio(struct _dtmf_event *events, int nevents, char *audio,
	size_t audiolen, char *noise, size_t noiselen);

size_t generate_noise(char *buffer, int duration);
size_t generate_noise_file(const char *name, char *buffer, int duration);
int generate_step_signal(FILE *input, FILE *audio_out, uint32_t length);
int generate_signal(FILE *input, FILE *noise_in, int noise_level,
	FILE *audio_out, uint32_t length, double amplifier_constant);

/* ref_goertzel.c */
void ref_goertzel_init(GOERTZEL_STATE *gp, uint32_t N, double k);
void ref_goertzel_step(GOERTZEL_STATE *gp, double x);
double ref_goertzel_strength(GOERTZEL_STATE *gp, double x);

/* ref_audio.c */
int ref_audio_read_header(FILE *in, AUDIO_HEADER *hp);
int ref_audio_write_header(FILE *out, AUDIO_HEADER *hp);
int ref_audio_read_sample(FILE *in, int16_t *samplep);
int ref_audio_write_sample(FILE *out, int16_t sample);

#endif	// _TEST_COMMON_H
