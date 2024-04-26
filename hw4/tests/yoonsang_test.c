#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <stdio.h>
#include<sys/time.h>
#include "unistd.h"


#include "legion.h"
#include "event.h"
#include "driver.h"
#include "tracker.h"
#include "__helper.h"

#define QUOTE1(x) #x
#define ECHO_CMD1(x) echo x
#define SCRIPT1(x) x##_script
#define SCRIPT(x) SCRIPT1(x)
#define QUOTE(x) QUOTE1(x)

#define LEGION_EXECUTABLE "bin/legion"
#define EXTRA_ARG_ANSWER_FILE "tests/rsrc/my_extra_arg.answer"

/*
 * Finalization function to try to ensure no stray processes are left over
 * after a test finishes.
 */
static void killall()
{
  system("killall -s KILL bin/legion");
}

static void reset_log_file(char* logfile_name, int version)
{
    char rm_cmd[500];
    sprintf(rm_cmd,"rm -f logs/'%s'.log.%d", logfile_name, version);
    system(rm_cmd);
}

static void reset_all_log_file(char* logfile_name)
{
    for (int i=0;i<LOG_VERSIONS;i++)
        reset_log_file(logfile_name, i);
}

//____________________________________________

#define SUITE logcheck_suite

//Log existence check [Daemon : logcheck]
#define TEST_NAME log_exists_test
#define PROG_NAME "my_log"
#define cmd_1 "register my_log logcheck"
#define cmd_2 "start my_log"
#define cmd_3 "quit"
static COMMAND SCRIPT(TEST_NAME)[] =
{
    // send,        expect,         modifiers,  timeout,    before,    after
    {  NULL,        INIT_EVENT,      0,          HND_MSEC,   NULL,      NULL },
    {  NULL,	    PROMPT_EVENT,   0,          HND_MSEC,   NULL,      NULL },
    {  cmd_1,       NO_EVENT,       0,          HND_MSEC,   NULL,      NULL },
    {  NULL,        REGISTER_EVENT,      0,          HND_MSEC,   NULL,      NULL },
    {  NULL,	    PROMPT_EVENT,   0,          HND_MSEC,   NULL,      NULL },
    {  cmd_2,       NO_EVENT,       0,          HND_MSEC,   NULL,      NULL },
    {  NULL,        START_EVENT,      0,          ONE_SEC,    NULL,      NULL },
    {  NULL,        ACTIVE_EVENT,      0,          TWO_SEC,    NULL,      NULL },
    {  NULL,	    PROMPT_EVENT,   0,          HND_MSEC,   NULL,      NULL },
    {  cmd_3,       NO_EVENT,       0,          HND_MSEC,   NULL,      NULL },
    {  NULL,        STOP_EVENT,         0,          THTY_SEC,   NULL,      NULL },
    {  NULL,        TERM_EVENT,         0,          TWO_SEC,    NULL,      NULL },
    {  NULL,        FINI_EVENT,         0,          HND_MSEC,   NULL,      NULL },
    {  NULL,        EOF_EVENT,      0,          TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .fini = killall, .timeout = 300)
{
    reset_all_log_file(PROG_NAME);

    int status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {LEGION_EXECUTABLE, "auto", NULL};
    run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_log_exists_check(PROG_NAME, 0);
}
#undef cmd_3
#undef cmd_2
#undef cmd_1
#undef PROG_NAME
#undef TEST_NAME

//____________________________________________

//Logrotate check [Daemon : logcheck]
#define TEST_NAME logrotate_test
#define PROG_NAME "my_log_rot"
#define cmd_1 "register my_log_rot logcheck"
#define cmd_2 "start my_log_rot"
#define cmd_3 "logrotate my_log_rot"
#define cmd_4 "logrotate my_log_rot"
#define cmd_5 "logrotate my_log_rot"
#define cmd_6 "stop my_log_rot"
#define cmd_7 "quit"
static COMMAND SCRIPT(TEST_NAME)[] =
{
    // send,        expect,                    modifiers,  timeout,    before,    after
    {  NULL,        INIT_EVENT,                 0,          HND_MSEC,   NULL,      NULL },
    {  NULL,	    PROMPT_EVENT,   0,          HND_MSEC,   NULL,      NULL },
    {  cmd_1,       NO_EVENT,                  0,          HND_MSEC,   NULL,      NULL },
    {  NULL,        REGISTER_EVENT,                 0,          HND_MSEC,   NULL,      NULL },
    {  NULL,	    PROMPT_EVENT,   0,          HND_MSEC,   NULL,      NULL },
    {  cmd_2,       NO_EVENT,                  0,          HND_MSEC,   NULL,      NULL },
    {  NULL,        START_EVENT,                 0,          ONE_SEC,    NULL,      NULL },
    {  NULL,        ACTIVE_EVENT,                 0,          TWO_SEC,    NULL,      NULL },
    {  NULL,	    PROMPT_EVENT,   0,          HND_MSEC,   NULL,      NULL },
    {  cmd_3,       NO_EVENT,                  0,          HND_MSEC,   NULL,      NULL },
    {  NULL,        LOGROTATE_EVENT,    EXPECT_SKIP_OTHER, TWO_SEC,    NULL,      NULL },
    {  NULL,        START_EVENT,        EXPECT_SKIP_OTHER, ONE_SEC,    NULL,      NULL },
    {  NULL,        ACTIVE_EVENT,              0,          TWO_SEC,    NULL,      NULL },
    {  NULL,	    PROMPT_EVENT,   0,          HND_MSEC,   NULL,      NULL },
    {  cmd_4,       NO_EVENT,                  0,          HND_MSEC,   NULL,      NULL },
    {  NULL,        LOGROTATE_EVENT,    EXPECT_SKIP_OTHER, TWO_SEC,    NULL,      NULL },
    {  NULL,        START_EVENT,        EXPECT_SKIP_OTHER, ONE_SEC,    NULL,      NULL },
    {  NULL,        ACTIVE_EVENT,              0,          TWO_SEC,    NULL,      NULL },
    {  NULL,	    PROMPT_EVENT,   0,          HND_MSEC,   NULL,      NULL },
    {  cmd_5,       NO_EVENT,                  0,          HND_MSEC,   NULL,      NULL },
    {  NULL,        LOGROTATE_EVENT,    EXPECT_SKIP_OTHER, TWO_SEC,    NULL,      NULL },
    {  NULL,        START_EVENT,        EXPECT_SKIP_OTHER, ONE_SEC,    NULL,      NULL },
    {  NULL,        ACTIVE_EVENT,              0,          TWO_SEC,    NULL,      NULL },
    {  NULL,	    PROMPT_EVENT,   0,          HND_MSEC,   NULL,      NULL },
    {  cmd_6,       NO_EVENT,                  0,          HND_MSEC,   NULL,      NULL },
    {  NULL,        STOP_EVENT,                0,          TWO_SEC,    NULL,      NULL },
    {  NULL,        TERM_EVENT,                0,          TWO_SEC,    NULL,      NULL },
    {  NULL,	    PROMPT_EVENT,   0,          HND_MSEC,   NULL,      NULL },
    {  cmd_7,       NO_EVENT,                  0,          HND_MSEC,   NULL,      NULL },
    {  NULL,        FINI_EVENT,                0,          HND_MSEC,   NULL,      NULL },
    {  NULL,        EOF_EVENT,                 0,          TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .fini = killall, .timeout = 300)
{
    int num_of_log_rotate = 3;
    reset_all_log_file(PROG_NAME);

    int status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {LEGION_EXECUTABLE, "auto", NULL};
    run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);

    for (int i=0;i<=num_of_log_rotate;i++)
        assert_log_exists_check(PROG_NAME, i);

}
#undef cmd_7
#undef cmd_6
#undef cmd_5
#undef cmd_4
#undef cmd_3
#undef cmd_2
#undef cmd_1
#undef PROG_NAME
#undef TEST_NAME

//____________________________________________

//Extra arg check [Daemon : logcheck]
#define TEST_NAME register_extra_arg_test
#define PROG_NAME "my_extra_arg"
#define cmd_1 "register my_extra_arg logcheck these are extra args"
#define cmd_2 "start my_extra_arg"
#define cmd_3 "stop my_extra_arg"
#define cmd_4 "quit"
static COMMAND SCRIPT(TEST_NAME)[] =
{
    // send,            expect,         modifiers,  timeout,    before,    after
    {  NULL,        INIT_EVENT,              0,          HND_MSEC,   NULL,      NULL },
    {  NULL,	    PROMPT_EVENT,   0,          HND_MSEC,   NULL,      NULL },
    {  cmd_1,       NO_EVENT,               0,          HND_MSEC,   NULL,      NULL },
    {  NULL,        REGISTER_EVENT,              0,          HND_MSEC,   NULL,      NULL },
    {  NULL,	    PROMPT_EVENT,   0,          HND_MSEC,   NULL,      NULL },
    {  cmd_2,       NO_EVENT,               0,          HND_MSEC,   NULL,      NULL },
    {  NULL,        START_EVENT,              0,          ONE_SEC,    NULL,      NULL },
    {  NULL,        ACTIVE_EVENT,              0,          TWO_SEC,    NULL,      NULL },
    {  NULL,	    PROMPT_EVENT,   0,          HND_MSEC,   NULL,      NULL },
    {  cmd_3,       NO_EVENT,               0,          HND_MSEC,   NULL,      NULL },
    {  NULL,        STOP_EVENT,             0,          TWO_SEC,    NULL,      NULL },
    {  NULL,        TERM_EVENT,             0,          TWO_SEC,    NULL,      NULL },
    {  NULL,	    PROMPT_EVENT,   0,          HND_MSEC,   NULL,      NULL },
    {  cmd_4,       NO_EVENT,               0,          HND_MSEC,   NULL,      NULL },
    {  NULL,        FINI_EVENT,             0,          HND_MSEC,   NULL,      NULL },
    {  NULL,        EOF_EVENT,              0,          TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .fini = killall, .timeout = 1000)
{
    reset_all_log_file(PROG_NAME);

    int status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {LEGION_EXECUTABLE, "auto", NULL};
    run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);

    assert_log_exists_check(PROG_NAME, 0);
    assert_diff_check(EXTRA_ARG_ANSWER_FILE, PROG_NAME, 0);
}
#undef cmd_4
#undef cmd_3
#undef cmd_2
#undef cmd_1
#undef PROG_NAME
#undef TEST_NAME

//____________________________________________

//Emptyspace processing-parse check [Daemon : logcheck]
//: Logfile names containing spacings require special parsing (=using quote chars)
#define TEST_NAME emptyspace_parse_test
#define PROG_NAME "my emptyspace test"
#define cmd_1 "register 'my emptyspace test' logcheck"
#define cmd_2 "start 'my emptyspace test'"
#define cmd_3 "stop 'my emptyspace test'"
#define cmd_4 "quit"
static COMMAND SCRIPT(TEST_NAME)[] =
{
    // send,            expect,         modifiers,  timeout,    before,    after
    {  NULL,        INIT_EVENT,              0,          HND_MSEC,   NULL,      NULL },
    {  NULL,	    PROMPT_EVENT,   0,          HND_MSEC,   NULL,      NULL },
    {  cmd_1,       NO_EVENT,               0,          HND_MSEC,   NULL,      NULL },
    {  NULL,        REGISTER_EVENT,              0,          HND_MSEC,   NULL,      NULL },
    {  NULL,	    PROMPT_EVENT,   0,          HND_MSEC,   NULL,      NULL },
    {  cmd_2,       NO_EVENT,               0,          HND_MSEC,   NULL,      NULL },
    {  NULL,        START_EVENT,              0,          ONE_SEC,    NULL,      NULL },
    {  NULL,        ACTIVE_EVENT,              0,          TWO_SEC,    NULL,      NULL },
    {  NULL,	    PROMPT_EVENT,   0,          HND_MSEC,   NULL,      NULL },
    {  cmd_3,       NO_EVENT,               0,          HND_MSEC,   NULL,      NULL },
    {  NULL,        STOP_EVENT,             0,          TWO_SEC,    NULL,      NULL },
    {  NULL,        TERM_EVENT,             0,          TWO_SEC,    NULL,      NULL },
    {  NULL,	    PROMPT_EVENT,   0,          HND_MSEC,   NULL,      NULL },
    {  cmd_4,       NO_EVENT,               0,          HND_MSEC,   NULL,      NULL },
    {  NULL,        FINI_EVENT,             0,          HND_MSEC,   NULL,      NULL },
    {  NULL,        EOF_EVENT,              0,          TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .fini = killall, .timeout = 300)
{
    reset_all_log_file(PROG_NAME);

    int status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {LEGION_EXECUTABLE, "auto", NULL};
    run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);

    assert_log_exists_check(PROG_NAME, 0);
}
#undef cmd_4
#undef cmd_3
#undef cmd_2
#undef cmd_1
#undef PROG_NAME
#undef TEST_NAME
