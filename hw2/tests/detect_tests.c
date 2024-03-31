#include "test_common.h"

struct _test_context {
	int duration;
	bool has_noise;
	size_t input_buf_len;
	size_t output_buf_len;
	size_t nsamples;
	char *input;
	char *output;
	char *noise;
	FILE *fin;
	FILE *fout;
};

static bool compare_dtmf_event_arrays(struct _dtmf_event *e1, int n1,
				      struct _dtmf_event *e2, int n2)
{
	if (n1 != n2) return false;
	for (int i = 0; i < n1; ++i) {
		if (e1[i].start_index != e2[i].start_index) return false;
		if (e1[i].end_index != e2[i].end_index) return false;
		if (e1[i].symbol != e2[i].symbol) return false;
	}
	return true;
}

static void setup_test(struct _test_context *ctx, struct _dtmf_event *events,
		       int n, int duration, int output_buf_len, bool has_noise,
		       int level)
{
	/* Assign context values */
	ctx->duration = duration;
	ctx->input_buf_len = sizeof(AUDIO_HEADER) + duration *
							AUDIO_FRAME_RATE /
							1000 * sizeof(int16_t);
	ctx->output_buf_len = output_buf_len;
	ctx->nsamples = duration * AUDIO_FRAME_RATE / 1000;
	ctx->has_noise = has_noise;
	ctx->noise = NULL;

	/* Allocate input and output buffer */
	ctx->input = malloc(ctx->input_buf_len);
	cr_assert((ctx->input != NULL), "Cannot malloc input buffer");
	ctx->output = malloc(ctx->output_buf_len);
	cr_assert((ctx->output != NULL), "Cannot malloc output buffer");

	/* If has noise, generate noise */
	if (has_noise) {
		ctx->noise = malloc(ctx->input_buf_len);
		cr_assert((ctx->noise != NULL), "Cannot malloc noise buffer");
		generate_noise(ctx->noise, duration);
	}

	/* Generate dtmf audio data based on the events */
	generate_dtmf_audio(events, n, ctx->input, ctx->input_buf_len,
			    ctx->noise, ctx->input_buf_len, level);

	/* Open files */
	ctx->fin = fmemopen(ctx->input, ctx->input_buf_len, "r");
	cr_assert((ctx->fin != NULL), "Cannot fmemopen input buffer");
	ctx->fout = fmemopen(ctx->output, ctx->output_buf_len, "w+");
	cr_assert((ctx->fout != NULL), "Cannot fmemopen output buffer");
}

static void cleanup_test(struct _test_context *ctx)
{
	fclose(ctx->fin);
	fclose(ctx->fout);
	free(ctx->input);
	free(ctx->output);
	if (ctx->has_noise) free(ctx->noise);
}

static void print_events(struct _dtmf_event *e, int n)
{
	for (int i = 0; i < n; ++i) {
		printf("%d\t%d\t%c\n", e[i].start_index, e[i].end_index,
		       e[i].symbol);
	}
}

static bool check_events(struct _dtmf_event *given, int ng,
			 struct _dtmf_event *detected, int nd)
{
	bool result = compare_dtmf_event_arrays(given, ng, detected, nd);
	if (!result) {
		printf("Given: \n");
		print_events(given, ng);
		printf("Detected: \n");
		print_events(detected, nd);
	}
	return result;
}

Test(detect_suite, basic, .timeout=10)
{
	struct _dtmf_event given_events[] = {{0, 1000, '0'},
					     {2000, 3000, '1'},
					     {4000, 5000, '2'},
					     {6000, 7000, '3'}};
	int N = nelem(given_events);
	const int duration_ms = 1000;
	const int output_sz = 4096;

	struct _dtmf_event detected_events[N];

	struct _test_context ctx;
	setup_test(&ctx, given_events, N, duration_ms, output_sz, false, 0);

	/* Execute dtmf_detect - output is written to ctx.fout */
	block_size = 100;
	dtmf_detect(ctx.fin, ctx.fout);

	/* Validate result */
	fflush(ctx.fout);
	int Nd = dtmf_event_from_text(detected_events, N, ctx.fout);
	bool result = check_events(given_events, N, detected_events, Nd);
	cr_assert((result),
		  "Detected events differ from given ones.\n"
		  "Output text is:\n%s\n",
		  ctx.output);

	cleanup_test(&ctx);
}

Test(detect_suite, narrower_gap, .timeout=10)
{
	struct _dtmf_event given_events[] = {
	    {0, 500, '5'},     {600, 1000, '#'},  {1100, 1500, 'A'},
	    {1600, 2000, 'D'}, {2100, 2500, 'C'}};
	int N = nelem(given_events);
	const int duration_ms = 1000;
	const int output_sz = 4096;

	struct _dtmf_event detected_events[N];

	struct _test_context ctx;
	setup_test(&ctx, given_events, N, duration_ms, output_sz, false, 0);

	/* Execute dtmf_detect - output is written to ctx.fout */
	block_size = 100;
	dtmf_detect(ctx.fin, ctx.fout);

	/* Validate result */
	fflush(ctx.fout);
	int Nd = dtmf_event_from_text(detected_events, N, ctx.fout);
	bool result = check_events(given_events, N, detected_events, Nd);
	cr_assert((result),
		  "Detected events differ from given ones.\n"
		  "Output text is:\n%s\n",
		  ctx.output);

	cleanup_test(&ctx);
}

Test(detect_suite, null_input, .timeout=10)
{
        char *inf = "/dev/null";
        char *outf = "/dev/null";
	FILE *in = fopen(inf, "r");
	FILE *out = fopen(outf, "w");

	if(in == NULL)
	    cr_skip_test("Couldn't open input file (%s)\n", inf);
	if(out == NULL)
	    cr_skip_test("Couldn't open output file (%s)\n", outf);

	block_size = 100;
	int ret = dtmf_detect(in, out);
	cr_assert((ret == EOF),
		  "Expected dtmf_detect to return failure but it didn't");

	fclose(in);
	fclose(out);
}

Test(detect_suite, full_output, .timeout=10)
{
        char *inf = "tests/rsrc/dtmf_0_500ms.au";
	char *outf = "/dev/full";
	FILE *in = fopen(inf, "r");
	FILE *out = fopen(outf, "w");

	if(in == NULL)
	    cr_skip_test("Couldn't open input file (%s)\n", inf);
	if(out == NULL)
	    cr_skip_test("Couldn't open output file (%s)\n", outf);

	block_size = 100;
	int ret = dtmf_detect(in, out);

	fclose(in);
	ret = ret || fclose(out);

	cr_assert(ret, "Expected output error but none seen.");
}

Test(detect_suite, truncated_header, .timeout=10)
{
        char *inf = "tests/rsrc/truncated_header.au";
	char *outf = "/dev/null";
	FILE *in = fopen(inf, "r");
	FILE *out = fopen(outf, "w");

	if(in == NULL)
	    cr_skip_test("Couldn't open input file (%s)\n", inf);
	if(out == NULL)
	    cr_skip_test("Couldn't open output file (%s)\n", outf);

	block_size = 100;
	int ret = dtmf_detect(in, out);
	cr_assert((ret == EOF), "Expected output error but none seen.");

	fclose(in);
	fclose(out);
}

Test(detect_suite, connecting_events, .timeout=10)
{
	struct _dtmf_event given_events[] = {{1000, 1500, 'A'},
					     {1500, 2000, 'B'},
					     {2000, 2500, 'C'},
					     {2500, 3000, 'D'},
					     {3000, 3500, '*'}};
	int N = nelem(given_events);
	const int duration_ms = 500;
	const int output_sz = 4096;

	struct _dtmf_event detected_events[N];

	struct _test_context ctx;
	setup_test(&ctx, given_events, N, duration_ms, output_sz, false, 0);

	/* Execute dtmf_detect - output is written to ctx.fout */
	block_size = 100;
	dtmf_detect(ctx.fin, ctx.fout);

	/* Validate result */
	fflush(ctx.fout);
	int Nd = dtmf_event_from_text(detected_events, N, ctx.fout);
	bool result = check_events(given_events, N, detected_events, Nd);
	cr_assert((result),
		  "Detected events differ from given ones.\n"
		  "Output text is:\n%s\n",
		  ctx.output);

	cleanup_test(&ctx);
}

Test(detect_suite, finer_grained_events, .timeout=10)
{
	struct _dtmf_event given_events[] = {
	    {0, 500, '4'},     {500, 900, '5'},	  {900, 1200, 'C'},
	    {1200, 1400, 'D'}, {1400, 1500, '#'}, {1500, 1600, '1'},
	    {1600, 1700, '0'}};
	int N = nelem(given_events);
	const int duration_ms = 500;
	const int output_sz = 4096;

	struct _dtmf_event detected_events[N];

	struct _test_context ctx;
	setup_test(&ctx, given_events, N, duration_ms, output_sz, false, 0);

	/* Execute dtmf_detect - output is written to ctx.fout */
	block_size = 100;
	dtmf_detect(ctx.fin, ctx.fout);

	/* Validate result */
	/* Expected: events shorter than 30ms should be discarded */
	struct _dtmf_event expected_events[] = {
	    {0, 500, '4'},     {500, 900, '5'},	  {900, 1200, 'C'}
	};
	int Ne = nelem(expected_events);
	fflush(ctx.fout);
	int Nd = dtmf_event_from_text(detected_events, N, ctx.fout);
	cr_assert((Ne == Nd),
		  "Expected to detect %d events, but actually %d\n"
		  "Output text is:\n%s\n",
		  Ne, Nd, ctx.output);
	bool result = check_events(expected_events, Ne, detected_events, Nd);
	cr_assert((result),
		  "Detected events differ from given ones.\n"
		  "Output text is:\n%s\n",
		  ctx.output);

	cleanup_test(&ctx);
}

Test(detect_suite, low_noise, .timeout=10)
{
	struct _dtmf_event given_events[] = {
	    {0, 500, '4'},     {1500, 2000, '5'}, {2000, 2400, 'C'},
	    {2500, 3000, 'D'}, {4000, 7000, '#'}, {7500, 8000, '1'},
	    {9000, 10000, '0'}};
	int N = nelem(given_events);
	const int duration_ms = 1500;
	const int output_sz = 4096;

	struct _dtmf_event detected_events[N];

	struct _test_context ctx;
	setup_test(&ctx, given_events, N, duration_ms, output_sz, true, -20);

	/* Execute dtmf_detect - output is written to ctx.fout */
	block_size = 100;
	dtmf_detect(ctx.fin, ctx.fout);

	/* Validate result */
	fflush(ctx.fout);
	int Nd = dtmf_event_from_text(detected_events, N, ctx.fout);
	bool result = check_events(given_events, N, detected_events, Nd);
	cr_assert((result),
		  "Detected events differ from given ones.\n"
		  "Output text is:\n%s\n",
		  ctx.output);

	cleanup_test(&ctx);
}

Test(detect_suite, high_noise, .timeout=10)
{
	struct _dtmf_event given_events[] = {
	    {0, 500, '4'},     {1500, 2000, '5'}, {2000, 2400, 'C'},
	    {2500, 3000, 'D'}, {4000, 7000, '#'}, {7500, 8000, '1'},
	    {9000, 10000, '0'}};
	int N = nelem(given_events);
	const int duration_ms = 1500;
	const int output_sz = 4096;

	struct _dtmf_event detected_events[N];

	struct _test_context ctx;
	setup_test(&ctx, given_events, N, duration_ms, output_sz, true, -5);

	/* Execute dtmf_detect - output is written to ctx.fout */
	block_size = 100;
	dtmf_detect(ctx.fin, ctx.fout);

	/* Validate result */
	fflush(ctx.fout);
	int Nd = dtmf_event_from_text(detected_events, N, ctx.fout);
	bool result = check_events(given_events, N, detected_events, Nd);
	cr_assert((result),
		  "Detected events differ from given ones.\n"
		  "Output text is:\n%s\n",
		  ctx.output);

	cleanup_test(&ctx);
}

Test(detect_suite, too_small_block, .timeout=10)
{
	struct _dtmf_event given_events[] = {
	    {0, 500, '4'},     {500, 1000, '5'},  {1100, 1200, 'C'},
	    {1200, 1400, 'D'}, {1400, 1500, '#'}, {1500, 1600, '1'},
	    {1600, 1700, '0'}};
	int N = nelem(given_events);
	const int duration_ms = 500;
	const int output_sz = 4096;

	struct _dtmf_event detected_events[N];

	struct _test_context ctx;
	setup_test(&ctx, given_events, N, duration_ms, output_sz, false, 0);

	/* Execute dtmf_detect - output is written to ctx.fout */
	block_size = 50;
	dtmf_detect(ctx.fin, ctx.fout);

	/* Validate result: Output should be empty because the block
	 * size is too small */
	fflush(ctx.fout);
	size_t output_real_len = strnlen(ctx.output, output_sz);
	cr_assert((output_real_len == 0), "Expected empty output");

	cleanup_test(&ctx);
}

Test(detect_suite, larger_block, .timeout=10)
{
	struct _dtmf_event given_events[] = {
	    {500, 600, 'A'},	{1000, 1100, '5'}, {1500, 1600, 'B'},
	    {2000, 2100, '1'},	{2500, 2700, '0'}, {3000, 3200, '1'},
	    {3500, 3700, '0'},	{4000, 4300, '2'}, {4500, 4800, '3'},
	    {5000, 5400, '4'},	{5500, 5900, '#'}, {6000, 6500, '*'},
	    {7000, 7500, 'D'},	{8000, 8600, '5'}, {9000, 9600, '7'},
	    {10000, 11000, '8'}};
	int N = nelem(given_events);
	const int duration_ms = 2000;
	const int output_sz = 4096;

	struct _dtmf_event detected_events[N];

	struct _test_context ctx;
	setup_test(&ctx, given_events, N, duration_ms, output_sz, false, 0);

	/* Execute dtmf_detect - output is written to ctx.fout */
	block_size = 600;
	dtmf_detect(ctx.fin, ctx.fout);

	/* Validate result: Output should be a different one because the block
	 * size is larger than some of the signal ranges */
	struct _dtmf_event expected[] = {
	    {2400, 3000, '0'}, {3000, 3600, '1'}, {4200, 4800, '3'},
	    {4800, 5400, '4'}, {5400, 6000, '#'}, {6000, 6600, '*'},
	    {6600, 7800, 'D'}, {7800, 9000, '5'}, {9000, 9600, '7'},
	    {9600, 11400, '8'}};
	int N2 = nelem(expected);
	fflush(ctx.fout);
	int Nd = dtmf_event_from_text(detected_events, N, ctx.fout);
	bool result = check_events(expected, N2, detected_events, Nd);
	cr_assert((result),
		  "Detected events differ from given ones.\n"
		  "Output text is:\n%s\n",
		  ctx.output);

	cleanup_test(&ctx);
}
