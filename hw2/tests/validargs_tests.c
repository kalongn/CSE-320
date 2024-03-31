#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <string.h>
#include "const.h"

#define FLAG_BITS 0x7

/* bin/dtmf */
Test(validargs_suite, dtmf, .timeout=10) {
    char *argv[] = {"bin/dtmf", NULL};
    int argc = sizeof(argv)/sizeof(char *) - 1;
    int ret = validargs(argc, argv);
    int exp_ret = -1;
    cr_assert_eq(ret, exp_ret, "Invalid return for validargs.  Got: %d | Expected: %d",
		 ret, exp_ret);
}

/* bin/dtmf -h */
Test(validargs_suite, dtmf_h, .timeout=10) {
    char* noise_file_exp;
    char *argv[] = {"bin/dtmf", "-h", NULL};
    int argc = sizeof(argv)/sizeof(char *) - 1;
    int ret = validargs(argc, argv);
    int exp_ret = 0;
    int flag = 0x1;
    cr_assert_eq(ret, exp_ret, "Invalid return for validargs.  Got: %d | Expected: %d",
		 ret, exp_ret);
    cr_assert_eq(global_options & FLAG_BITS, flag, "Correct bit (0x%x) not set for -h. Got: %x",
		 flag, global_options);
}

/* bin/dtmf -g */
Test(validargs_suite, dtmf_g, .timeout=10) {
    char *argv[] = {"bin/dtmf", "-g", NULL};
    int argc = sizeof(argv)/sizeof(char *) - 1;
    int ret = validargs(argc, argv);
    int exp_ret = 0;
    int flag = 0x2;
    cr_assert_eq(ret, exp_ret, "Invalid return for validargs.  Got: %d | Expected: %d",
		 ret, exp_ret);
    cr_assert_eq(global_options & FLAG_BITS, flag, "Correct bit (0x%x) not set for -h. Got: %x",
		 flag, global_options);
}

/* bin/dtmf -d */
Test(validargs_suite, dtmf_d, .timeout=10) {
    char *argv[] = {"bin/dtmf", "-d", NULL};
    int argc = sizeof(argv)/sizeof(char *) - 1;
    int ret = validargs(argc, argv);
    int exp_ret = 0;
    int flag = 0x4;
    cr_assert_eq(ret, exp_ret, "Invalid return for validargs.  Got: %d | Expected: %d",
		 ret, exp_ret);
    cr_assert_eq(global_options & FLAG_BITS, flag, "Correct bit (0x%x) not set for -h. Got: %x",
		 flag, global_options);
}

/* bin/dtmf -g -t 2000 */
Test(validargs_suite, dtmf_g_t_2000, .timeout=10) {
    char *argv[] = {"bin/dtmf", "-g", "-t", "2000", NULL};
    int argc = sizeof(argv)/sizeof(char *) - 1;
    int ret = validargs(argc, argv);
    int exp_ret = 0;
    int flag = 0x2;
    int samples = 2000 * AUDIO_FRAME_RATE / 1000;
    cr_assert_eq(ret, exp_ret, "Invalid return for validargs.  Got: %d | Expected: %d",
		 ret, exp_ret);
    cr_assert_eq(global_options & FLAG_BITS, flag, "Correct bit (0x%x) not set for -h. Got: %x",
		 flag, global_options);
    cr_assert_eq(audio_samples, samples, "Correct audio_samples (%d) not set for -t. Got: %d",
		 audio_samples);
}

/* bin/dtmf -g -n noise_file_exp */
Test(validargs_suite, dtmf_g_n_noise_file, .timeout=10) {
    char *noise_file_exp = "./rsrc/white_noise_10s.au";
    char *argv[] = {"bin/dtmf", "-g", "-n", noise_file_exp, NULL};
    int argc = sizeof(argv)/sizeof(char *) - 1;
    int ret = validargs(argc, argv);
    int exp_ret = 0;
    int flag = 0x2;
    cr_assert_eq(ret, exp_ret, "Invalid return for validargs.  Got: %d | Expected: %d",
		 ret, exp_ret);
    cr_assert_eq(global_options & FLAG_BITS, flag, "Correct bit (0x%x) not set for -h. Got: %x",
		 flag, global_options);
    cr_assert(!strcmp(noise_file, noise_file_exp),
	      "Correct noise_file (\"%s\") not set for -g. Got: %s",
	      noise_file_exp, noise_file);
}

/* bin/dtmf -g -n noise_file_exp -l level_str */
Test(validargs_suite, dtmf_g_n_noise_file_l_level, .timeout=10) {
    char *noise_file_exp = "./rsrc/white_noise_10s.au";
    char *level_str = "-3";
    int level_exp = atoi(level_str);
    char *argv[] = {"bin/dtmf", "-g", "-n", noise_file_exp, "-l", level_str, NULL};
    int argc = sizeof(argv)/sizeof(char *) - 1;
    int ret = validargs(argc, argv);
    int exp_ret = 0;
    int flag = 0x2;
    cr_assert_eq(ret, exp_ret, "Invalid return for validargs.  Got: %d | Expected: %d",
		 ret, exp_ret);
    cr_assert_eq(global_options & FLAG_BITS, flag, "Correct bit (0x%x) not set for -h. Got: %x",
		 flag, global_options);
    cr_assert(!strcmp(noise_file, noise_file_exp),
	      "Correct noise_file (\"%s\") not set for -g. Got: %s",
	      noise_file_exp, noise_file);
    cr_assert_eq(noise_level, level_exp, "Correct level (%d) not set for -l. Got: %d",
		 level_exp, noise_level);
}

/* bin/dtmf -d -b bsize_str */
Test(validargs_suite, dtmf_d_b_bsize, .timeout=10) {
    char *bsize_str = "120";
    int bsize_exp = atoi(bsize_str);
    char *argv[] = {"bin/dtmf", "-d", "-b", bsize_str, NULL};
    int argc = sizeof(argv)/sizeof(char *) - 1;
    int ret = validargs(argc, argv);
    int exp_ret = 0;
    int flag = 0x4;
    cr_assert_eq(ret, exp_ret, "Invalid return for validargs.  Got: %d | Expected: %d",
		 ret, exp_ret);
    cr_assert_eq(global_options & FLAG_BITS, flag, "Correct bit (0x%x) not set for -h. Got: %x",
		 flag, global_options);
    cr_assert_eq(bsize_exp, block_size, "Correct block_size (%d) not set for -b. Got: %d",
		 bsize_exp, block_size);
}

/* bin/dtmf -d -b -1 */
Test(validargs_suite, dtmf_d_b_invalidBSize, .timeout=10) {
    char* bsize_str = "-1";
    int bsize_exp = atoi(bsize_str);
    char *argv[] = {"bin/dtmf", "-d", "-b", bsize_str, NULL};
    int argc = sizeof(argv)/sizeof(char *) - 1;
    int ret = validargs(argc, argv);
    int exp_ret = -1;
    cr_assert_eq(ret, exp_ret, "Invalid return for validargs.  Got: %d | Expected: %d",
		 ret, exp_ret);
}

/* bin/dtmf -h -g -d */
Test(validargs_suite, dtmf_h_g_d, .timeout=10) {
    char *argv[] = {"bin/dtmf", "-h", "-d", "-g", NULL};
    int argc = sizeof(argv)/sizeof(char *) - 1;
    int ret = validargs(argc, argv);
    int flag = 0x1;
    int exp_ret = 0;
    cr_assert_eq(ret, exp_ret, "Invalid return for validargs.  Got: %d | Expected: %d",
		 ret, exp_ret);
    cr_assert_eq(global_options & FLAG_BITS, flag, "Correct bit (0x%x) not set for -h. Got: %x",
		 flag, global_options);
}

/* ./bin/dtmf -g -n noise_file -l [-30, 30] */
Test(validargs_suite, dtmf_g_n_noiseFile_l_level, .timeout=10) {
    char *noise_file_exp = "./rsrc/white_noise_10s.au";
    char *argv[] = {"bin/dtmf", "-g", "-n", noise_file_exp, "-l", "0", NULL};
    int argc = sizeof(argv)/sizeof(char *) - 1;
    int ret = validargs(argc, argv);
    int opt = global_options;
    int exp_ret = 0;
    int flag = 0x2;
    cr_assert_eq(ret, exp_ret, "Invalid return for validargs.  Got: %d | Expected: %d",
		 ret, exp_ret);
    cr_assert_eq(global_options & FLAG_BITS, flag, "Correct bit (0x%x) not set for -h. Got: %x",
		 flag, global_options);
    cr_assert(!strcmp(noise_file, noise_file_exp),
	      "Variable 'noise_file' was not properly set.  Got: %s | Expected: %s",
	      noise_file, noise_file_exp);
}

/* ./bin/dtmf -g -n noise_file -l -70 */
Test(validargs_suite, dtmf_g_n_noiseFile_l_invalidLevel, .timeout=10) {
    char *noise_file_exp = "./rsrc/white_noise_10s.au";
    char *argv[] = {"bin/dtmf", "-g", "-n", noise_file_exp, "-l", "-70", NULL};
    int argc = sizeof(argv)/sizeof(char *) - 1;
    int ret = validargs(argc, argv);
    int exp_ret = -1;
    cr_assert_eq(ret, exp_ret, "Invalid return for validargs.  Got: %d | Expected: %d",
		 ret, exp_ret);
}

/* ./bin/dtmf -a */
Test(validargs_suite, dtmf_illegal_flags, .timeout=10) {
    char *argv[] = {"bin/dtmf", "-a", NULL};
    int argc = sizeof(argv)/sizeof(char *) - 1;
    int ret = validargs(argc, argv);
    int exp_ret = -1;
    cr_assert_eq(ret, exp_ret, "Invalid return for validargs.  Got: %d | Expected: %d",
		 ret, exp_ret);
}
