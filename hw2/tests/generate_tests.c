#include "test_common.h"

struct _test_context {
	int duration;
	size_t expected_output_len;
	size_t input_buf_len;
	size_t input_actual_len;
	size_t nsamples;
	char *input;
	char *output;
	FILE *fin;
	FILE *fout;
};

static void setup_test(struct _test_context *ctx, struct _dtmf_event *events,
		       int n, int duration, int input_buf_len)
{
	/* Assign context values */
	ctx->duration = duration;
	ctx->expected_output_len =
	    sizeof(AUDIO_HEADER) +
	    duration * AUDIO_FRAME_RATE / 1000 * sizeof(int16_t);
	ctx->input_buf_len = input_buf_len;
	ctx->input_actual_len = 0;
	ctx->nsamples = duration * AUDIO_FRAME_RATE / 1000;

	/* Allocate input and output buffer */
	ctx->input = malloc(input_buf_len);
	cr_assert((ctx->input != NULL), "Cannot malloc input buffer");
	ctx->output = malloc(ctx->expected_output_len + 1);
	cr_assert((ctx->output != NULL), "Cannot malloc output buffer");

	/* Generate dtmf event input text */
	ctx->input_actual_len = dtmf_event_to_text(events, n, ctx->input);

	/* Open files */
	ctx->fin = fmemopen(ctx->input, ctx->input_actual_len, "r");
	cr_assert((ctx->fin != NULL), "Cannot fmemopen input buffer");
	ctx->fout = fmemopen(ctx->output, ctx->expected_output_len + 1, "wb");
	cr_assert((ctx->fout != NULL), "Cannot fmemopen output buffer");
}

static void cleanup_test(struct _test_context *ctx)
{
	fclose(ctx->fin);
	fclose(ctx->fout);
	free(ctx->input);
	free(ctx->output);
}

Test(generate_suite, basic, .timeout=10)
{
	struct _dtmf_event events[] = {{0, 1000, '0'},
				       {2000, 3000, '1'},
				       {4000, 5000, '2'},
				       {6000, 7000, '3'}};

	struct _test_context ctx;
	setup_test(&ctx, events, nelem(events), 1000, 4096);

	// Execute dtmf_generate
	int ret = dtmf_generate(ctx.fin, ctx.fout, ctx.nsamples);
	cr_assert((ret == 0),
		  "Failed to write audio file in dtmf_generate (%d)", ret);

	// fflush is required to sync back the C library's internal file buffer
	fflush(ctx.fout);

	// Validate output
	validate_dtmf_audio(events, nelem(events), ctx.output,
			    ctx.expected_output_len, NULL, 0);

	cleanup_test(&ctx);
}

Test(generate_suite, narrower_gaps, .timeout=10)
{
	struct _dtmf_event events[] = {{0, 500, '5'},	  {600, 1000, '#'},
				       {1100, 1500, 'A'}, {1600, 2000, 'D'},
				       {2100, 2500, 'C'}, {2600, 2800, '0'}};

	struct _test_context ctx;
	setup_test(&ctx, events, nelem(events), 1000, 4096);

	// Execute dtmf_generate
	int ret = dtmf_generate(ctx.fin, ctx.fout, ctx.nsamples);
	cr_assert((ret == 0),
		  "Failed to write audio file in dtmf_generate (%d)", ret);

	// fflush is required to sync back the C library's internal file buffer
	fflush(ctx.fout);

	// Validate output
	validate_dtmf_audio(events, nelem(events), ctx.output,
			    ctx.expected_output_len, NULL, 0);

	cleanup_test(&ctx);
}

Test(generate_suite, early_eof, .timeout=10)
{
	FILE *emptyf = fopen("/dev/full", "r");
	int nsamples = 8000;
	int ret;

	cr_assert((emptyf != NULL), "Unable to open /dev/full");

	ret = dtmf_generate(emptyf, stdout, nsamples);

	fclose(emptyf);
	cr_assert((ret == EOF),
		  "Expected dtmf_generate to return failure but it didn't");
}

Test(generate_suite, connecting_events, .timeout=10)
{
	struct _dtmf_event events[] = {{1000, 1500, 'A'},
				       {1500, 2000, 'B'},
				       {2000, 2500, 'C'},
				       {2500, 3000, 'D'},
				       {3000, 3500, '*'}};

	struct _test_context ctx;
	setup_test(&ctx, events, nelem(events), 500, 4096);

	int ret = dtmf_generate(ctx.fin, ctx.fout, ctx.nsamples);
	cr_assert((ret == 0),
		  "Failed to write audio file in dtmf_generate (%d)", ret);

	fflush(ctx.fout);
	validate_dtmf_audio(events, nelem(events), ctx.output,
			    ctx.expected_output_len, NULL, 0);

	cleanup_test(&ctx);
}

Test(generate_suite, invalid_events, .timeout=10)
{
	struct _dtmf_event events[] = {{1000, 500, '5'}, {1500, 2000, '6'}};

	struct _test_context ctx;
	setup_test(&ctx, events, nelem(events), 500, 4096);

	int ret = dtmf_generate(ctx.fin, ctx.fout, ctx.nsamples);
	cr_assert((ret == EOF),
		  "Expected failure on DTMF generation but returned success");

	cleanup_test(&ctx);
}

Test(generate_suite, overlapping_events, .timeout=10)
{
	struct _dtmf_event events[] = {
	    {100, 800, 'A'},
	    {1000, 2000, '5'},
	    {1500, 2500, '6'},
	    {3000, 5000, '7'},
	};

	struct _test_context ctx;
	setup_test(&ctx, events, nelem(events), 800, 4096);

	int ret = dtmf_generate(ctx.fin, ctx.fout, ctx.nsamples);
	cr_assert((ret == EOF),
		  "Expected failure on DTMF generation but returned success");

	cleanup_test(&ctx);
}

Test(generate_suite, events_out_of_range, .timeout=10)
{
	struct _dtmf_event events[] = {{100, 500, '5'},
				       {1500, 2000, '6'},
				       {3000, 5000, '7'},
				       {6000, 7000, 'A'},
				       {7200, 8800, '#'}};

	struct _test_context ctx;
	setup_test(&ctx, events, nelem(events), 950, 4096);

	int ret = dtmf_generate(ctx.fin, ctx.fout, ctx.nsamples);
	cr_assert((ret == EOF),
		  "Expected failure on DTMF generation but returned success");

	cleanup_test(&ctx);
}

Test(generate_suite, low_noise, .timeout=10)
{
	struct _dtmf_event events[] = {{100, 900, '5'},
				       {1000, 2000, '6'},
				       {2400, 3200, 'A'},
				       {3500, 4200, '#'},
				       {4400, 7200, '*'}};

	char *noise_file_name = "randnoise1.au";
	const size_t noise_data_len =
	    sizeof(AUDIO_HEADER) +
	    1000 * AUDIO_FRAME_RATE / 1000 * sizeof(int16_t);
	struct _test_context ctx;
	char noise_data[noise_data_len];

	setup_test(&ctx, events, nelem(events), 1000, 4096);
	generate_noise_file(noise_file_name, noise_data, 1000);

	noise_file = noise_file_name;
	noise_level = -10;

	int ret = dtmf_generate(ctx.fin, ctx.fout, ctx.nsamples);
	cr_assert((ret == 0),
		  "Failed to write audio file in dtmf_generate (%d)", ret);
	fflush(ctx.fout);

	validate_dtmf_audio(events, nelem(events), ctx.output,
			    ctx.expected_output_len, noise_data,
			    noise_data_len);

	unlink(noise_file_name);
	cleanup_test(&ctx);
}

Test(generate_suite, high_noise, .timeout=10)
{
	struct _dtmf_event events[] = {{600, 1200, '1'},
				       {1600, 2400, '*'},
				       {2500, 2999, 'C'},
				       {3000, 4200, 'D'},
				       {4500, 6000, '9'}};

	char *noise_file_name = "randnoise2.au";
	const size_t noise_data_len =
	    sizeof(AUDIO_HEADER) +
	    1000 * AUDIO_FRAME_RATE / 1000 * sizeof(int16_t);
	struct _test_context ctx;
	char noise_data[noise_data_len];

	setup_test(&ctx, events, nelem(events), 1000, 4096);
	generate_noise_file(noise_file_name, noise_data, 1000);

	noise_file = noise_file_name;
	noise_level = 20;

	int ret = dtmf_generate(ctx.fin, ctx.fout, ctx.nsamples);
	cr_assert((ret == 0),
		  "Failed to write audio file in dtmf_generate (%d)", ret);
	fflush(ctx.fout);

	validate_dtmf_audio(events, nelem(events), ctx.output,
			    ctx.expected_output_len, noise_data,
			    noise_data_len);

	unlink(noise_file_name);
	cleanup_test(&ctx);
}

Test(generate_suite, short_noise, .timeout=10)
{
	struct _dtmf_event events[] = {{600, 900, 'C'},
				       {1000, 2000, '*'},
				       {2400, 3000, '1'},
				       {3600, 8000, 'A'},
				       {9000, 9600, '9'}};

	char *noise_file_name = "randnoise3.au";
	const size_t noise_data_len =
	    sizeof(AUDIO_HEADER) +
	    1000 * AUDIO_FRAME_RATE / 1000 * sizeof(int16_t);
	struct _test_context ctx;
	char noise_data[noise_data_len];

	setup_test(&ctx, events, nelem(events), 1500, 4096);
	generate_noise_file(noise_file_name, noise_data, 1000);

	noise_file = noise_file_name;
	noise_level = 20;

	int ret = dtmf_generate(ctx.fin, ctx.fout, ctx.nsamples);
	cr_assert((ret == 0),
		  "Failed to write audio file in dtmf_generate (%d)", ret);
	fflush(ctx.fout);

	validate_dtmf_audio(events, nelem(events), ctx.output,
			    ctx.expected_output_len, noise_data,
			    noise_data_len);

	unlink(noise_file_name);
	cleanup_test(&ctx);
}
