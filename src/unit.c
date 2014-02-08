
#include "unit.h"

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

extern test_instance_t *PASTE(TEST_PREFIX, get_all_tests)();

#define FD_READ 0
#define FD_WRITE 1

static FILE *output;
static char *test_name;

void PASTE(TEST_PREFIX, set_name)(char *name)
{
    test_name = name;
}

void _test_assert(int b, int line, char *test, char *msg)
{
    if (!b) {
        if (msg) {
            fprintf(output, "%d => '%s'\n", line, msg);
        } else {
            fprintf(output, "%d => '%s'\n", line, test);
        }
        exit(EXIT_FAILURE);
    }
}

void _test_failed(int line, char *msg)
{
    if (msg) {
        fprintf(output, "%d => failed: %s\n", line, msg);
    } else {
        fprintf(output, "%d => failed\n", line);
    }
    exit(EXIT_FAILURE);
}

void _test_successful()
{
    fprintf(output, "test completed successfully\n");
    exit(EXIT_SUCCESS);
}

typedef struct {
    int pipe[2];
} ForkContext;

typedef struct {
    int status;
    char *msg;
} TestResult;

typedef struct {
    int result_pipe;
    test_setup_t setup;
    pid_t pid;
    pthread_t thread;
    char *name;
    int show_output;
} TestContext;

typedef struct {
    TestContext *tests;
    TestResult *results;
    int count;
} Context;

static void prefork(ForkContext *fc)
{
    pipe(fc->pipe);
}

static void postfork_child(ForkContext *fc, TestContext *tc)
{
    close(fc->pipe[FD_READ]);

    output = fdopen(fc->pipe[FD_WRITE], "w");

    if (!tc->show_output) {
        close(0);
        close(1);
        close(2);
    }
}

static char *read_test_result_thread(TestContext *tc)
{
    const int buf_size = 1024;
    char *buf[buf_size];
    ssize_t size;
    char *out;

    size = read(tc->result_pipe, buf, buf_size);

    out = malloc(size);
    memcpy(out, buf, size);

    return out;
}

static void postfork_parent(ForkContext *fc, TestContext *tc, pid_t pid)
{
    close(fc->pipe[FD_WRITE]);

    tc->pid = pid;
    tc->result_pipe = fc->pipe[FD_READ];

    if (pthread_create(&tc->thread, NULL, (void *(*)(void *)) read_test_result_thread, tc)) {
        fprintf(stderr, "Failed to create result read thread\n");
    }
}

static void run_test(TestContext *tc)
{
    ForkContext fc;
    pid_t pid;

    prefork(&fc);

    pid = fork();

    if (!pid) { /* child */
        postfork_child(&fc, tc);
        tc->setup();
        _test_successful();
    } else if (pid > 0) { /* parent */
        postfork_parent(&fc, tc, pid);
    } else { /* error */
        fprintf(stderr, "Fork failed\n");
    }
}

static void print_result(Context *c)
{
    int num_success = 0;
    int i;

    for (i = 0; i < c->count; ++i) {
        if (!c->results[i].status) {
            ++num_success;
        }
    }

    if (c->count == num_success) {
        printf("All tests completed successfully\n");
    } else if (num_success == 0) {
        printf("All tests failed\n");
    } else {
        printf("%d/%d tests completed sucessfully\n", num_success, c->count);
    }

    for (i = 0; i < c->count; ++i) {
        switch (c->results[i].status) {
        case 0:
            printf("%s => Successful\n", c->tests[i].name);
            break;
        case SIGSEGV:
            printf("%s => Segmentation fault\n", c->tests[i].name);
            break;
        case 256:
            printf("%s => Failed:%s",
                c->tests[i].name,
                c->results[i].msg);
            break;
        default:
            printf("%s => [%d]:%s",
                c->tests[i].name,
                c->results[i].status,
                c->results[i].msg);
            break;
        }
    }
}

static void run_all_tests(Context *c)
{
    int result_count;
    int i;
    int status;
    pid_t pid;

    for (i = 0; i < c->count; ++i) {
        run_test(&c->tests[i]);
    }

    result_count = 0;
    while (result_count < c->count) {
        pid = wait(&status);

        for (i = 0; i < c->count; ++i) {
            if (pid == c->tests[i].pid) {
                printf("Test '%s' complete\n", c->tests[i].name);
                if (pthread_join(c->tests[i].thread, (void **) &c->results[i].msg)) {
                    fprintf(stderr, "Failed to join thread");
                }
                c->results[i].status = status;
                ++result_count;
                break;
            }
        }
    }

    print_result(c);

    free(c->tests);
    free(c->results);
    free(c);
}

static int get_test_index(char const *name)
{
    test_instance_t *tests = PASTE(TEST_PREFIX, get_all_tests)();
    int i;

    for (i = 0; tests[i].get_name; ++i) {
        if (!strcmp(tests[i].get_name(), name)) {
            return i;
        }
    }

    return -1;
}

static void run_single_test(char const *name)
{
    test_instance_t *tests = PASTE(TEST_PREFIX, get_all_tests)();
    int i;

    output = stdout;

    i = get_test_index(name);

    if (i < 0) {
        printf("No test named '%s' was found\n", name);
        exit(EXIT_FAILURE);
    }

    tests[i].setup();
    _test_successful();
}

static Context *create_context(int argc, char const *argv[])
{
    Context *c = malloc(sizeof(Context));
    test_instance_t *tests = PASTE(TEST_PREFIX, get_all_tests)();
    int i;

    for (c->count = 0; tests[c->count].setup; ++(c->count));

    c->tests = calloc(c->count, sizeof(TestContext));
    c->results = calloc(c->count, sizeof(TestResult));

    for (i = 0; i < c->count; ++i) {
        c->tests[i].show_output = 0;
        c->tests[i].name = tests[i].get_name();
        c->tests[i].setup = tests[i].setup;
    }

    if (argc > 2) {
        if (strcmp(argv[1], "-")) {
            printf("Invalid arguments\n");

            return NULL;
        } else {
            int id;

            for (i = 2; i < argc; ++i) {
                id = get_test_index(argv[i]);

                if (id < 0) {
                    printf("No test named '%s' was found\n", argv[i]);
                } else {
                    c->tests[id].show_output = 1;
                }
            }
        }
    }

    return c;
}

int main(int argc, char const *argv[])
{
    if (argc == 2) {
        run_single_test(argv[1]);
    } else {
        Context *c = create_context(argc, argv);
        if (c) {
            run_all_tests(c);
        }
    }

    return EXIT_SUCCESS;
}
