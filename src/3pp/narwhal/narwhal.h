#ifndef NARWHAL_H
#define NARWHAL_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST(name)                                              \
    void name(void);                                            \
    void name ## _before_fork() {}                              \
    static struct narwhal_test_t narwhal_test_ ## name = {      \
        .name_p = #name,                                        \
        .func = name,                                           \
        .before_fork_func = name ## _before_fork,               \
        .next_p = NULL                                          \
    };                                                          \
    __attribute__ ((constructor))                               \
    void narwhal_constructor_ ## name(void)                     \
    {                                                           \
        narwhal_register_test(&narwhal_test_ ## name);          \
    }                                                           \
    void name(void)

#define NARWHAL_TEST_FAILURE(message_p)                 \
    narwhal_test_failure(__FILE__, __LINE__, message_p)

#define NARWHAL_PRINT_FORMAT(value)                     \
    _Generic((value),                                   \
             char: "%c",                                \
             const char: "%c",                          \
             signed char: "%hhd",                       \
             const signed char: "%hhd",                 \
             unsigned char: "%hhu",                     \
             const unsigned char: "%hhu",               \
             signed short: "%hd",                       \
             const signed short: "%hd",                 \
             unsigned short: "%hu",                     \
             const unsigned short: "%hu",               \
             signed int: "%d",                          \
             const signed int: "%d",                    \
             unsigned int: "%u",                        \
             const unsigned int: "%u",                  \
             long int: "%ld",                           \
             const long int: "%ld",                     \
             unsigned long int: "%lu",                  \
             const unsigned long int: "%lu",            \
             long long int: "%lld",                     \
             const long long int: "%lld",               \
             unsigned long long int: "%llu",            \
             const unsigned long long int: "%llu",      \
             float: "%f",                               \
             const float: "%f",                         \
             double: "%f",                              \
             const double: "%f",                        \
             long double: "%Lf",                        \
             const long double: "%Lf",                  \
             char *: "\"%s\"",                          \
             const char *: "\"%s\"",                    \
             bool: "%d",                                \
    default: "%p")

#define NARWHAL_BINARY_ASSERTION(left, right, check, format, formatter) \
    if (!check((left), (right))) {                                      \
        char _narwhal_assert_format[512];                               \
                                                                        \
        snprintf(&_narwhal_assert_format[0],                            \
                 sizeof(_narwhal_assert_format),                        \
                 format,                                                \
                 NARWHAL_PRINT_FORMAT(left),                            \
                 NARWHAL_PRINT_FORMAT(right));                          \
        NARWHAL_TEST_FAILURE(formatter(_narwhal_assert_format,          \
                                       (left),                          \
                                       (right)));                       \
    }                                                                   \

#define NARWHAL_CHECK_EQ(left, right)                   \
    _Generic(                                           \
        (left),                                         \
         char *: _Generic(                              \
             (right),                                   \
             char *: narwhal_check_string_equal(        \
                 (char *)(uintptr_t)(left),             \
                 (char *)(uintptr_t)(right)),           \
             const char *: narwhal_check_string_equal(  \
                 (char *)(uintptr_t)(left),             \
                 (char *)(uintptr_t)(right)),           \
             default: false),                           \
        const char *: _Generic(                         \
            (right),                                    \
            char *: narwhal_check_string_equal(         \
                (char *)(uintptr_t)(left),              \
                (char *)(uintptr_t)(right)),            \
            const char *: narwhal_check_string_equal(   \
                (char *)(uintptr_t)(left),              \
                (char *)(uintptr_t)(right)),            \
            default: false),                            \
        default: (left) == (right))

#define NARWHAL_CHECK_NE(left, right)                           \
    _Generic(                                                   \
        (left),                                                 \
        char *: _Generic(                                       \
            (right),                                            \
            char *: (!narwhal_check_string_equal(               \
                         (char *)(uintptr_t)(left),             \
                         (char *)(uintptr_t)(right))),          \
            const char *: (!narwhal_check_string_equal(         \
                               (char *)(uintptr_t)(left),       \
                               (char *)(uintptr_t)(right))),    \
            default: true),                                     \
        const char *: _Generic(                                 \
            (right),                                            \
            char *: (!narwhal_check_string_equal(               \
                         (char *)(uintptr_t)(left),             \
                         (char *)(uintptr_t)(right))),          \
            const char *: (!narwhal_check_string_equal(         \
                               (char *)(uintptr_t)(left),       \
                               (char *)(uintptr_t)(right))),    \
            default: true),                                     \
        default: (left) != (right))

#define NARWHAL_CHECK_LT(left, right) ((left) < (right))

#define NARWHAL_CHECK_LE(left, right) ((left) <= (right))

#define NARWHAL_CHECK_GT(left, right) ((left) > (right))

#define NARWHAL_CHECK_GE(left, right) ((left) >= (right))

#define NARWHAL_CHECK_SUBSTRING(left, right)    \
    narwhal_check_substring(left, right)

#define NARWHAL_CHECK_NOT_SUBSTRING(left, right)        \
    (!narwhal_check_substring(left, right))

#define NARWHAL_FORMAT_EQ(format, left, right)                  \
    _Generic(                                                   \
        (left),                                                 \
        char *: _Generic(                                       \
            (right),                                            \
            char *: narwhal_format_string(                      \
                format,                                         \
                (char *)(uintptr_t)(left),                      \
                (char *)(uintptr_t)(right)),                    \
            const char *: narwhal_format_string(                \
                format,                                         \
                (char *)(uintptr_t)(left),                      \
                (char *)(uintptr_t)(right)),                    \
        default: narwhal_format(format, (left), (right))),      \
        const char *: _Generic(                                 \
            (right),                                            \
            char *: narwhal_format_string(                      \
                format,                                         \
                (char *)(uintptr_t)(left),                      \
                (char *)(uintptr_t)(right)),                    \
            const char *: narwhal_format_string(                \
                format,                                         \
                (char *)(uintptr_t)(left),                      \
                (char *)(uintptr_t)(right)),                    \
        default: narwhal_format(format, (left), (right))),      \
    default: narwhal_format(format, (left), (right)))

#define ASSERT_EQ(left, right)                                          \
    NARWHAL_BINARY_ASSERTION(left,                                      \
                             right,                                     \
                             NARWHAL_CHECK_EQ,                          \
                             "First argument %s is not equal to %s.\n", \
                             NARWHAL_FORMAT_EQ)

#define ASSERT_NE(left, right)                                          \
    NARWHAL_BINARY_ASSERTION(left,                                      \
                             right,                                     \
                             NARWHAL_CHECK_NE,                          \
                             "First argument %s is not different from %s.\n", \
                             narwhal_format)

#define ASSERT_LT(left, right)                                          \
    NARWHAL_BINARY_ASSERTION(left,                                      \
                             right,                                     \
                             NARWHAL_CHECK_LT,                          \
                             "First argument %s is not less than %s.\n", \
                             narwhal_format)

#define ASSERT_LE(left, right)                                          \
    NARWHAL_BINARY_ASSERTION(left,                                      \
                             right,                                     \
                             NARWHAL_CHECK_LE,                          \
                             "First argument %s is not less than or equal to %s.\n", \
                             narwhal_format)

#define ASSERT_GT(left, right)                                          \
    NARWHAL_BINARY_ASSERTION(left,                                      \
                             right,                                     \
                             NARWHAL_CHECK_GT,                          \
                             "First argument %s is not greater than %s.\n", \
                             narwhal_format)

#define ASSERT_GE(left, right)                                          \
    NARWHAL_BINARY_ASSERTION(left,                                      \
                             right,                                     \
                             NARWHAL_CHECK_GE,                          \
                             "First argument %s is not greater or equal to %s.\n", \
                             narwhal_format)

#define ASSERT_SUBSTRING(string, substring)                             \
    NARWHAL_BINARY_ASSERTION(string,                                    \
                             substring,                                 \
                             NARWHAL_CHECK_SUBSTRING,                   \
                             "First argument %s doesn't contain %s.\n", \
                             narwhal_format)

#define ASSERT_NOT_SUBSTRING(string, substring)                         \
    NARWHAL_BINARY_ASSERTION(string,                                    \
                             substring,                                 \
                             NARWHAL_CHECK_NOT_SUBSTRING,               \
                             "First argument %s contains %s.\n",        \
                             narwhal_format)

#define ASSERT_MEMORY(left, right, size)                                \
    do {                                                                \
        if (memcmp((left), (right), (size)) != 0) {                     \
            NARWHAL_TEST_FAILURE(narwhal_format_memory((left),          \
                                                       (right),         \
                                                       (size)));        \
        }                                                               \
    } while (0)

#define ASSERT(cond)                                    \
    if (!(cond)) {                                      \
        NARWHAL_TEST_FAILURE(strdup("Assert.\n"));      \
    }

#define FAIL() NARWHAL_TEST_FAILURE(strdup("Fail.\n"))

#define CAPTURE_OUTPUT(stdout_name, stderr_name)                        \
    int stdout_name ## i;                                               \
    static char *stdout_name = NULL;                                    \
    static char *stderr_name = NULL;                                    \
                                                                        \
    for (stdout_name ## i = 0, narwhal_capture_output_start(&stdout_name, \
                                                            &stderr_name); \
         stdout_name ## i < 1;                                          \
         stdout_name ## i++, narwhal_capture_output_stop())

struct narwhal_test_t {
    const char *name_p;
    void (*func)(void);
    void (*before_fork_func)(void);
    int exit_code;
    int signal_number;
    float elapsed_time_ms;
    struct narwhal_test_t *next_p;
};

bool narwhal_check_string_equal(const char *actual_p, const char *expected_p);

const char *narwhal_format(const char *format_p, ...);

const char *narwhal_format_string(const char *format_p, ...);

const char *narwhal_format_memory(const void *left_p,
                                  const void *right_p,
                                  size_t size);

bool narwhal_check_substring(const char *actual_p, const char *expected_p);

void narwhal_capture_output_start(char **stdout_pp, char **stderr_pp);

void narwhal_capture_output_stop(void);

__attribute__ ((noreturn)) void narwhal_test_failure(const char *file_p,
                                                     int line,
                                                     const char *message_p);

void narwhal_register_test(struct narwhal_test_t *test_p);

int narwhal_run_tests(void);

#endif
