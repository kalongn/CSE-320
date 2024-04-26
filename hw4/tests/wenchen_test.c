#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <stdio.h>
#include<sys/time.h>

#include "event.h"
#include "driver.h"
#include "tracker.h"
#include "__helper.h"

#define QUOTE1(x) #x
#define QUOTE(x) QUOTE1(x)
#define SCRIPT1(x) x##_script
#define SCRIPT(x) SCRIPT1(x)

#define LEGION_EXECUTABLE "bin/legion"

/*
 * Finalization function to try to ensure no stray processes are left over
 * after a test finishes.
 */
static void killall() {
  system("killall -s KILL bin/legion");
  system("killall -s KILL crash");
  system("killall -s KILL lazy");
  system("killall -s KILL systat");
  system("killall -s KILL logcheck");
  system("killall -s KILL nosync");
}

#if 0 /* COMMENT_1 */
#endif /* COMMENT_1 */

/*---------------------------test unknown cmd error----------------------------*/
#define SUITE tracker_suite
#define TEST_NAME unknown_quit_test
#define unknown "unknown"
#define quit_cmd "quit"
static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,            expect,			modifiers,  timeout,    before,    after
    {  NULL,		INIT_EVENT,		    0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  unknown,		NO_EVENT,		    0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		ERROR_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  quit_cmd,	NO_EVENT,		    0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		FINI_EVENT,		    0,          TWO_SEC,	NULL,      NULL },
    {  NULL,            EOF_EVENT,		0,          TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .fini = killall, .timeout = 30)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {LEGION_EXECUTABLE, "auto", NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}
#undef  unknown
#undef  quit_cmd
#undef TEST_NAME

/*---------------------------test quit cmd------------------------------------*/
#define SUITE tracker_suite
#define TEST_NAME quit_test
#define quit_cmd "quit"
static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,            expect,			modifiers,  timeout,    before,    after
    {  NULL,		INIT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  quit_cmd,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		FINI_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,            EOF_EVENT,		0,          TEN_MSEC,   NULL,      NULL }
};


Test(SUITE, TEST_NAME, .fini = killall, .timeout = 30)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {LEGION_EXECUTABLE, "auto", NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}
#undef  quit_cmd
#undef TEST_NAME

/*---------------------------test help cmd------------------------------------*/
#define SUITE tracker_suite
#define TEST_NAME help_test
#define help_cmd "help"
#define quit_cmd "quit"
static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,            expect,			modifiers,  timeout,    before,    after
    {  NULL,		INIT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  help_cmd,	PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },	// expect no ERROR event
    {  quit_cmd,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		FINI_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,            EOF_EVENT,		0,          TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .fini = killall, .timeout = 30)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {LEGION_EXECUTABLE, "auto", NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}
#undef  help_cmd
#undef  quit_cmd
#undef TEST_NAME


/*---------------------------test register cmd--------------------------------*/
#define SUITE tracker_suite
#define TEST_NAME regi_quit_test
#define lzy_cmd_1 "register lazy lazy"
#define lzy_cmd_2 "quit"
static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,            expect,			modifiers,  timeout,    before,    after
    {  NULL,		INIT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  lzy_cmd_1,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		REGISTER_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  lzy_cmd_2,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		FINI_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,            EOF_EVENT,		0,          TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .fini = killall, .timeout = 30)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {LEGION_EXECUTABLE, "auto", NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}
#undef  lzy_cmd_1
#undef  lzy_cmd_2
#undef TEST_NAME

/*---------------------------test register the same name error----------------*/
#define SUITE tracker_suite
#define TEST_NAME regi_same_name_error_test
#define reg_cmd_1 "register lazy lazy"
#define reg_cmd_2 "register lazy lazy"
#define reg_cmd_3 "quit"
static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,            expect,			modifiers,  timeout,    before,    after
    {  NULL,		INIT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  reg_cmd_1,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		REGISTER_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  reg_cmd_2,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		ERROR_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  reg_cmd_3,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		FINI_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,            EOF_EVENT,		0,          TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .fini = killall, .timeout = 30)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {LEGION_EXECUTABLE, "auto", NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}
#undef  reg_cmd_1
#undef  reg_cmd_2
#undef  reg_cmd_3
#undef TEST_NAME

/*---------------------------test unregister cmd------------------------------*/
#define SUITE tracker_suite
#define TEST_NAME regi_unregi_quit_test
#define lzy_cmd_1 "register lazy lazy"
#define lzy_cmd_2 "unregister lazy"
#define lzy_cmd_3 "quit"
static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,            expect,			modifiers,  timeout,    before,    after
    {  NULL,		INIT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  lzy_cmd_1,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		REGISTER_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  lzy_cmd_2,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		UNREGISTER_EVENT,	0,  	    HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  lzy_cmd_3,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		FINI_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,            EOF_EVENT,		0,          TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .fini = killall, .timeout = 30)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {LEGION_EXECUTABLE, "auto", NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}
#undef  lzy_cmd_1
#undef  lzy_cmd_2
#undef  lzy_cmd_3
#undef TEST_NAME

/*---------------------------test unregister unknown daemon error ------------*/
#define SUITE tracker_suite
#define TEST_NAME unreg_error_test
#define lzy_cmd_1 "unregister unknown"
#define lzy_cmd_2 "quit"
static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,            expect,			modifiers,  timeout,    before,    after
    {  NULL,		INIT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  lzy_cmd_1,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		ERROR_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  lzy_cmd_2,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		FINI_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,            EOF_EVENT,		0,          TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .fini = killall, .timeout = 30)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {LEGION_EXECUTABLE, "auto", NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}
#undef  lzy_cmd_1
#undef  lzy_cmd_2
#undef  lzy_cmd_3
#undef TEST_NAME

/*---------------------------test register and unregister multiply times------*/
#define SUITE tracker_suite
#define TEST_NAME regi_unregi_mult_test
#define reg_cmd_1 "register lazy1 lazy"
#define reg_cmd_2 "register lazy2 lazy"
#define reg_cmd_3 "register lazy3 lazy"
#define reg_cmd_4 "unregister lazy1"
#define reg_cmd_5 "unregister lazy2"
#define reg_cmd_6 "unregister lazy3"
#define reg_cmd_7 "quit"
static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,            expect,			modifiers,  timeout,    before,    after
    {  NULL,		INIT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  reg_cmd_1,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		REGISTER_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  reg_cmd_2,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		REGISTER_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  reg_cmd_3,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		REGISTER_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  reg_cmd_4,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		UNREGISTER_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  reg_cmd_5,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		UNREGISTER_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  reg_cmd_6,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		UNREGISTER_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  reg_cmd_7,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		FINI_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,            EOF_EVENT,		0,          TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .fini = killall, .timeout = 30)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {LEGION_EXECUTABLE, "auto", NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}
#undef TEST_NAME


/*---------------------------test start and quit cmd--------------------------*/
#define SUITE tracker_suite
#define TEST_NAME start_quit_test
#define lzy_cmd_1 "register lazy lazy"
#define lzy_cmd_2 "start lazy"
#define lzy_cmd_3 "quit"
static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,            expect,			modifiers,  timeout,    before,    after
    {  NULL,		INIT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  lzy_cmd_1,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		REGISTER_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  lzy_cmd_2,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		START_EVENT,		0,          ONE_SEC,	NULL,      NULL },
    {  NULL,		ACTIVE_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  lzy_cmd_3,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		STOP_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		TERM_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		FINI_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,            EOF_EVENT,		0,          TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .fini = killall, .timeout = 30)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {LEGION_EXECUTABLE, "auto", NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}
#undef  lzy_cmd_1
#undef  lzy_cmd_2
#undef  lzy_cmd_3
#undef TEST_NAME

/*---------------------------test start cmd error-----------------------------*/
#define SUITE tracker_suite
#define TEST_NAME start_error_test
#define err_cmd_1 "start unknown"
#define err_cmd_2 "quit"
static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,            expect,			modifiers,  timeout,    before,    after
    {  NULL,		INIT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  err_cmd_1,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		ERROR_EVENT,		0,          ONE_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  err_cmd_2,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		FINI_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,            EOF_EVENT,		0,          TEN_MSEC,   NULL,      NULL }
};


Test(SUITE, TEST_NAME, .fini = killall, .timeout = 30)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {LEGION_EXECUTABLE, "auto", NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}
#undef  err_cmd_1
#undef  err_cmd_2
#undef TEST_NAME

/*---------------------------test stop cmd on lazy----------------------------*/
#define SUITE tracker_suite
#define TEST_NAME start_stop_lazy_test
#define lzy_cmd_1 "register lazy lazy"
#define lzy_cmd_2 "start lazy"
#define lzy_cmd_3 "stop lazy"
#define lzy_cmd_4 "quit"
static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,            expect,			modifiers,  timeout,    before,    after
    {  NULL,		INIT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  lzy_cmd_1,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		REGISTER_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  lzy_cmd_2,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		START_EVENT,		0,          ONE_SEC,	NULL,      NULL },
    {  NULL,		ACTIVE_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  lzy_cmd_3,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		STOP_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		TERM_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  lzy_cmd_4,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		FINI_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,            EOF_EVENT,		0,          TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .fini = killall, .timeout = 30)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {LEGION_EXECUTABLE, "auto", NULL};
    struct timeval t1, t2;
    double elapsedTime;

    gettimeofday(&t1, NULL);
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    gettimeofday(&t2, NULL);

    // compute and print the elapsed time in millisec
    elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
    elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
    printf("exec %f ms.\n", elapsedTime);
    assert_proper_exit_status(err, status);
}
#undef  lzy_cmd_1
#undef  lzy_cmd_2
#undef  lzy_cmd_3
#undef  lzy_cmd_4
#undef TEST_NAME

/*---------------------------test stop cmd error------------------------------*/
#define SUITE tracker_suite
#define TEST_NAME stop_error_test
#define lzy_cmd_1 "stop unknown"
#define lzy_cmd_2 "quit"
static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,            expect,			modifiers,  timeout,    before,    after
    {  NULL,		INIT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  lzy_cmd_1,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		ERROR_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  lzy_cmd_2,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		FINI_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,            EOF_EVENT,		0,          TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .fini = killall, .timeout = 30)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {LEGION_EXECUTABLE, "auto", NULL};
    struct timeval t1, t2;
    double elapsedTime;

    gettimeofday(&t1, NULL);
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    gettimeofday(&t2, NULL);

    // compute and print the elapsed time in millisec
    elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
    elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
    printf("exec %f ms.\n", elapsedTime);
    assert_proper_exit_status(err, status);
}
#undef  lzy_cmd_1
#undef  lzy_cmd_2
#undef TEST_NAME

/*---------------------------test start stop cmd on systat--------------------*/
#define SUITE tracker_suite
#define TEST_NAME systat_start_end_test
#define sys_cmd_4 "register systat systat"
#define sys_cmd_5 "start systat"
#define sys_cmd_6 "stop systat"
#define sys_cmd_7 "quit"
static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,            expect,			modifiers,  timeout,    before,    after
    {  NULL,		INIT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  sys_cmd_4,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		REGISTER_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  sys_cmd_5,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		START_EVENT,		0,          ONE_SEC,	NULL,      NULL },
    {  NULL,		ACTIVE_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  sys_cmd_6,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		STOP_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		TERM_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  sys_cmd_7,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		FINI_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,            EOF_EVENT,		0,          TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .fini = killall, .timeout = 30)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {LEGION_EXECUTABLE, "auto", NULL};
    struct timeval t1, t2;
    double elapsedTime;

    gettimeofday(&t1, NULL);
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    gettimeofday(&t2, NULL);

    // compute and print the elapsed time in millisec
    elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
    elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
    printf("exec %f ms.\n", elapsedTime);
    assert_proper_exit_status(err, status);
}
#undef  sys_cmd_4
#undef  sys_cmd_5
#undef  sys_cmd_6
#undef  sys_cmd_7
#undef TEST_NAME

/*---------------------------test start stop multiple times-------------------*/
#define SUITE tracker_suite
#define TEST_NAME lazy_start_stop_multi_test
#define mul_cmd_0 "register lazy lazy"
#define mul_cmd_1 "start lazy"
#define mul_cmd_2 "stop lazy"
#define mul_cmd_3 "start lazy"
#define mul_cmd_4 "stop lazy"
#define mul_cmd_5 "start lazy"
#define mul_cmd_6 "stop lazy"
#define mul_cmd_7 "quit"
static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,            expect,			modifiers,  timeout,    before,    after
    {  NULL,		INIT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  mul_cmd_0,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		REGISTER_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  mul_cmd_1,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		START_EVENT,		0,          ONE_SEC,	NULL,      NULL },
    {  NULL,		ACTIVE_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  mul_cmd_2,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		STOP_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		TERM_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  mul_cmd_2,	NO_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		RESET_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  mul_cmd_3,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		START_EVENT,		0,          ONE_SEC,	NULL,      NULL },
    {  NULL,		ACTIVE_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  mul_cmd_4,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		STOP_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		TERM_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  mul_cmd_4,	NO_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		RESET_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  mul_cmd_5,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		START_EVENT,		0,          ONE_SEC,	NULL,      NULL },
    {  NULL,		ACTIVE_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  mul_cmd_6,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		STOP_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		TERM_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  mul_cmd_6,	NO_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		RESET_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  mul_cmd_7,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		FINI_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,            EOF_EVENT,		0,          TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .fini = killall, .timeout = 30)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {LEGION_EXECUTABLE, "auto", NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}
#undef  mul_cmd_0
#undef  mul_cmd_1
#undef  mul_cmd_2
#undef  mul_cmd_3
#undef  mul_cmd_4
#undef  mul_cmd_5
#undef  mul_cmd_6
#undef  mul_cmd_7
#undef TEST_NAME

/*---------------------------test start stop multiply daemons-----------------*/
#define SUITE tracker_suite
#define TEST_NAME multi_lazy_test
#define mul_cmd_0 "register lazy1 lazy"
#define mul_cmd_1 "register lazy2 lazy"
#define mul_cmd_2 "register lazy3 lazy"
#define mul_cmd_3 "start lazy1"
#define mul_cmd_4 "start lazy2"
#define mul_cmd_5 "start lazy3"
#define mul_cmd_6 "stop lazy1"
#define mul_cmd_7 "stop lazy2"
#define mul_cmd_8 "stop lazy3"
#define mul_cmd_9 "quit"
static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,            expect,			modifiers,  timeout,    before,    after
    {  NULL,		INIT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  mul_cmd_0,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		REGISTER_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  mul_cmd_1,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		REGISTER_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  mul_cmd_2,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		REGISTER_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  mul_cmd_3,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		START_EVENT,		0,          ONE_SEC,	NULL,      NULL },
    {  NULL,		ACTIVE_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  mul_cmd_4,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		START_EVENT,		0,          ONE_SEC,	NULL,      NULL },
    {  NULL,		ACTIVE_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  mul_cmd_5,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		START_EVENT,		0,          ONE_SEC,	NULL,      NULL },
    {  NULL,		ACTIVE_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  mul_cmd_6,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		STOP_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		TERM_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  mul_cmd_7,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		STOP_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		TERM_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  mul_cmd_8,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		STOP_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		TERM_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  mul_cmd_9,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		FINI_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,            EOF_EVENT,		0,          TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .fini = killall, .timeout = 30)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {LEGION_EXECUTABLE, "auto", NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}
#undef  mul_cmd_0
#undef  mul_cmd_1
#undef  mul_cmd_2
#undef  mul_cmd_3
#undef  mul_cmd_4
#undef  mul_cmd_5
#undef  mul_cmd_6
#undef  mul_cmd_7
#undef  mul_cmd_8
#undef  mul_cmd_9
#undef TEST_NAME

/*---------------------------test start stop between lazy and systat----------*/
#define SUITE tracker_suite
#define TEST_NAME two_start_end_test
#define two_cmd_1 "register lazy lazy"
#define two_cmd_2 "register systat systat"
#define two_cmd_3 "start lazy"
#define two_cmd_4 "start systat"
#define two_cmd_5 "stop lazy"
#define two_cmd_6 "stop systat"
#define two_cmd_7 "quit"
static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,            expect,			modifiers,  timeout,    before,    after
    {  NULL,		INIT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  two_cmd_1,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		REGISTER_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  two_cmd_2,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		REGISTER_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  two_cmd_3,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		START_EVENT,		0,          ONE_SEC,	NULL,      NULL },
    {  NULL,		ACTIVE_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  two_cmd_4,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		START_EVENT,		0,          ONE_SEC,	NULL,      NULL },
    {  NULL,		ACTIVE_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  two_cmd_5,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		STOP_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		TERM_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  two_cmd_6,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		STOP_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		TERM_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  two_cmd_7,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		FINI_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,            EOF_EVENT,		0,          TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .fini = killall, .timeout = 30)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {LEGION_EXECUTABLE, "auto", NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}
#undef  two_cmd_1
#undef  two_cmd_2
#undef  two_cmd_3
#undef  two_cmd_4
#undef  two_cmd_5
#undef  two_cmd_6
#undef  two_cmd_7
#undef TEST_NAME

/*---------------------------test status cmd ---------------------------------*/
#define SUITE tracker_suite
#define TEST_NAME status_test
#define sta_cmd_1 "register lazy lazy"
#define sta_cmd_2 "start lazy"
#define sta_cmd_3 "status lazy"
#define sta_cmd_4 "quit"
static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,            expect,			modifiers,  timeout,    before,    after
    {  NULL,		INIT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  sta_cmd_1,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		REGISTER_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  sta_cmd_2,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		START_EVENT,		0,          ONE_SEC,	NULL,      NULL },
    {  NULL,		ACTIVE_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  sta_cmd_3,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL }, // expect no error event
    {  NULL,		STATUS_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  sta_cmd_4,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		STOP_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		TERM_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		FINI_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,            EOF_EVENT,		0,          TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .fini = killall, .timeout = 30)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {LEGION_EXECUTABLE, "auto", NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}
#undef  sta_cmd_1
#undef  sta_cmd_2
#undef  sta_cmd_3
#undef  sta_cmd_4
#undef TEST_NAME

/*---------------------------test status cmd error----------------------------*/
#define SUITE tracker_suite
#define TEST_NAME status_error_test
#define sta_cmd_1 "status unknown"
#define sta_cmd_2 "quit"
static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,            expect,			modifiers,  timeout,    before,    after
    {  NULL,		INIT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  sta_cmd_1,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		ERROR_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  sta_cmd_2,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		FINI_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,            EOF_EVENT,		0,          TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .fini = killall, .timeout = 30)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {LEGION_EXECUTABLE, "auto", NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}
#undef  sta_cmd_1
#undef  sta_cmd_2
#undef TEST_NAME

/*-----------------test status-all cmd,---------------------------------------*
 * inserting status-all at any moment should not raise error_event------------*/
#define SUITE tracker_suite
#define TEST_NAME status_all_test
#define sts_cmd_0 "status-all"
#define sts_cmd_1 "register lazy1 lazy"
#define sts_cmd_2 "register lazy2 lazy"
#define sts_cmd_3 "start lazy1"
#define sts_cmd_4 "start lazy2"
#define sts_cmd_5 "stop lazy1"
#define sts_cmd_6 "stop lazy2"
#define sts_cmd_7 "quit"
static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,            expect,			modifiers,  timeout,    before,    after
    {  NULL,		INIT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  sts_cmd_0,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  sts_cmd_1,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		REGISTER_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  sts_cmd_2,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		REGISTER_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  sts_cmd_0,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		STATUS_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		STATUS_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  sts_cmd_3,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		START_EVENT,		0,          ONE_SEC,	NULL,      NULL },
    {  NULL,		ACTIVE_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  sts_cmd_0,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		STATUS_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		STATUS_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  sts_cmd_4,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		START_EVENT,		0,          ONE_SEC,	NULL,      NULL },
    {  NULL,		ACTIVE_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  sts_cmd_0,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		STATUS_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		STATUS_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  sts_cmd_5,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		STOP_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		TERM_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  sts_cmd_0,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		STATUS_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		STATUS_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  sts_cmd_6,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		STOP_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		TERM_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  sts_cmd_0,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		STATUS_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		STATUS_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  sts_cmd_7,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		FINI_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,            EOF_EVENT,		0,          TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .fini = killall, .timeout = 30)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {LEGION_EXECUTABLE, "auto", NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}
#undef  sts_cmd_0
#undef  sts_cmd_1
#undef  sts_cmd_2
#undef  sts_cmd_3
#undef  sts_cmd_4
#undef  sts_cmd_5
#undef  sts_cmd_6
#undef  sts_cmd_7
#undef TEST_NAME

/*---------------------------test logrotate cmd-------------------------------*/
#define SUITE tracker_suite
#define TEST_NAME logrotate_lazy_test
#define log_cmd_0 "register lazy lazy"
#define log_cmd_1 "start lazy"
#define log_cmd_2 "logrotate lazy"
#define log_cmd_3 "stop lazy"
#define log_cmd_4 "quit"
static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,            expect,			modifiers,  timeout,    before,    after
    {  NULL,		INIT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  log_cmd_0,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		REGISTER_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  log_cmd_1,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		START_EVENT,		0,          ONE_SEC,	NULL,      NULL },
    {  NULL,		ACTIVE_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  log_cmd_2,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		LOGROTATE_EVENT,	EXPECT_SKIP_OTHER, TWO_SEC,	NULL,      NULL },
    {  NULL,		START_EVENT,		EXPECT_SKIP_OTHER, ONE_SEC,	NULL,      NULL },
    {  NULL,		ACTIVE_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  log_cmd_3,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		STOP_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		TERM_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  log_cmd_4,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		FINI_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,            EOF_EVENT,		0,          TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .fini = killall, .timeout = 30)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {LEGION_EXECUTABLE, "auto", NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}
#undef  log_cmd_0
#undef  log_cmd_1
#undef  log_cmd_2
#undef  log_cmd_3
#undef  log_cmd_4
#undef TEST_NAME

/*---------------------------test logrotate cmd error-------------------------*/
#define SUITE tracker_suite
#define TEST_NAME logrotate_error_test
#define log_cmd_1 "logrotate unknown"
#define log_cmd_2 "quit"
static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,            expect,			modifiers,  timeout,    before,    after
    {  NULL,		INIT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  log_cmd_1,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		ERROR_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  log_cmd_2,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		FINI_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,            EOF_EVENT,		0,          TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .fini = killall, .timeout = 30)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {LEGION_EXECUTABLE, "auto", NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}
#undef  log_cmd_1
#undef  log_cmd_2
#undef TEST_NAME

/*---------------------------test logrotate between two daemons---------------*/
#define SUITE tracker_suite
#define TEST_NAME logrotate_two_lazy_test
#define two_log_cmd_0 "register lazy1 lazy"
#define two_log_cmd_1 "register lazy2 lazy"
#define two_log_cmd_2 "start lazy1"
#define two_log_cmd_3 "logrotate lazy1"
#define two_log_cmd_4 "stop lazy1"
#define two_log_cmd_5 "start lazy2"
#define two_log_cmd_6 "logrotate lazy2"
#define two_log_cmd_7 "stop lazy2"
#define two_log_cmd_8 "quit"
static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,            expect,			modifiers,  timeout,    before,    after
    {  NULL,		INIT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  two_log_cmd_0,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		REGISTER_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  two_log_cmd_1,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		REGISTER_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  two_log_cmd_2,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		START_EVENT,		0,          ONE_SEC,	NULL,      NULL },
    {  NULL,		ACTIVE_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  two_log_cmd_3,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		LOGROTATE_EVENT,	EXPECT_SKIP_OTHER, TWO_SEC,	NULL,      NULL },
    {  NULL,		START_EVENT,		EXPECT_SKIP_OTHER, ONE_SEC,	NULL,      NULL },
    {  NULL,		ACTIVE_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  two_log_cmd_4,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		STOP_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		TERM_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  two_log_cmd_5,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		START_EVENT,		0,          ONE_SEC,	NULL,      NULL },
    {  NULL,		ACTIVE_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  two_log_cmd_6,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		LOGROTATE_EVENT,	EXPECT_SKIP_OTHER, TWO_SEC,	NULL,      NULL },
    {  NULL,		START_EVENT,		EXPECT_SKIP_OTHER, ONE_SEC,	NULL,      NULL },
    {  NULL,		ACTIVE_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  two_log_cmd_7,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		STOP_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		TERM_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  two_log_cmd_8,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		FINI_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,            EOF_EVENT,		0,          TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .fini = killall, .timeout = 30)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {LEGION_EXECUTABLE, "auto", NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}
#undef  two_log_cmd_0
#undef  two_log_cmd_1
#undef  two_log_cmd_2
#undef  two_log_cmd_3
#undef  two_log_cmd_4
#undef  two_log_cmd_5
#undef  two_log_cmd_6
#undef  two_log_cmd_7
#undef  two_log_cmd_8
#undef TEST_NAME

/*---------------------------test crash daemon--------------------------------*/
#define SUITE tracker_suite
#define TEST_NAME crash_test
#define cra_cmd_1 "register crash crash"
#define cra_cmd_2 "start crash"
#define cra_cmd_3 "stop crash" // crash wouldn't handle SYSTERM as expect, cause legion to timeout, and sent SYSKILL
#define cra_cmd_4 "quit"
static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,            expect,			modifiers,  timeout,    before,    after
    {  NULL,		INIT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  cra_cmd_1,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		REGISTER_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  cra_cmd_2,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		START_EVENT,		0,          ONE_SEC,	NULL,      NULL },
    {  NULL,		ACTIVE_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  cra_cmd_3,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		STOP_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    // There should be an ERROR_EVENT somewhere in here, too, but order is unspecified.
    {  NULL,		KILL_EVENT,		EXPECT_SKIP_OTHER,          TWO_SEC,	NULL,      NULL },
    // I think if the CRASH_EVENT occurs after the prompt, something funny is going on.
    {  NULL,		CRASH_EVENT,		EXPECT_SKIP_OTHER,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		EXPECT_SKIP_OTHER,          HND_MSEC,   NULL,      NULL },
    {  cra_cmd_4,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		FINI_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,            EOF_EVENT,		0,          TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .fini = killall, .timeout = 30)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {LEGION_EXECUTABLE, "auto", NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}
#undef  cra_cmd_1
#undef  cra_cmd_2
#undef  cra_cmd_3
#undef  cra_cmd_4
#undef TEST_NAME

/*---------------------------test RESET_EVENT via crash daemon----------------*/
#define SUITE tracker_suite
#define TEST_NAME crash_reset_test
#define crt_cmd_1 "register crash crash"
#define crt_cmd_2 "start crash"
#define crt_cmd_3 "stop crash" // crash wouldn't handle SYSTERM as expect, cause legion to timeout, and sent SYSKILL
#define crt_cmd_4 "stop crash" // crash proc turn from crash to inactive, expect reset_event
#define crt_cmd_5 "quit"
static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,            expect,			modifiers,  timeout,    before,    after
    {  NULL,		INIT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  crt_cmd_1,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		REGISTER_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  crt_cmd_2,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		START_EVENT,		0,          ONE_SEC,	NULL,      NULL },
    {  NULL,		ACTIVE_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  crt_cmd_3,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		STOP_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    // There should be an ERROR_EVENT somewhere in here, too, but order is unspecified.
    {  NULL,		KILL_EVENT,		EXPECT_SKIP_OTHER,          TWO_SEC,	NULL,      NULL },
    // I think if the CRASH_EVENT occurs after the prompt, something funny is going on.
    {  NULL,		CRASH_EVENT,		EXPECT_SKIP_OTHER,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		EXPECT_SKIP_OTHER,          HND_MSEC,   NULL,      NULL },
    {  crt_cmd_4,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		RESET_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  crt_cmd_5,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		FINI_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,            EOF_EVENT,		0,          TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .fini = killall, .timeout = 30)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {LEGION_EXECUTABLE, "auto", NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}
#undef  crt_cmd_1
#undef  crt_cmd_2
#undef  crt_cmd_3
#undef  crt_cmd_4
#undef  crt_cmd_5
#undef TEST_NAME

/*---------------------------test nosync daemon-------------------------------*/
#define SUITE tracker_suite
#define TEST_NAME nosync_test
#define syn_cmd_1 "register nosync nosync"
#define syn_cmd_2 "start nosync" // nosync wouldn't sync with legion as expected, cause legion to timeout, and send SYSKILL, and start failed
#define syn_cmd_3 "quit"
static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,            expect,			modifiers,  timeout,    before,    after
    {  NULL,		INIT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  syn_cmd_1,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		REGISTER_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		PROMPT_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  syn_cmd_2,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    // The specification did not clearly state that a KILL_EVENT should occur here.
    //{  NULL,		KILL_EVENT,		0,          TWO_SEC,	NULL,      NULL },
    // There should be an ERROR_EVENT, but the order is unspecified
    {  NULL,		CRASH_EVENT,		EXPECT_SKIP_OTHER,          TWO_SEC,	NULL,      NULL },
    // I think if the CRASH_EVENT occurs after the prompt, something funny is going on.
    {  NULL,		PROMPT_EVENT,		EXPECT_SKIP_OTHER,          HND_MSEC,   NULL,      NULL },
    {  syn_cmd_3,	NO_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,		FINI_EVENT,		0,          HND_MSEC,   NULL,      NULL },
    {  NULL,            EOF_EVENT,		0,          TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .fini = killall, .timeout = 30)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {LEGION_EXECUTABLE, "auto", NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}
#undef  syn_cmd_1
#undef  syn_cmd_2
#undef  syn_cmd_3
#undef TEST_NAME

#if 0 /* COMMENT_2 */
#endif /* COMMENT_2 */
