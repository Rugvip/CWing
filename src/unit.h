
#include <stdlib.h>
#include <string.h>

#ifndef TEST_PREFIX
#define TEST_PREFIX _unit_test_
#endif

#define _PASTE(a, b) a##b
#define PASTE(a, b) _PASTE(a, b)

#define STRINGIFY(s) STRINGIFY_(s)
#define STRINGIFY_(s) #s

#define TEST_CONTEXT_PREFIX PASTE(TEST_PREFIX, context_)

#define COMPL(b) _PASTE(COMPL_, b)
#define COMPL_0 1
#define COMPL_1 0

#define DEC(x) _PASTE(DEC_, x)
#define DEC_0 0
#define DEC_1 0
#define DEC_2 1
#define DEC_3 2
#define DEC_4 3
#define DEC_5 4
#define DEC_6 5
#define DEC_7 6
#define DEC_8 7
#define DEC_9 8
#define DEC_10 9
#define DEC_11 10
#define DEC_12 11
#define DEC_13 12
#define DEC_14 13
#define DEC_15 14
#define DEC_16 15
#define DEC_17 16
#define DEC_18 17
#define DEC_19 18
#define DEC_20 19

#define EVAL(...)  EVAL1(EVAL1(EVAL1(__VA_ARGS__)))
#define EVAL1(...) EVAL2(EVAL2(EVAL2(__VA_ARGS__)))
#define EVAL2(...) EVAL3(EVAL3(EVAL3(__VA_ARGS__)))
#define EVAL3(...) EVAL4(EVAL4(EVAL4(__VA_ARGS__)))
#define EVAL4(...) EVAL5(EVAL5(EVAL5(__VA_ARGS__)))
#define EVAL5(...) __VA_ARGS__

#define _MAP_END(...)

#define _MAP_DONE
#define _MAP_GET_END() 0, _MAP_END
#define _MAP_NEXT0(item, next, ...) next _MAP_DONE
#define _MAP_NEXT1(item, next) _MAP_NEXT0(item, next, 0)
#define _MAP_NEXT(item, next)  _MAP_NEXT1(_MAP_GET_END item, next)

#define MAP_ARGS_0(macro, x, first, ...) macro(x)_MAP_NEXT(first, MAP_ARGS_1)(macro, first, __VA_ARGS__)
#define MAP_ARGS_1(macro, x, first, ...) ,macro(x)_MAP_NEXT(first, MAP_ARGS_2)(macro, first, __VA_ARGS__)
#define MAP_ARGS_2(macro, x, first, ...) ,macro(x)_MAP_NEXT(first, MAP_ARGS_1)(macro, first, __VA_ARGS__)
#define MAP_1(macro, x, first, ...) macro(x)_MAP_NEXT(first, MAP_2)(macro, first, __VA_ARGS__)
#define MAP_2(macro, x, first, ...) macro(x)_MAP_NEXT(first, MAP_1)(macro, first, __VA_ARGS__)
#define MAP_ARGS(macro, ...) EVAL(MAP_ARGS_0(macro, __VA_ARGS__, (), 0))
#define MAP(macro, ...) EVAL(MAP_1(macro, __VA_ARGS__, (), 0))

#define EMPTY()
#define DEFER(id) id EMPTY()
#define OBSTRUCT(id) id DEFER(EMPTY)()
#define EXPAND(...) __VA_ARGS__

#define CHECK_N(x, n, ...) n
#define CHECK(...) CHECK_N(__VA_ARGS__, 0,)

#define NOT(x) CHECK(_PASTE(NOT_, x))
#define NOT_0 ~, 1,

#define IIF(c) _PASTE(IIF_, c)
#define IIF_0(t, ...) __VA_ARGS__
#define IIF_1(t, ...) t

#define BOOL(x) COMPL(NOT(x))
#define IF(c) IIF(BOOL(c))

#define EAT(...)
#define EXPAND(...) __VA_ARGS__
#define WHEN(c) IF(c)(EXPAND, EAT)

#define REPEAT(count, macro, ...) \
WHEN(count)(\
    OBSTRUCT(REPEAT_INDIRECT)()(DEC(count),macro,__VA_ARGS__)\
    OBSTRUCT(macro)(DEC(count),__VA_ARGS__))
#define REPEAT_INDIRECT() REPEAT

#define CONTEXT_TYPE_NAME(name) PASTE(TEST_CONTEXT_PREFIX, PASTE(name, _t))
#define CONTEXT_TYPE_DECL(name) CONTEXT_TYPE_NAME(name) name
#define CONTEXT_TYPE_DECL_PTR(name) CONTEXT_TYPE_NAME(name) *name

#define CONTEXT_INIT_NAME(name) PASTE(TEST_CONTEXT_PREFIX, PASTE(name, _init))
#define CONTEXT_INIT_DECL(name) void CONTEXT_INIT_NAME(name)(CONTEXT_TYPE_DECL_PTR(name))
#define CONTEXT_INIT_CALL(name) CONTEXT_INIT_NAME(name)(name);
#define CONTEXT_INIT_CALL_STACK(name) CONTEXT_INIT_NAME(name)(&name);

#define CONTEXT(name, field, ...) \
typedef struct {field,##__VA_ARGS__} CONTEXT_TYPE_NAME(name); \
CONTEXT_INIT_DECL(name)

#define REFERENCE(ptr) &ptr
#define APPEND_SEMI(x) x;
#define TEST_SETUP_NAME(id) PASTE(TEST_PREFIX, id)
#define TEST_SETUP_DECL(id) void TEST_SETUP_NAME(id)()
#define TEST_RUN_NAME(name) PASTE(TEST_PREFIX, PASTE(name, _run))
#define TEST_RUN_DECL(name, ...) void TEST_RUN_NAME(name)(MAP_ARGS(CONTEXT_TYPE_DECL_PTR, __VA_ARGS__))
#define TEST_RUN_CALL_STACK(name, ...) TEST_RUN_NAME(name)(MAP_ARGS(REFERENCE, __VA_ARGS__));

#define TEST_GET_NAME_NAME(id) PASTE(PASTE(TEST_PREFIX, id), _get_name)
#define TEST_GET_NAME_DECL(id, name) \
char *TEST_GET_NAME_NAME(id)() { \
    return STRINGIFY(name); \
}

#define _TEST_INSTANCE(name, id, ...) \
TEST_GET_NAME_DECL(id, name) \
TEST_SETUP_DECL(id) { \
    PASTE(TEST_PREFIX, set_name)(STRINGIFY(name)); \
    MAP(APPEND_SEMI, MAP_ARGS(CONTEXT_TYPE_DECL, __VA_ARGS__)) \
    MAP(CONTEXT_INIT_CALL_STACK, __VA_ARGS__) \
    TEST_RUN_CALL_STACK(name, __VA_ARGS__) \
}

#define TEST(name, ...) \
TEST_RUN_DECL(name, __VA_ARGS__); \
_TEST_INSTANCE(name, __COUNTER__, __VA_ARGS__) \
TEST_RUN_DECL(name, __VA_ARGS__)

#define TEST_SETUP_NAME_ARR(id, _) {TEST_SETUP_NAME(id), TEST_GET_NAME_NAME(id)},

typedef void (*test_setup_t)();
typedef char *(*test_get_name_t)();

typedef struct {
    test_setup_t setup;
    test_get_name_t get_name;
} test_instance_t;

void PASTE(TEST_PREFIX, set_name)(char *);

#define RUN_ALL_TESTS \
test_instance_t *PASTE(TEST_PREFIX, get_all_tests)() { \
    test_instance_t tests[] = {EVAL(REPEAT(__COUNTER__, TEST_SETUP_NAME_ARR)) {(void*)0, (void*)0}}; \
    test_instance_t *ptr = malloc(sizeof(tests)); \
    memcpy(ptr, tests, sizeof(tests)); \
    return ptr; \
} static void PASTE(TEST_PREFIX, dummy_func)

#define _ASSERT_HELPER_(test, line, str, msg, ...) _test_assert(test, line, str, msg)
#define _ASSERT_HELPER(test, ...) _ASSERT_HELPER_(test, __LINE__, STRINGIFY(test), __VA_ARGS__)
#define _FAILED_HELPER(line, msg, ...) _test_failed(line, msg)

#define test_assert(...) _ASSERT_HELPER(__VA_ARGS__, NULL)
#define test_failed(...) _FAILED_HELPER(__LINE__, ##__VA_ARGS__, NULL)
#define test_successful() _test_successful()

void _test_assert(int, int, char *, char *);
void _test_failed(int, char *);
void _test_successful();
