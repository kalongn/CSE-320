#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <string.h>
#include <math.h>
#include "const.h"
#include "test_common.h"

static const double epsilon = 1e-12;

static void setup_env(TEST_SIGNAL* audio_signal, uint32_t data_length)
{
	audio_signal->output =
	    open_memstream(&audio_signal->out_content, &audio_signal->out_size);
	int gen_err =
	    generate_step_signal(audio_signal->input, audio_signal->output, data_length);

	if(gen_err == EOF)
	    cr_skip_test("Signal generation did not succeed");
}

static void exit_env(TEST_SIGNAL* audio_signal)
{
	fclose(audio_signal->input);
	fclose(audio_signal->output);
	free(audio_signal->out_content);
	audio_signal->input = NULL;
	audio_signal->output = NULL;
}

static double compute_k(int f, int block_size) {
    double k = ((double)dtmf_freqs[f] * block_size) / AUDIO_FRAME_RATE;
    return k;
}

static void compare_res(FILE* input_signal, GOERTZEL_STATE *studp, GOERTZEL_STATE *evalp,
			int block_size)
{
	for(int block = 0; 1; block++) {
		for(int f = 0; f < NUM_DTMF_FREQS; f++)	{
			double k = compute_k(f, block_size);
			goertzel_init(&studp[f], block_size, k);
			ref_goertzel_init(&evalp[f], block_size, k);
		}
		int16_t sample;
		for(int i = 0; i < block_size-1; i++) {
			if(ref_audio_read_sample(input_signal, &sample) == EOF)
				return;
			for(int f = 0; f < NUM_DTMF_FREQS; f++) {
				double x_arg = ((double)sample) / INT16_MAX;
				goertzel_step(&studp[f], x_arg);
				ref_goertzel_step(&evalp[f], x_arg);
			}
		}
		if(ref_audio_read_sample(input_signal, &sample) == EOF)
			return;

		double strength_stud[NUM_DTMF_FREQS];
		double strength_eval[NUM_DTMF_FREQS];
		for(int f = 0; f < NUM_DTMF_FREQS; f++)
		{
			double x_arg = ((double)sample) / INT16_MAX;
			strength_stud[f] = goertzel_strength(&studp[f], x_arg);
			strength_eval[f] = ref_goertzel_strength(&evalp[f], x_arg);

			cr_assert(fabs(strength_stud[f] - strength_eval[f]) < epsilon,
				"Unexpected strengths value for frequency %dHz,\
				(%lf != expected value %lf)",
				dtmf_freqs[f], strength_stud[f], strength_eval[f]);
		}
	}
}

static void assert_equal_state(GOERTZEL_STATE *studp, GOERTZEL_STATE *refp) {
	//N value check
	cr_assert_eq(studp->N, refp->N,
		     "Unexpected N value in Goertzel state, (%u != expected value %u)",
		     studp->N, refp->N);

	//K value check
	cr_assert_eq(studp->k, refp->k,
		     "Unexpected K value in Goertzel state, (%lf != expected value %lf)",
		     studp->k, refp->k);

	//A value check
	cr_assert_eq(studp->A, refp->A,
		     "Unexpected A value in Goertzel state, (%lf != expected value %lf)",
		     studp->A, refp->A);

	//B value check
	cr_assert_eq(studp->B, refp->B,
		     "Unexpected B value in Goertzel state, (%lf != expected value %lf)",
		     studp->B, refp->B);

	//s1 value check
	cr_assert_eq(studp->s1, refp->s1,
		     "Unexpected s1 value in Goertzel state, (%lf != expected value %lf)",
		     studp->s1, refp->s1);

	//s2 value check
	cr_assert_eq(studp->s2, refp->s2,
		     "Unexpected s2 value in Goertzel state, (%lf != expected value %lf)",
		     studp->s2, refp->s2);
}

Test(goertzel_suite, goertzel_init, .timeout=10) {
	const int freq_index = 3;
	const int block_size = 191;
	const double k = compute_k(freq_index, block_size);

	//Clear Goertzel structures to start from a clean state
	GOERTZEL_STATE stud, ref;
	memset(&stud, 0, sizeof(GOERTZEL_STATE));
	memset(&ref, 0, sizeof(GOERTZEL_STATE));

	//Get student's result
	goertzel_init(&stud, block_size, k);

	//Get expected result
	ref_goertzel_init(&ref, block_size, k);

	// Compare results
	assert_equal_state(&stud, &ref);
}

Test(goertzel_suite, goertzel_step, .timeout=10) {
	const int block_size = 10;
	const int16_t sample_arr[] = {0x1135, 0xe1d0, 0xdbbd, 0x30ff, 0xe005};
	int sample_arr_len = sizeof(sample_arr) / sizeof(sample_arr[0]);
	GOERTZEL_STATE stud[NUM_DTMF_FREQS];
	GOERTZEL_STATE ref[NUM_DTMF_FREQS];

	//Initialize Goertzel state using the reference implementation.
	for(int f = 0; f < NUM_DTMF_FREQS; f++) {
		double k = compute_k(f, block_size);
		ref_goertzel_init(&stud[f], block_size, k);
		ref_goertzel_init(&ref[f], block_size, k);
	}

	//Call Goertzel_step function
	for(int i = 0; i < sample_arr_len; i++) {
		for(int f = 0; f < NUM_DTMF_FREQS; f++) {
			//Assign sample value from sample_arr
			int16_t sample = sample_arr[i];
			goertzel_step(&stud[f], ((double)sample) / INT16_MAX);
			ref_goertzel_step(&ref[f], ((double)sample) / INT16_MAX);
		}
	}

	//Compare result with precalculated answer
	for(int f = 0; f < NUM_DTMF_FREQS; f++) {
		double s0_res = stud[f].s0;
		double s1_res = stud[f].s1;
		double s2_res = stud[f].s2;
		double s0_exp_res = ref[f].s0;
		double s1_exp_res = ref[f].s1;
		double s2_exp_res = ref[f].s2;

		cr_assert((fabs(s0_res - s0_exp_res) < epsilon),
			"Calculation error, s0 (%lf != expected value %lf)",
			s0_res, s0_exp_res);
		cr_assert((fabs(s1_res - s0_exp_res) < epsilon),
			"Calculation error, s1 (%lf != expected value %lf)",
			s1_res, s0_exp_res);
		cr_assert((fabs(s2_res - s2_exp_res) < epsilon),
			"Calculation error, s2 (%lf != expected value %lf)",
			s2_res, s2_exp_res);
	}
}

Test(goertzel_suite, goertzel_strength, .timeout=10) {

	const int block_size = 10;
	const int16_t sample_arr[] = {
	    0x1135, 0xe1d0, 0xdbbd, 0x30ff, 0xe005,
	    0x3fff, 0xe941, 0xcc58, 0x185e, 0xfe1e
	};

	int sample_arr_len = sizeof(sample_arr) / sizeof(sample_arr[0]);
	GOERTZEL_STATE stud[NUM_DTMF_FREQS];
	GOERTZEL_STATE ref[NUM_DTMF_FREQS];

	//Manually calculate initialize Goertzel structure
	for(int f = 0; f < NUM_DTMF_FREQS; f++) {
		double k = compute_k(f, block_size);
		ref_goertzel_init(&stud[f], block_size, k);
	}

	int16_t sample;
	//Manually calculate Goertzel_step
	for(int i = 0; i < sample_arr_len-1; i++) {
		for(int f = 0; f < NUM_DTMF_FREQS; f++) {
			//Assign sample value from sample_arr
			sample = sample_arr[i];
			double x_arg = (double)sample / INT16_MAX;
			ref_goertzel_step(&stud[f], x_arg);
			memcpy(&ref[f], &stud[f], sizeof(ref[f]));
		}
	}

	//Call Goertzel_strength function
	sample = sample_arr[sample_arr_len-1];
	double stud_res[NUM_DTMF_FREQS];
	double ref_res[NUM_DTMF_FREQS];
	for(int f = 0; f < NUM_DTMF_FREQS; f++) {
		stud_res[f] = goertzel_strength(&stud[f], ((double)sample) / INT16_MAX);
		ref_res[f] = ref_goertzel_strength(&ref[f], ((double)sample) / INT16_MAX);
	}

	//Compare result with precalculated answer
	for(int f = 0; f < NUM_DTMF_FREQS; f++) {
		cr_assert((fabs(stud_res[f] - ref_res[f]) < epsilon),
			  "Calculation error, strengths value (%lf != expected value %lf)",
			  stud_res[f], ref_res[f]);
	}
}

Test(goertzel_suite, step_signal, .timeout=10) {
	const int block_size = 150;
	const uint32_t data_length = 6000;
	char content[] = "0\t800\t16000\n1600\t2400\t17000\n3200\t4000\t18000\n4800\t5600\t19000\n";
	//start_ind, end_ind, const val (uint16)

	TEST_SIGNAL audio_signal;
	audio_signal.input = fmemopen(content, sizeof(content)/sizeof(content[0])+1, "r");
	setup_env(&audio_signal, data_length);

	GOERTZEL_STATE stud[NUM_DTMF_FREQS];
	GOERTZEL_STATE ref[NUM_DTMF_FREQS];

	AUDIO_HEADER hdr;
	if(ref_audio_read_header(audio_signal.output, &hdr) == EOF)
		cr_skip_test("Read back header did not succeed");
	compare_res(audio_signal.output, stud, ref, block_size);
	exit_env(&audio_signal);
}

Test(goertzel_suite, zero_signal, .timeout=10) {
	const int block_size = 200;
	const uint32_t data_length = 8000;
	char content[] = "400\t1000\t0\n"; 	//start_ind, end_ind, const val (uint16)

	TEST_SIGNAL audio_signal;
	audio_signal.input = fmemopen(content, sizeof(content)/sizeof(content[0])+1, "r");
	setup_env(&audio_signal, data_length);

	GOERTZEL_STATE stud[NUM_DTMF_FREQS];
	GOERTZEL_STATE ref[NUM_DTMF_FREQS];

	AUDIO_HEADER hdr;
	if(ref_audio_read_header(audio_signal.output, &hdr) == EOF)
		cr_skip_test("Read back header did not succeed");
	compare_res(audio_signal.output, stud, ref, block_size);
	exit_env(&audio_signal);
}
