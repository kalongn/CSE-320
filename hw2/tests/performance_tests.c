#include "test_common.h"

#include <sys/stat.h>
#include <sys/types.h>

#define PERFORMANCE_DIR "hw1-test-output"

static void prepare_black_box_test(void)
{
	if (access(PERFORMANCE_DIR, F_OK) == -1) {
		mkdir(PERFORMANCE_DIR, 0755);
	}
}

#define PERFORMANCE_GENERATE_INPUT_FILENAME "tests/rsrc/performance_events.txt"
#define PERFORMANCE_GENERATE_OUTPUT_FILENAME "hw1-test-output/performance_audio_out.au"
#define PERFORMANCE_GENERATE_EXPECT_FILE_NAME "tests/rsrc/performance_audio.au"

Test(performance_suite, generate_basic, .timeout=10)
{
	prepare_black_box_test();

	char *cmd = "bin/dtmf -g -t 300000 < "PERFORMANCE_GENERATE_INPUT_FILENAME" > "PERFORMANCE_GENERATE_OUTPUT_FILENAME;
	int return_code = WEXITSTATUS(system(cmd));
	cr_assert_eq(return_code, EXIT_SUCCESS, "Incorrect exit status. Expected: 0x%x | Got: 0x%x", EXIT_SUCCESS, return_code);

	char *cmp = "cmp "PERFORMANCE_GENERATE_OUTPUT_FILENAME" "PERFORMANCE_GENERATE_EXPECT_FILE_NAME;
	int cmp_return_code = WEXITSTATUS(system(cmp));
	cr_assert_eq(cmp_return_code, EXIT_SUCCESS, "Output mismatch. Expected: 0x%x | Got: 0x%x", EXIT_SUCCESS, cmp_return_code);
}

#define PERFORMANCE_DETECT_INPUT_FILENAME "tests/rsrc/performance_audio.au"
#define PERFORMANCE_DETECT_OUTPUT_FILENAME "hw1-test-output/performance_events_out.txt"
#define PERFORMANCE_DETECT_EXPECT_FILE_NAME "tests/rsrc/performance_events.txt"

Test(performance_suite, detect_basic, .timeout=10)
{
	prepare_black_box_test();

	char *cmd = "bin/dtmf -d < "PERFORMANCE_DETECT_INPUT_FILENAME" > "PERFORMANCE_DETECT_OUTPUT_FILENAME;
	int return_code = WEXITSTATUS(system(cmd));
	cr_assert_eq(return_code, EXIT_SUCCESS, "Incorrect exit status. Expected: 0x%x | Got: 0x%x", EXIT_SUCCESS, return_code);

	char *cmp = "cmp "PERFORMANCE_DETECT_OUTPUT_FILENAME" "PERFORMANCE_DETECT_EXPECT_FILE_NAME;
	int cmp_return_code = WEXITSTATUS(system(cmp));
	cr_assert_eq(cmp_return_code, EXIT_SUCCESS, "Output mismatch. Expected: 0x%x | Got: 0x%x", EXIT_SUCCESS, cmp_return_code);
}
