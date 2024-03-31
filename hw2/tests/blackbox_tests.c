#include "test_common.h"

#include <sys/stat.h>
#include <sys/types.h>

#define DTMF_PROG "bin/dtmf"
#define OUTPUT_DIR "hw1-test-output"

static void prepare_black_box_test(void)
{
	if (access(OUTPUT_DIR, F_OK) == -1) {
		mkdir(OUTPUT_DIR, 0755);
	}
}

#define DTMF_GEN_AUDIO_OUT OUTPUT_DIR "/dtmfgen_audio1.au"

Test(black_box_generate_suite, basic, .timeout=10)
{
	struct _dtmf_event events[] = {
	    {500, 600, 'A'},	{1000, 1100, '5'}, {1500, 1600, 'B'},
	    {2000, 2100, '1'},	{2500, 2700, '0'}, {3000, 3200, '1'},
	    {3500, 3700, '0'},	{4000, 4300, '2'}, {4500, 4800, '3'},
	    {5000, 5400, '4'},	{5500, 5900, '#'}, {6000, 6500, '*'},
	    {7000, 7500, 'D'},	{8000, 8600, '5'}, {9000, 9600, '7'},
	    {10000, 11000, '8'}};
	int N = nelem(events);
	char input[4096];
	size_t input_len = dtmf_event_to_text(events, N, input);
	size_t output_len = sizeof(AUDIO_HEADER) +
			    2000 * AUDIO_FRAME_RATE / 1000 * sizeof(int16_t);
	char *outbuf = malloc(output_len);
	const char *cmd = DTMF_PROG " -g -t 2000 > " DTMF_GEN_AUDIO_OUT " 2> /dev/null";

	size_t writesz, readsz;

	FILE *infp, *outfp;
	prepare_black_box_test();
	unlink(DTMF_GEN_AUDIO_OUT);
	/* Execute command using popen(), write event text */
	infp = popen(cmd, "w");
	if(infp == NULL)
	    cr_skip_test("Cannot execute command '%s'", cmd);

	writesz = fwrite(input, 1, input_len, infp);
	cr_assert((writesz == input_len), "Error writing dtmf events (%zu)", writesz);
	int err = pclose(infp);
	cr_assert_eq(err, 0, "Command did not exit successfully (pclose returned %d)", err);

	/* Read output audio and check */
	outfp = fopen(DTMF_GEN_AUDIO_OUT, "r");
	cr_assert((outfp != NULL), "Cannot open output file %s", DTMF_GEN_AUDIO_OUT);
	readsz = fread(outbuf, 1, output_len, outfp);
	cr_assert((readsz == output_len), "Error reading audio (%zu)", readsz);
	fclose(outfp);

	validate_dtmf_audio(events, N, outbuf, output_len, NULL, 0);

	/* Cleanup */
	free(outbuf);
}

#define DTMF_DET_AUDIO_IN OUTPUT_DIR "/dtmfdet_audio_in.txt"
#define DTMF_DET_EVENTS_REF OUTPUT_DIR "/dtmfdet_events_ref.txt"
#define DTMF_DET_EVENTS_OUT OUTPUT_DIR "/dtmfdet_events_out.txt"
#define DTMF_CMP_LOG_FILE OUTPUT_DIR "/cmp_check_res.txt";

Test(black_box_detect_suite, basic, .timeout=10)
{
	int noise_level = 0; // [-30, 30]
	int block_size = DEFAULT_BLOCK_SIZE; //[10, 1000]
	int audio_samples = 16000; //AUDIO_FRAME_RATE; //data length

	//Initialize command line variables
	char run_cmd[200] = {0};
	char cmp_cmd[200] = {0};
	char *cmp_log_file = DTMF_CMP_LOG_FILE;

	sprintf(run_cmd, "cat %s | ./bin/dtmf -d -b %d > %s 2> /dev/null",
		DTMF_DET_AUDIO_IN, block_size, DTMF_DET_EVENTS_OUT);
	sprintf(cmp_cmd, "cmp %s %s > %s",
		DTMF_DET_EVENTS_REF, DTMF_DET_EVENTS_OUT, cmp_log_file);

	prepare_black_box_test();
	unlink(DTMF_DET_AUDIO_IN);
	unlink(DTMF_DET_EVENTS_REF);
	unlink(DTMF_DET_EVENTS_OUT);

	//Generate event data to a file (file_name: input_file variable)
	char events[] = "500\t1500\tB\n1500\t2500\t0\n";
	int events_len = sizeof(events)/sizeof(events[0])+1;

	FILE *events_file = fopen(DTMF_DET_EVENTS_REF, "w");
	if(events_file==NULL)
	        cr_skip_test("Events file creation failed");
	fprintf(events_file, "%s", events);
	fclose(events_file);
	events_file = fopen(DTMF_DET_EVENTS_REF, "r");

	FILE *audio_res = fopen(DTMF_DET_AUDIO_IN, "w");
	if(audio_res==NULL)
		cr_skip_test("Reference audio file creation failed");
	int gen_err = generate_signal(events_file, NULL, noise_level,
				      audio_res, audio_samples, 1.0);
	if(gen_err==EOF)
		cr_skip_test("Reference audio signal generation failed");
	fclose(events_file);
	fclose(audio_res);

	FILE *infp = popen(run_cmd, "w");
	if(infp == NULL)
	        cr_assert_fail("Unable to execute command: %s", run_cmd);
	size_t writesz = fwrite(events, 1, events_len, infp);
	if(writesz != events_len)
	        cr_assert_fail("Error writing dtmf events to pipe");
	int err = pclose(infp);
	cr_assert_eq(err, 0, "Command did not exit successfully (pclose returned %d)", err);

	int cmp_file_ret = WEXITSTATUS(system(cmp_cmd));
	cr_assert_eq(cmp_file_ret, 0,
		     "Unexpected DTMF detect result, there is a mismatch between the expected result");
}

Test(exit_status_suite, no_args, .timeout=10) {
	char run_cmd[200] = {0};
	sprintf(run_cmd, "./bin/dtmf > /dev/null 2>&1");
	int exec_file_ret = WEXITSTATUS(system(run_cmd));
	cr_assert_eq(exec_file_ret, EXIT_FAILURE, "[EXIT_FAILURE] (return value %d)",
		     exec_file_ret);
}

Test(exit_status_suite, dtmf_h, .timeout=10) {
	char run_cmd[200] = {0};
	sprintf(run_cmd, "./bin/dtmf -h > /dev/null 2>&1");
	int exec_file_ret = WEXITSTATUS(system(run_cmd));
	cr_assert_eq(exec_file_ret, EXIT_SUCCESS, "[EXIT_FAILURE] (return value %d)",
		     exec_file_ret);
}

Test(exit_status_suite, dtmf_g, .timeout=10) {
	char run_cmd[200] = {0};
	sprintf(run_cmd, "./bin/dtmf -g < ./tests/rsrc/dtmf_0_500ms.txt > /dev/null 2>&1");
	int exec_file_ret = WEXITSTATUS(system(run_cmd));
	cr_assert_eq(exec_file_ret, EXIT_SUCCESS, "[EXIT_FAILURE] (return value %d)",
		     exec_file_ret);
}

Test(exit_status_suite, dtmf_d, .timeout=10) {
	char run_cmd[200] = {0};
	sprintf(run_cmd, "./bin/dtmf -d < ./tests/rsrc/dtmf_0_500ms.au > /dev/null 2>&1");
	int exec_file_ret = WEXITSTATUS(system(run_cmd));
	cr_assert_eq(exec_file_ret, EXIT_SUCCESS, "[EXIT_FAILURE] (return value %d)",
		     exec_file_ret);
}

Test(exit_status_suite, dtmf_invalid_arg, .timeout=10) {
	char run_cmd[200] = {0};
	sprintf(run_cmd, "./bin/dtmf -a > /dev/null 2>&1");
	int exec_file_ret = WEXITSTATUS(system(run_cmd));
	cr_assert_eq(exec_file_ret, EXIT_FAILURE, "[EXIT_FAILURE] (return value %d)",
		     exec_file_ret);
}
