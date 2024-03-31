#include "test_common.h"
#include "debug.h"

static void write_header(char *audiobuf, size_t audio_len);
static void write_sample(char *audiobuf, int16_t sample);
static double combine_with_noise(double signal, int16_t noise_sample, int level);
static int16_t ref_combine_noise(int16_t dtmf, int16_t noise, int noise_level, bool has_noise);
static void get_freqs_of_symbol(uint8_t symbol, int *rowfreq, int *colfreq);
static unsigned int ref_symbol_to_freqs(uint8_t symbol);
static void int16_big2little(int16_t *value);
static uint32_t uint32_little2big(uint32_t val);
static int16_t ref_sample(unsigned int freqs, uint32_t index, double amplifier_constant);

int dtmf_event_to_text(struct _dtmf_event *_events, int n, char *text)
{
	char *textptr = text;
	int linelen, total_len = 0;
	/* {UINT32_MAX}\t{UINT32_MAX}\t{CHAR}\n */
	const int line_max_size = 10 + 1 + 10 + 1 + 1 + 1;
	for (int i = 0; i < n; ++i) {
		linelen = snprintf(textptr, line_max_size, "%u\t%u\t%c\n",
				   _events[i].start_index, _events[i].end_index,
				   _events[i].symbol);
		textptr += linelen;
		total_len += linelen;
	}
	*textptr = '\0';
	return total_len;
}

int dtmf_event_from_text(struct _dtmf_event *_events, int maxn, FILE *textf)
{
	int count = 0;
	int result;
	char line[512] = {0};

	rewind(textf);
	while (fgets(line, 512, textf) && count < maxn) {
		int start_index, end_index;
		char symbol;
		result = sscanf(line, "%d\t%d\t%c", &start_index, &end_index,
				&symbol);
		if (result < 3) break;
		_events[count].start_index = start_index;
		_events[count].end_index = end_index;
		_events[count].symbol = symbol;
		count++;
	}
	return count;
}

void generate_dtmf_audio(struct _dtmf_event *events, int nevents, char *audio,
			 size_t audiolen, char *noise, size_t noiselen,
			 int level)
{
	static struct _dtmf_event empty_event = {0, 0, 0};
	struct _dtmf_event *cur_event = events;
	double received, expected;
	size_t offset = sizeof(AUDIO_HEADER);
	size_t noise_offset = sizeof(AUDIO_HEADER);
	int16_t *sample = (int16_t *)(audio + offset);
	int16_t *noise_sample_ptr = NULL;
	// i - index of sample; e - index of event
	int i, e;

	if (noise) {
		noise_sample_ptr = (int16_t *)(noise + offset);
	}
	if (nevents == 0) {
		cur_event = &empty_event;
	}

	/* Write header to audio buffer */
	write_header(audio, audiolen);

	for (e = 0, i = 0; offset < audiolen;
	     ++sample, offset += sizeof(int16_t), ++i) {
		/* If sample index exceeds the event's end index,
		 * try going to the next event */
		if (i >= cur_event->end_index && e < nevents - 1) {
			cur_event++;
			e++;
		}

		/* If the index of current sample is not in the range of
		 * the current event, assume it's 0. */
		if (i < cur_event->start_index || i >= cur_event->end_index) {
			expected = 0.0;
		} else {
			/* Otherwise compute expected sample value */
			int rowfreq, colfreq;
			get_freqs_of_symbol(cur_event->symbol, &rowfreq,
					    &colfreq);
			double a = 0.5 * cos(2.0 * M_PI * rowfreq * i /
					     AUDIO_FRAME_RATE);
			double b = 0.5 * cos(2.0 * M_PI * colfreq * i /
					     AUDIO_FRAME_RATE);
			expected = a + b;
		}
		/* If there is noise supplied, combine expected signal with
		 * noise */
		if (noise_sample_ptr) {
			int16_t noise_sample = 0;
			if (noise_offset < noiselen)
				noise_sample = *noise_sample_ptr;
			int16_big2little(&noise_sample);
			expected =
			    combine_with_noise(expected, noise_sample, level);
			if (noise_offset < noiselen) {
				noise_offset += sizeof(int16_t);
				noise_sample_ptr++;
			}
		}

		/* Write expected output to audio data buffer */
		int16_t output = INT16_MAX * expected;
		/* NOTE: make sure to shift endianess! */
		int16_big2little(&output);
		*sample = output;
	}
}

size_t generate_noise(char *buffer, int duration)
{
        srand(1);  // Always generate the same data.
	size_t audio_len =
	    sizeof(AUDIO_HEADER) + AUDIO_CHANNELS * sizeof(int16_t) * duration *
				       AUDIO_FRAME_RATE / 1000;
	char *bufptr = buffer;
	size_t remaining = audio_len;
	int ret;
	write_header(bufptr, audio_len);
	bufptr += sizeof(AUDIO_HEADER);
	remaining -= sizeof(AUDIO_HEADER);
	while (remaining-- > 0) {
	        char c = rand() % 0xff;
		*bufptr++ = c;
	}
	return audio_len;
}

size_t generate_noise_file(const char *name, char *buffer, int duration)
{
	FILE *fp = fopen(name, "w");
        srand(1);  // Always generate the same data.
	size_t audio_len = AUDIO_CHANNELS * sizeof(int16_t) * duration *
			   AUDIO_FRAME_RATE / 1000;
	AUDIO_HEADER header = {
	    .magic_number = uint32_little2big(AUDIO_MAGIC),
	    .data_offset = uint32_little2big(AUDIO_DATA_OFFSET),
	    .data_size = uint32_little2big(audio_len),
	    .encoding = uint32_little2big(PCM16_ENCODING),
	    .sample_rate = uint32_little2big(AUDIO_FRAME_RATE),
	    .channels = uint32_little2big(AUDIO_CHANNELS)};
	char *bufptr = buffer;
	size_t remaining = audio_len;
	int ret;

	if(fp == NULL)
	    cr_skip_test("Cannot open noise file %s (%d)", name, errno);

	memcpy(bufptr, &header, sizeof(header));
	bufptr += sizeof(header);
	ret = fwrite(&header, 1, sizeof(header), fp);
	cr_assert((ret == sizeof(header)), "Error writing header to %s", name);
	while (remaining-- > 0) {
	        char c = rand() % 0xff;
                fputc(c, fp);
		*bufptr++ = c;
	}
	if(fclose(fp) == EOF)
	    cr_skip_test("Error writing noise file '%s'", name);
	return sizeof(header) + audio_len;
}

/*
 * Generate a "step" signal composed of a sequence of intervals on each of which
 * the value of the signal is constant.  The shape of the signal is specifed by
 * a series of 3-tuples: start_index/end_index/sample_value read from an input file.
 */
int generate_step_signal(FILE *input, FILE *audio_out, uint32_t length) {
	uint32_t pos = 0;
	uint32_t start_index = 0;
	uint32_t end_index = 0;
	uint16_t sample_value = 0;

	AUDIO_HEADER hdr = const_hdr;
	hdr.data_size = length * AUDIO_CHANNELS * AUDIO_BYTES_PER_SAMPLE;
	if(ref_audio_write_header(audio_out, &hdr) == EOF) {
		fprintf(stderr, "Error writing audio header\n");
		return EOF;
	}
	while(1) {
		int ret_val = fscanf(input, "%u%u%hu", &start_index, &end_index, &sample_value);
		if(ret_val != 3 || ret_val == EOF)
			break;
		// fprintf(stdout,"%-6u|%-6u|%-6hu|\n", start_index, end_index, sample_value);
		if(start_index < pos || end_index > length)
			return EOF;
		while(pos < start_index) {
			if(ref_audio_write_sample(audio_out, 0) == EOF)
				return EOF;
			pos++;
		}
		while(pos < end_index) {
			if(ref_audio_write_sample(audio_out, sample_value) == EOF)
				return EOF;
			pos++;
		}
	}
	while(pos < length) {
		if(ref_audio_write_sample(audio_out, 0) == EOF)
			return EOF;
		pos++;
	}

	return 0;
}

int generate_signal(FILE *input, FILE *noise_in, int noise_level,
	FILE *audio_out, uint32_t length, double amplifier_constant) {
	uint32_t pos = 0;
	int16_t sample = 0;
	int16_t noise = 0;
	uint32_t start_index = 0;
	uint32_t end_index = 0;
	uint8_t empty;
	uint8_t symbol;

	if(noise_in != NULL) {
		AUDIO_HEADER nhdr;
		if(ref_audio_read_header(noise_in, &nhdr) == EOF) {
			fprintf(stderr, "Error reading noise file header\n");
			return EOF;
		}
	}

	AUDIO_HEADER hdr = const_hdr;
	hdr.data_size = length * AUDIO_CHANNELS * AUDIO_BYTES_PER_SAMPLE;
	if(ref_audio_write_header(audio_out, &hdr) == EOF) {
		fprintf(stderr, "Error writing audio header\n");
		return EOF;
	}

	int ret_val = -1;
	while(1) {
		ret_val = fscanf(input, "%u%u%c%c",
			&start_index, &end_index, &empty, &symbol);
		if(ret_val != 4 || ret_val == EOF)
			break;
		unsigned int freqs = ref_symbol_to_freqs(symbol);
		if(start_index < pos || end_index > length)
			return EOF;

		// Output either samples from the noise file or sample values of 0
		// until start of event has been reached.
		while(pos < start_index) {
			if(noise_in != NULL &&
				ref_audio_read_sample(noise_in, &noise) == EOF)
				return EOF;
			sample = ref_combine_noise(0, noise, noise_level, noise_in != NULL);
			if(ref_audio_write_sample(audio_out, sample) == EOF)
				return EOF;
			pos++;
		}

		// Output synthesized DTMF samples combined with noise samples
		// until end of event has been reached.
		while(pos < end_index) {
			noise = 0;
			if(noise_in != NULL &&
				ref_audio_read_sample(noise_in, &noise) == EOF)
				return EOF;
			//default amplifier constant = 1.0
			sample = ref_combine_noise(ref_sample(freqs, pos,
				amplifier_constant), noise, noise_level, noise_in != NULL);
			if(ref_audio_write_sample(audio_out, sample) == EOF)
				return EOF;
			pos++;
		}
	}
    // Output noise or sample values of 0 until specified length has been reached.
	while(pos < length) {
		noise = 0;
		if(noise_in != NULL &&
			ref_audio_read_sample(noise_in, &noise) == EOF)
			sample = 0;
		else
			sample = ref_combine_noise(0, noise, noise_level, noise_in != NULL);
		if(ref_audio_write_sample(audio_out, sample) == EOF)
			return EOF;
		pos++;
	}

	return 0;
}

void validate_dtmf_audio(struct _dtmf_event *events, int nevents, char *audio,
			 size_t audiolen, char *noise, size_t noiselen)
{
	static struct _dtmf_event empty_event = {0, 0, 0};
	/* Use a looser epsilon value: because the conversion of
	 * double (target's dtmf_generate) -> int16_t (target's write_sample)
	 * -> double (converts the -32768~32767 ranged sample back into double)
	 * will lose some precision
	 */
	const double my_epsilon = 1e-4;
	struct _dtmf_event *cur_event = events;
	double received, expected;
	size_t offset = sizeof(AUDIO_HEADER);
	size_t noise_offset = sizeof(AUDIO_HEADER);
	int16_t *sample = (int16_t *)(audio + offset);
	int16_t *noise_sample_ptr = NULL;
	// i - index of sample; e - index of event
	int i, e;

	if (noise) {
		noise_sample_ptr = (int16_t *)(noise + offset);
	}
	if (nevents == 0) {
		cur_event = &empty_event;
	}

	for (e = 0, i = 0; offset < audiolen;
	     ++sample, offset += sizeof(int16_t), ++i) {
		/* NOTE: Remember to shift endianess! */
		int16_big2little(sample);

		/* If sample index exceeds the event's end index,
		 * try going to the next event */
		if (i >= cur_event->end_index && e < nevents - 1) {
			cur_event++;
			e++;
		}

		/* If the index of current sample is not in the range of
		 * the current event, assume it's 0. */
		if (i < cur_event->start_index || i >= cur_event->end_index) {
			expected = 0.0;
		} else {
			/* Otherwise compute expected sample value */
			int rowfreq, colfreq;
			get_freqs_of_symbol(cur_event->symbol, &rowfreq,
					    &colfreq);
			double a = 0.5 * cos(2.0 * M_PI * rowfreq * i /
					     AUDIO_FRAME_RATE);
			double b = 0.5 * cos(2.0 * M_PI * colfreq * i /
					     AUDIO_FRAME_RATE);
			expected = a + b;
		}
		/* If there is noise supplied, combine expected signal with
		 * noise */
		if (noise_sample_ptr) {
			int16_t noise_sample = 0;
			if (noise_offset < noiselen)
				noise_sample = *noise_sample_ptr;
			int16_big2little(&noise_sample);
			expected = combine_with_noise(expected, noise_sample,
						      noise_level);
			if (noise_offset < noiselen) {
				noise_offset += sizeof(int16_t);
				noise_sample_ptr++;
			}
		}
		/* Compare */
		received = 1.0 * (*sample) / INT16_MAX;
		cr_assert(
		    (fabs(received - expected) < my_epsilon),
		    "Output audio sample is wrong, expected %.6f, got %.6f, "
		    "audio data offset = %zu, current event[%d] = {%d, %d, "
		    "%c}, "
		    "sample in hex = %#x, sample index = %d",
		    expected, received, offset, e, cur_event->start_index,
		    cur_event->end_index, cur_event->symbol, *sample, i);
	}
}

static void get_freqs_of_symbol(uint8_t symbol, int *rowfreq, int *colfreq)
{
	for (int row = 0; row < 4; ++row) {
		for (int col = 0; col < 4; ++col) {
			if (dtmf_symbol_names[row][col] == symbol) {
				*rowfreq = dtmf_freqs[row];
				*colfreq = dtmf_freqs[col + 4];
				return;
			}
		}
	}
	*rowfreq = 0;
	*colfreq = 0;
}

static void int16_big2little(int16_t *value)
{
	char *bytes = (char *)value;
	char tmp = bytes[0];
	bytes[0] = bytes[1];
	bytes[1] = tmp;
}

static uint32_t uint32_little2big(uint32_t val)
{
	uint32_t a, b, c, d;
	a = (val >> 24) & 0x000000ff;
	b = (val >> 8) & 0x0000ff00;
	c = (val << 8) & 0x00ff0000;
	d = (val << 24) & 0xff000000;
	return a | b | c | d;
}

static void write_header(char *audiobuf, size_t audio_len)
{
	AUDIO_HEADER header = {
	    .magic_number = uint32_little2big(AUDIO_MAGIC),
	    .data_offset = uint32_little2big(AUDIO_DATA_OFFSET),
	    .data_size = uint32_little2big(audio_len - sizeof(AUDIO_HEADER)),
	    .encoding = uint32_little2big(PCM16_ENCODING),
	    .sample_rate = uint32_little2big(AUDIO_FRAME_RATE),
	    .channels = uint32_little2big(AUDIO_CHANNELS)};
	memcpy(audiobuf, &header, sizeof(header));
}

static void write_sample(char *audiobuf, int16_t sample)
{
	int16_t *outp = (int16_t *)audiobuf;
	int16_big2little(&sample);
	*outp = sample;
}

static double combine_with_noise(double signal, int16_t noise_sample, int level)
{
	double noise = 1.0 * noise_sample / INT16_MAX;
	double p = pow(10, 1.0 * level / 10);
	double noise_ratio = p / (1.0 + p);
	double signal_ratio = 1.0 / (1.0 + p);
	return signal * signal_ratio + noise * noise_ratio;
}

static int16_t ref_sample(unsigned int freqs, uint32_t index,
	double amplifier_constant) {
	double v = 0.0;
	int16_t sample;
	for(int f = 0; f < NUM_DTMF_FREQS; f++) {
		if(freqs & (0x1 << f)) {
			v += (amplifier_constant *
				cos(2.0 * M_PI * dtmf_freqs[f] * index / AUDIO_FRAME_RATE));
		}
	}

	sample = (int16_t)((v / (2 * amplifier_constant)) * INT16_MAX);
	return sample;
}

static unsigned int ref_symbol_to_freqs(uint8_t symbol) {
	unsigned int freqs = 0x0;
	switch(symbol) {
		case '1': case '2': case '3': case 'A':
		freqs |= 0x1;
		break;
		case '4': case '5': case '6': case 'B':
		freqs |= 0x2;
		break;
		case '7': case '8': case '9': case 'C':
		freqs |= 0x4;
		break;
		case '*': case '0': case '#': case 'D':
		freqs |= 0x8;
		break;
		default:
		return 0x0;
	}
	switch(symbol) {
		case '1': case '4': case '7': case '*':
		freqs |= 0x10;
		break;
		case '2': case '5': case '8': case '0':
		freqs |= 0x20;
		break;
		case '3': case '6': case '9': case '#':
		freqs |= 0x40;
		break;
		case 'A': case 'B': case 'C': case 'D':
		freqs |= 0x80;
		break;
		default:
		return 0x0;
	}
	return freqs;
}

static int16_t ref_combine_noise(int16_t dtmf, int16_t noise, int noise_level,
	bool has_noise) {
	double ddtmf = (double)dtmf / INT16_MAX;
	double dnoise = (double)noise / INT16_MAX;

	if (has_noise) {
		double p = pow(10, (double)noise_level/10);
		double nf = p / (1 + p);
		double df = 1 / (1 + p);
		return (int16_t)((nf * dnoise + df * ddtmf) * INT16_MAX);
	} else {
		return dtmf;
	}
}

