#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <ctype.h>
#include <errno.h>
#include <sys/time.h>
// #include "subprocess.h"
/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Erik Moqvist
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * This file is part of the subprocess project.
 */

#include <string.h>
#include <stdbool.h>

#define NARWHAL_SUBPROCESS_VERSION "0.3.0"

typedef void (*narwhal_subprocess_entry_t)(void *arg_p);

struct narwhal_subprocess_output_t {
    /* Always null-terminated. */
    char *buf_p;
    /* Buffer length, not incuding last null-termination. */
    size_t length;
    /* Buffer size, including unused bytes. */
    size_t size;
};

struct narwhal_subprocess_result_t {
    int exit_code;
    int signal_number;
    struct narwhal_subprocess_output_t stdout;
    struct narwhal_subprocess_output_t stderr;
};

/**
 * Call given function with given argument in a subprocess. Returns
 * captured subprocess' exit code, or NULL if the subprocess could not
 * be started.
 */
struct narwhal_subprocess_result_t *narwhal_subprocess_call(narwhal_subprocess_entry_t entry,
                                            void *arg_p);

/**
 * Call given function with given argument in a subprocess. Returns
 * captured subprocess' stdout, stderr and exit code, or NULL if the
 * subprocess could not be started.
 */
struct narwhal_subprocess_result_t *narwhal_subprocess_call_output(narwhal_subprocess_entry_t entry,
                                                   void *arg_p);

/**
 * Execute given command in a subprocess. Returns captured subprocess'
 * exit code, or NULL if the subprocess could not be started.
 */
struct narwhal_subprocess_result_t *narwhal_subprocess_exec(const char *command_p);

/**
 * Execute given command in a subprocess. Returns captured subprocess'
 * stdout, stderr and exit code, or NULL if the subprocess could not
 * be started.
 */
struct narwhal_subprocess_result_t *narwhal_subprocess_exec_output(const char *command_p);

/**
 * Returns true if the subprocess was started and exited with 0,
 * otherwise false.
 */
bool narwhal_subprocess_completed_successfully(struct narwhal_subprocess_result_t *result_p);

/**
 * Print subprocess exit code, stdout and stderr.
 */
void narwhal_subprocess_result_print(struct narwhal_subprocess_result_t *self_p);

/**
 * Free given result.
 */
void narwhal_subprocess_result_free(struct narwhal_subprocess_result_t *self_p);

// #include "traceback.h"
/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Erik Moqvist
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * This file is part of the traceback project.
 */

#define NARWHAL_TRACEBACK_VERSION "0.1.0"

/**
 * Print a traceback.
 */
void narwhal_traceback_print(const char *prefix_p);

#include "narwhal.h"
// #include "diff/diff.h"
#ifndef NARWHAL_DIFF_H
#define NARWHAL_DIFF_H

#include <stdlib.h>

// #include "types.h"
#ifndef NARWHAL_DIFF_TYPES_H
#define NARWHAL_DIFF_TYPES_H

typedef struct NarwhalDiffMatrix NarwhalDiffMatrix;
typedef enum NarwhalDiffChunkType NarwhalDiffChunkType;
typedef struct NarwhalDiff NarwhalDiff;
typedef struct NarwhalDiffChunk NarwhalDiffChunk;

#endif


struct NarwhalDiffMatrix
{
    size_t rows;
    size_t columns;
    int *content;
};

enum NarwhalDiffChunkType
{
    NARWHAL_DIFF_CHUNK_TYPE_MATCHED,
    NARWHAL_DIFF_CHUNK_TYPE_ADDED,
    NARWHAL_DIFF_CHUNK_TYPE_REPLACED,
    NARWHAL_DIFF_CHUNK_TYPE_DELETED
};

struct NarwhalDiff
{
    size_t size;
    NarwhalDiffChunk *chunks;
};

struct NarwhalDiffChunk
{
    NarwhalDiffChunkType type;
    size_t original_start;
    size_t original_end;
    size_t modified_start;
    size_t modified_end;
};

NarwhalDiffMatrix *narwhal_new_diff_matrix(size_t rows, size_t columns);
NarwhalDiffMatrix *narwhal_new_diff_matrix_from_lengths(size_t original_length,
                                                        size_t modified_lengths);
void narwhal_diff_matrix_fill_from_strings(NarwhalDiffMatrix *diff_matrix,
                                           const char *original,
                                           const char *modified);
void narwhal_diff_matrix_fill_from_lines(NarwhalDiffMatrix *diff_matrix,
                                         const char *original,
                                         const char *modified);
NarwhalDiff narwhal_diff_matrix_get_diff(const NarwhalDiffMatrix *diff_matrix);

size_t narwhal_diff_matrix_index(const NarwhalDiffMatrix *diff_matrix, size_t row, size_t column);
int narwhal_diff_matrix_get(const NarwhalDiffMatrix *diff_matrix, size_t row, size_t column);
void narwhal_diff_matrix_set(const NarwhalDiffMatrix *diff_matrix,
                             size_t row,
                             size_t column,
                             int value);

NarwhalDiff narwhal_diff_strings_lengths(const char *original,
                                         size_t original_length,
                                         const char *modified,
                                         size_t modified_length);
NarwhalDiff narwhal_diff_strings(const char *original, const char *modified);
NarwhalDiff narwhal_diff_lines(const char *original, const char *modified);

void narwhal_free_diff_matrix(NarwhalDiffMatrix *diff_matrix);

#endif

// #include "hexdump/hexdump.h"
#ifndef NARWHAL_HEXDUMP_H
#define NARWHAL_HEXDUMP_H

#include <stdint.h>
#include <stdlib.h>

char *narwhal_hexdump(const uint8_t *buffer, size_t size, size_t bytes_per_row);
size_t narwhal_optimal_bytes_per_row(size_t element_size, size_t target, size_t range);

#endif

// #include "utils.h"
#ifndef NARWHAL_UTILS_H
#define NARWHAL_UTILS_H

#include <stdbool.h>
#include <stdio.h>

size_t narwhal_util_read_stream(FILE *stream, char **buffer);
bool narwhal_is_short_string(const char *string);
int narwhal_min_int(int a, int b);
size_t narwhal_min_size_t(size_t a, size_t b);
size_t narwhal_count_chars(const char *string, char chr);
const char *narwhal_next_line(const char *string);
const char *narwhal_next_lines(const char *string, size_t lines);

#endif


#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"

#define ANSI_BOLD "\x1b[1m"
#define ANSI_RESET "\x1b[0m"

#define COLOR(color, ...) ANSI_RESET ANSI_COLOR_##color __VA_ARGS__ ANSI_RESET

#define BOLD(...) ANSI_RESET ANSI_BOLD __VA_ARGS__ ANSI_RESET

#define COLOR_BOLD(color, ...)                                          \
    ANSI_RESET ANSI_COLOR_##color ANSI_BOLD __VA_ARGS__ ANSI_RESET

struct tests_t {
    struct narwhal_test_t *head_p;
    struct narwhal_test_t *tail_p;
};

struct capture_output_t {
    bool running;
    char **output_pp;
    size_t length;
    int original_fd;
    FILE *temporary_file_p;
    FILE *original_file_p;
};

static struct narwhal_test_t *current_test_p = NULL;

static struct tests_t tests = {
    .head_p = NULL,
    .tail_p = NULL
};

static struct capture_output_t capture_stdout;
static struct capture_output_t capture_stderr;

int setup(void);

int teardown(void);

void narmock_assert_all_mocks_completed(void);

__attribute__ ((weak)) void narmock_assert_all_mocks_completed(void)
{
}

static void capture_output_init(struct capture_output_t *self_p,
                                FILE *file_p)
{
    self_p->output_pp = NULL;
    self_p->length = 0;
    self_p->original_file_p = file_p;
}

static void capture_output_destroy(struct capture_output_t *self_p)
{
    if (self_p->output_pp != NULL) {
        if (*self_p->output_pp != NULL) {
            free(*self_p->output_pp);
        }

        self_p->output_pp = NULL;
    }
}

static void capture_output_redirect(struct capture_output_t *self_p)
{
    fflush(self_p->original_file_p);

    self_p->temporary_file_p = tmpfile();
    self_p->original_fd = dup(fileno(self_p->original_file_p));

    while ((dup2(fileno(self_p->temporary_file_p),
                 fileno(self_p->original_file_p)) == -1)
           && (errno == EINTR));
}

static void capture_output_restore(struct capture_output_t *self_p)
{
    fflush(self_p->original_file_p);

    while ((dup2(self_p->original_fd, fileno(self_p->original_file_p)) == -1)
           && (errno == EINTR));

    close(self_p->original_fd);
}

static void capture_output_start(struct capture_output_t *self_p,
                                 char **output_pp)
{
    self_p->output_pp = output_pp;
    self_p->length = 0;
    self_p->running = true;
    capture_output_redirect(self_p);
}

static void capture_output_stop(struct capture_output_t *self_p)
{
    size_t nmembers;

    if (!self_p->running) {
        return;
    }

    self_p->running = false;
    capture_output_restore(self_p);

    self_p->length = (size_t)ftell(self_p->temporary_file_p);
    fseek(self_p->temporary_file_p, 0, SEEK_SET);
    *self_p->output_pp = malloc(self_p->length + 1);

    if (*self_p->output_pp == NULL) {
        printf("Failed to allocate memory.\n");
        exit(1);
    }

    if (self_p->length > 0) {
        nmembers = fread(*self_p->output_pp,
                         self_p->length,
                         1,
                         self_p->temporary_file_p);

        if (nmembers != 1) {
            printf("Failed to read capture output.\n");
            exit(1);
        }
    }

    (*self_p->output_pp)[self_p->length] = '\0';
    fclose(self_p->temporary_file_p);

    printf("%s", *self_p->output_pp);
}

static float timeval_to_ms(struct timeval *timeval_p)
{
    float res;

    res = (float)timeval_p->tv_usec;
    res /= 1000;
    res += (float)(1000 * timeval_p->tv_sec);

    return (res);
}

static void print_signal_failure(struct narwhal_test_t *test_p)
{
    printf("\n");
    printf("%s failed:\n\n", test_p->name_p);
    printf("  Location: " COLOR_BOLD(GREEN, "unknown\n"));
    printf("  Error:    " COLOR_BOLD(RED, "Terminated by signal %d.\n"),
           test_p->signal_number);
}

static void print_location_context(const char *filename_p, size_t line_number)
{
    FILE *file_p;
    char line_prefix[64];
    char line[256];
    size_t first_line;
    size_t i;

    file_p = fopen(filename_p, "r");

    if (file_p == NULL) {
        return;
    }

    if (line_number < 2) {
        first_line = 1;
    } else {
        first_line = (line_number - 2);
    }

    for (i = 1; i < line_number + 3; i++) {
        if (fgets(&line[0], sizeof(line), file_p) == NULL) {
            goto out1;
        }

        if (i < first_line) {
            continue;
        }

        if (i == line_number) {
            snprintf(line_prefix,
                     sizeof(line_prefix),
                     "> " COLOR_BOLD(MAGENTA, "%ld"),
                     i);
            printf("  %23s", line_prefix);
            printf(" |  " COLOR_BOLD(CYAN, "%s"), line);
        } else {
            printf("  " COLOR(MAGENTA, "%6zu"), i);
            printf(" |  %s", line);
        }
    }

 out1:

    printf("\n");
    fclose(file_p);
}

static void print_test_results(struct narwhal_test_t *test_p,
                               float elapsed_time_ms)
{
    int total;
    int passed;
    int failed;
    const char *result_p;

    total = 0;
    passed = 0;
    failed = 0;

    printf("\nTest results:\n\n");
    fflush(stdout);

    while (test_p != NULL) {
        total++;

        if (test_p->exit_code == 0) {
            result_p = COLOR_BOLD(GREEN, "PASSED");
            passed++;
        } else {
            result_p = COLOR_BOLD(RED, "FAILED");
            failed++;
        }

        printf("  %s %s (" COLOR_BOLD(YELLOW, "%.02f ms") ")",
               result_p,
               test_p->name_p,
               test_p->elapsed_time_ms);

        if (test_p->signal_number != -1) {
            printf(" (signal: %d)", test_p->signal_number);
        }

        printf("\n");
        fflush(stdout);

        test_p = test_p->next_p;
    }

    printf("\nTests: ");

    if (failed > 0) {
        printf(COLOR_BOLD(RED, "%d failed") ", ", failed);
    }

    if (passed > 0) {
        printf(COLOR_BOLD(GREEN, "%d passed") ", ", passed);
    }

    printf("%d total\n", total);
    printf("Time: " COLOR_BOLD(YELLOW, "%.02f ms") "\n", elapsed_time_ms);
}

static void test_entry(void *arg_p)
{
    struct narwhal_test_t *test_p;
    int res;

    test_p = (struct narwhal_test_t *)arg_p;

    capture_output_init(&capture_stdout, stdout);
    capture_output_init(&capture_stderr, stderr);

    res = setup();

    if (res == 0) {
        test_p->func();
        res = teardown();

        if (res == 0) {
            narmock_assert_all_mocks_completed();
        }
    }

    capture_output_destroy(&capture_stdout);
    capture_output_destroy(&capture_stderr);

    exit(res == 0 ? 0 : 1);
}

static int run_tests(struct narwhal_test_t *tests_p)
{
    int res;
    struct timeval start_time;
    struct timeval end_time;
    struct timeval test_start_time;
    struct timeval test_end_time;
    struct timeval elapsed_time;
    struct narwhal_test_t *test_p;
    struct narwhal_subprocess_result_t *result_p;

    test_p = tests_p;
    gettimeofday(&start_time, NULL);
    res = 0;

    while (test_p != NULL) {
        gettimeofday(&test_start_time, NULL);
        current_test_p = test_p;
        test_p->before_fork_func();

        result_p = narwhal_subprocess_call(test_entry, test_p);

        test_p->exit_code = result_p->exit_code;
        test_p->signal_number = result_p->signal_number;
        narwhal_subprocess_result_free(result_p);

        if (test_p->exit_code != 0) {
            res = 1;
        }

        gettimeofday(&test_end_time, NULL);
        timersub(&test_end_time, &test_start_time, &elapsed_time);
        test_p->elapsed_time_ms = timeval_to_ms(&elapsed_time);

        if (test_p->signal_number != -1) {
            print_signal_failure(test_p);
        }

        test_p = test_p->next_p;
    }

    gettimeofday(&end_time, NULL);
    timersub(&end_time, &start_time, &elapsed_time);
    print_test_results(tests_p, timeval_to_ms(&elapsed_time));

    return (res);
}

bool narwhal_check_string_equal(const char *actual_p, const char *expected_p)
{
    return (strcmp(actual_p, expected_p) == 0);
}

const char *narwhal_format(const char *format_p, ...)
{
    char buf[1024];
    va_list vl;

    va_start(vl, format_p);
    vsnprintf(&buf[0], sizeof(buf), format_p, vl);
    va_end(vl);
    buf[sizeof(buf) - 1] = '\0';

    return strdup(&buf[0]);
}

static const char *display_inline_diff(FILE *file_p,
                                       const NarwhalDiff *inline_diff,
                                       size_t lines,
                                       const char *string,
                                       size_t *line_number,
                                       bool use_original)
{
    NarwhalDiffChunk *inline_chunk = &inline_diff->chunks[0];

    size_t line_index = 0;
    size_t index = 0;

    for (size_t i = 0; i < lines; i++)
        {
            const char *next = narwhal_next_line(string);
            size_t line_length = (size_t)(next - string);

            char line_prefix[64];

            if (use_original)
                {
                    snprintf(line_prefix,
                             sizeof(line_prefix),
                             COLOR(RED, "- ") COLOR_BOLD(RED, "%ld"),
                             *line_number);
                    fprintf(file_p, "   %37s" COLOR(RED, " |  "), line_prefix);
                }
            else
                {
                    snprintf(line_prefix,
                             sizeof(line_prefix),
                             COLOR(GREEN, "+ ") COLOR_BOLD(GREEN, "%ld"),
                             *line_number);
                    fprintf(file_p, "   %37s" COLOR(GREEN, " |  "), line_prefix);
                }

            while (index - line_index < line_length)
                {
                    size_t chunk_end =
                        use_original ? inline_chunk->original_end : inline_chunk->modified_end;

                    size_t start = index - line_index;
                    size_t end = narwhal_min_size_t(chunk_end - line_index, line_length);

                    size_t characters = end - start;

                    if (inline_chunk->type == NARWHAL_DIFF_CHUNK_TYPE_MATCHED)
                        {
                            fprintf(file_p, "%.*s", (int)characters, string + index - line_index);
                        }
                    else if (characters > 0)
                        {
                            if (use_original)
                                {
                                    fprintf(file_p,
                                            COLOR_BOLD(RED, "%.*s"),
                                            (int)characters,
                                            string + index - line_index);
                                }
                            else
                                {
                                    fprintf(file_p,
                                            COLOR_BOLD(GREEN, "%.*s"),
                                            (int)characters,
                                            string + index - line_index);
                                }
                        }

                    index += characters;

                    if (index >= chunk_end)
                        {
                            inline_chunk++;
                        }
                }

            fprintf(file_p, "\n");

            if (!use_original)
                {
                    (*line_number)++;
                }

            string = next + 1;
            line_index += line_length + 1;
            index = line_index;
        }

    return string;
}

static void print_string_diff(FILE *file_p,
                              const char *original,
                              const char *modified)
{
    fprintf(file_p, "  Diff:\n\n");

    NarwhalDiff diff = narwhal_diff_lines(original, modified);

    size_t line_number = 1;

    for (size_t chunk_index = 0; chunk_index < diff.size; chunk_index++)
        {
            NarwhalDiffChunk *chunk = &diff.chunks[chunk_index];

            size_t original_lines = chunk->original_end - chunk->original_start;
            size_t modified_lines = chunk->modified_end - chunk->modified_start;

            if (chunk->type == NARWHAL_DIFF_CHUNK_TYPE_MATCHED)
                {
                    for (size_t i = 0; i < original_lines; i++)
                        {
                            const char *original_next = narwhal_next_line(original);
                            const char *modified_next = narwhal_next_line(modified);

                            if (original_lines < 7 || (i < 2 && chunk_index > 0) ||
                                (original_lines - i < 3 && chunk_index < diff.size - 1))
                                {
                                    fprintf(file_p, COLOR(MAGENTA, "%6zu"), line_number);
                                    fprintf(file_p, " |  %.*s\n", (int)(original_next - original), original);
                                }
                            else if (i == 2)
                                {
                                    fprintf(file_p, "   :\n");
                                }

                            line_number++;
                            original = original_next + 1;
                            modified = modified_next + 1;
                        }
                }
            else if (chunk->type == NARWHAL_DIFF_CHUNK_TYPE_REPLACED)
                {
                    const char *original_end = narwhal_next_lines(original, original_lines);
                    const char *modified_end = narwhal_next_lines(modified, modified_lines);

                    size_t original_length = (size_t)(original_end - original);
                    size_t modified_length = (size_t)(modified_end - modified);

                    NarwhalDiff inline_diff =
                        narwhal_diff_strings_lengths(original,
                                                     original_length,
                                                     modified,
                                                     modified_length);

                    original = display_inline_diff(file_p,
                                                   &inline_diff,
                                                   original_lines,
                                                   original,
                                                   &line_number,
                                                   true);
                    modified = display_inline_diff(file_p,
                                                   &inline_diff,
                                                   modified_lines,
                                                   modified,
                                                   &line_number,
                                                   false);

                    free(inline_diff.chunks);
                }
            else if (chunk->type == NARWHAL_DIFF_CHUNK_TYPE_DELETED)
                {
                    for (size_t i = 0; i < original_lines; i++)
                        {
                            const char *original_next = narwhal_next_line(original);

                            char line_prefix[64];
                            snprintf(line_prefix,
                                     sizeof(line_prefix),
                                     COLOR(RED, "- ") COLOR_BOLD(RED, "%ld"),
                                     line_number);

                            printf("   %37s", line_prefix);
                            printf(COLOR(RED, " |  ") COLOR_BOLD(RED, "%.*s\n"),
                                   (int)(original_next - original),
                                   original);

                            original = original_next + 1;
                        }
                }
            else if (chunk->type == NARWHAL_DIFF_CHUNK_TYPE_ADDED)
                {
                    for (size_t i = 0; i < modified_lines; i++)
                        {
                            const char *modified_next = narwhal_next_line(modified);

                            char line_prefix[64];
                            snprintf(line_prefix,
                                     sizeof(line_prefix),
                                     COLOR(GREEN, "+ ") COLOR_BOLD(GREEN, "%ld"),
                                     line_number);

                            printf("   %37s", line_prefix);
                            printf(COLOR(GREEN, " |  ") COLOR_BOLD(GREEN, "%.*s\n"),
                                   (int)(modified_next - modified),
                                   modified);

                            line_number++;
                            modified = modified_next + 1;
                        }
                }
        }

    free(diff.chunks);
}

const char *narwhal_format_string(const char *format_p, ...)
{
    size_t size;
    char *buf_p;
    FILE *file_p;
    va_list vl;
    const char *left_p;
    const char *right_p;

    va_start(vl, format_p);
    left_p = va_arg(vl, const char *);
    right_p = va_arg(vl, const char *);
    va_end(vl);

    file_p = open_memstream(&buf_p, &size);
    fprintf(file_p, format_p, left_p, right_p);
    fprintf(file_p, " See diff for details.\n");
    print_string_diff(file_p, left_p, right_p);
    fputc('\0', file_p);
    fclose(file_p);

    return (buf_p);
}

const char *narwhal_format_memory(const void *left_p,
                                  const void *right_p,
                                  size_t size)
{
    size_t file_size;
    char *buf_p;
    FILE *file_p;
    char *left_hexdump_p;
    char *right_hexdump_p;

    file_p = open_memstream(&buf_p, &file_size);
    fprintf(file_p, "Memory mismatch. See diff for details.\n");
    left_hexdump_p = narwhal_hexdump(left_p, size, 16);
    right_hexdump_p = narwhal_hexdump(right_p, size, 16);
    print_string_diff(file_p, right_hexdump_p, left_hexdump_p);
    free(left_hexdump_p);
    free(right_hexdump_p);
    fputc('\0', file_p);
    fclose(file_p);

    return (buf_p);
}

bool narwhal_check_substring(const char *actual_p, const char *expected_p)
{
    return ((actual_p != NULL)
            && (expected_p != NULL)
            && (strstr(actual_p, expected_p) != NULL));
}

void narwhal_test_failure(const char *file_p,
                          int line,
                          const char *message_p)
{
    narwhal_capture_output_stop();
    capture_output_destroy(&capture_stdout);
    capture_output_destroy(&capture_stderr);
    printf("\n");
    printf("%s failed:\n\n", current_test_p->name_p);
    printf("  Location: " COLOR_BOLD(GREEN, "%s:%d:\n"), file_p, line);
    printf("  Error:    %s\n", message_p);
    print_location_context(file_p, (size_t)line);
    narwhal_traceback_print("  ");
    printf("\n");
    exit(1);
}

void narwhal_capture_output_start(char **output_pp, char **errput_pp)
{
    capture_output_start(&capture_stdout, output_pp);
    capture_output_start(&capture_stderr, errput_pp);
}

void narwhal_capture_output_stop()
{
    capture_output_stop(&capture_stdout);
    capture_output_stop(&capture_stderr);
}

void narwhal_register_test(struct narwhal_test_t *test_p)
{
    if (tests.head_p == NULL) {
        tests.head_p = test_p;
    } else {
        tests.tail_p->next_p = test_p;
    }

    tests.tail_p = test_p;
}

int narwhal_run_tests()
{
    return (run_tests(tests.head_p));
}

__attribute__((weak)) int setup(void)
{
    return (0);
}

__attribute__((weak)) int teardown(void)
{
    return (0);
}

__attribute__((weak)) int main(void)
{
    return (narwhal_run_tests());
}
/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Erik Moqvist
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * This file is part of the subprocess project.
 */

#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
// #include "subprocess.h"


static void fatal_error(const char *message_p)
{
    perror(message_p);
    exit(1);
}

static int output_init(struct narwhal_subprocess_output_t *self_p)
{
    int res;

    res = -1;
    self_p->length = 0;
    self_p->size = 4096;
    self_p->buf_p = malloc(self_p->size);

    if (self_p->buf_p != NULL) {
        self_p->buf_p[0] = '\0';
        res = 0;
    }

    return (res);
}

static void output_append(struct narwhal_subprocess_output_t *self_p, int fd)
{
    ssize_t res;

    while (1) {
        res = read(fd,
                   &self_p->buf_p[self_p->length],
                   self_p->size - self_p->length - 1);

        if (res == 0) {
            break;
        } else if (res > 0) {
            self_p->length += (size_t)res;
            self_p->buf_p[self_p->length] = '\0';

            if ((self_p->length + 1) == self_p->size) {
                self_p->size += 4096;
                self_p->buf_p = realloc(self_p->buf_p, self_p->size);
            }
        } else {
            if (errno != EINTR) {
                fatal_error("read");
            }
        }
    }
}

static void output_print(struct narwhal_subprocess_output_t *self_p,
                         const char *name_p)
{
    printf("%s (length: %ld):\n", name_p, self_p->length);
    printf("%s\n", self_p->buf_p);
}

static void redirect_output(int *fds_p, int fileno)
{
    close(fds_p[0]);
    while ((dup2(fds_p[1], fileno) == -1) && (errno == EINTR));
    close(fds_p[1]);
}

static void close_fds(int *fds_p)
{
    close(fds_p[0]);
    close(fds_p[1]);
}

static struct narwhal_subprocess_result_t *result_new(void)
{
    struct narwhal_subprocess_result_t *result_p;
    int res;

    result_p = malloc(sizeof(*result_p));

    if (result_p == NULL) {
        return (NULL);
    }

    result_p->exit_code = -1;
    result_p->signal_number = -1;
    res = output_init(&result_p->stdout);

    if (res != 0) {
        goto out1;
    }

    res = output_init(&result_p->stderr);

    if (res != 0) {
        goto out2;
    }

    return (result_p);

 out2:
    free(result_p->stdout.buf_p);

 out1:
    free(result_p);

    return (NULL);
}

static void call_child(narwhal_subprocess_entry_t entry,
                       void *arg_p)
{
    entry(arg_p);
}

static struct narwhal_subprocess_result_t *call_parent(pid_t child_pid)
{
    struct narwhal_subprocess_result_t *result_p;
    int status;

    result_p = result_new();

    waitpid(child_pid, &status, 0);

    if (result_p != NULL) {
        if (WIFEXITED(status)) {
            result_p->exit_code = WEXITSTATUS(status);
        }

        if (WIFSIGNALED(status)) {
            result_p->signal_number = WTERMSIG(status);
        }
    }

    return (result_p);
}

static void call_output_child(narwhal_subprocess_entry_t entry,
                              void *arg_p,
                              int *stdoutfds_p,
                              int *stderrfds_p)
{
    redirect_output(stdoutfds_p, STDOUT_FILENO);
    redirect_output(stderrfds_p, STDERR_FILENO);
    call_child(entry, arg_p);
}

static struct narwhal_subprocess_result_t *call_output_parent(pid_t child_pid,
                                                      int *stdoutfds_p,
                                                      int *stderrfds_p)
{
    struct narwhal_subprocess_result_t *result_p;

    /* Close write ends. */
    close(stdoutfds_p[1]);
    close(stderrfds_p[1]);

    result_p = call_parent(child_pid);

    /* Poll stdout and stderr pipes. */
    if (result_p != NULL) {
        output_append(&result_p->stdout, stdoutfds_p[0]);
        output_append(&result_p->stderr, stderrfds_p[0]);
    }

    close(stdoutfds_p[0]);
    close(stderrfds_p[0]);

    return (result_p);
}

static void exec_entry(const char *command_p)
{
    int res;

    res = execl("/bin/sh", "sh", "-c", command_p, NULL);

    if (res != 0) {
        exit(1);
    }
}

struct narwhal_subprocess_result_t *narwhal_subprocess_call(narwhal_subprocess_entry_t entry,
                                            void *arg_p)
{
    pid_t pid;
    struct narwhal_subprocess_result_t *result_p;

    fflush(stdout);
    fflush(stderr);

    pid = fork();

    if (pid < 0) {
        result_p = NULL;
    } else if (pid == 0) {
        call_child(entry, arg_p);
        exit(0);
    } else {
        result_p = call_parent(pid);
    }

    return (result_p);
}

struct narwhal_subprocess_result_t *narwhal_subprocess_call_output(narwhal_subprocess_entry_t entry,
                                                   void *arg_p)
{
    pid_t pid;
    int stdoutfds[2];
    int stderrfds[2];
    struct narwhal_subprocess_result_t *result_p;

    fflush(stdout);
    fflush(stderr);

    if (pipe(stdoutfds) < 0) {
        return (NULL);
    }

    if (pipe(stderrfds) < 0) {
        goto out1;
    }

    pid = fork();

    if (pid < 0) {
        goto out2;
    } else if (pid == 0) {
        call_output_child(entry, arg_p, &stdoutfds[0], &stderrfds[0]);
        exit(0);
    } else {
        result_p = call_output_parent(pid, &stdoutfds[0], &stderrfds[0]);
    }

    return (result_p);

 out2:
    close_fds(&stderrfds[0]);

 out1:
    close_fds(&stdoutfds[0]);

    return (NULL);
}

struct narwhal_subprocess_result_t *narwhal_subprocess_exec(const char *command_p)
{
    return (narwhal_subprocess_call((narwhal_subprocess_entry_t)exec_entry,
                            (void *)command_p));
}

struct narwhal_subprocess_result_t *narwhal_subprocess_exec_output(const char *command_p)
{
    return (narwhal_subprocess_call_output((narwhal_subprocess_entry_t)exec_entry,
                                   (void *)command_p));
}

bool narwhal_subprocess_completed_successfully(struct narwhal_subprocess_result_t *result_p)
{
    return ((result_p != NULL) && (result_p->exit_code == 0));
}

void narwhal_subprocess_result_print(struct narwhal_subprocess_result_t *self_p)
{
    printf("exit_code: %d\n", self_p->exit_code);
    output_print(&self_p->stdout, "stdout");
    output_print(&self_p->stderr, "stderr");
}

void narwhal_subprocess_result_free(struct narwhal_subprocess_result_t *self_p)
{
    free(self_p->stdout.buf_p);
    free(self_p->stderr.buf_p);
    free(self_p);
}
/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Erik Moqvist
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * This file is part of the traceback project.
 */

#include <stdio.h>
#include <execinfo.h>
// #include "traceback.h"

#include <stdlib.h>
#include <unistd.h>

#define DEPTH_MAX 100

void narwhal_traceback_print(const char *prefix_p)
{
    int depth;
    void *addresses[DEPTH_MAX];
    char exe[256];
    char command[384];
    ssize_t size;
    int res;
    int i;

    depth = backtrace(&addresses[0], DEPTH_MAX);

    printf("%sTraceback (most recent call last):\n", prefix_p);

    size = readlink("/proc/self/exe", &exe[0], sizeof(exe) - 1);

    if (size == -1) {
        printf("%sNo executable found!\n", prefix_p);

        return;
    }

    exe[size] = '\0';

    for (i = (depth - 1); i >= 0; i--) {
        printf("%s  ", prefix_p);
        fflush(stdout);

        snprintf(&command[0],
                 sizeof(command),
                 "addr2line -f -p -e %s %p",
                 &exe[0],
                 addresses[i]);
        command[sizeof(command) - 1] = '\0';

        res = system(&command[0]);

        if (res == -1) {
            return;
        } else if (WIFEXITED(res)) {
            if (WEXITSTATUS(res) != 0) {
                return;
            }
        } else {
            return;
        }
    }
}
// #include "utils.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

size_t narwhal_util_read_stream(FILE *stream, char **output_buffer)
{
    char buffer[256];
    size_t output_length = 0;

    size_t read_count = fread(buffer, 1, sizeof(buffer) - 1, stream);
    buffer[read_count] = '\0';

    if (read_count > 0)
    {
        *output_buffer = malloc(read_count + 1);
        memcpy(*output_buffer, buffer, read_count + 1);
    }

    output_length = read_count;

    while (read_count + 1 == sizeof(buffer))
    {
        read_count = fread(buffer, 1, sizeof(buffer) - 1, stream);
        buffer[read_count] = '\0';
        output_length += read_count;

        *output_buffer = realloc(*output_buffer, output_length + 1);
        memcpy(*output_buffer + output_length - read_count, buffer, read_count + 1);
    }

    return output_length;
}

bool narwhal_is_short_string(const char *string)
{
    return strlen(string) < 64 && strchr(string, '\n') == NULL;
}

int narwhal_min_int(int a, int b)
{
    return a < b ? a : b;
}

size_t narwhal_min_size_t(size_t a, size_t b)
{
    return a < b ? a : b;
}

size_t narwhal_count_chars(const char *string, char chr)
{
    size_t count = 0;

    for (size_t i = 0; string[i] != '\0'; i++)
    {
        if (string[i] == chr)
        {
            count++;
        }
    }

    return count;
}

const char *narwhal_next_line(const char *string)
{
    char *next_line = strchr(string, '\n');

    if (next_line != NULL)
    {
        return next_line;
    }
    else
    {
        return string + strlen(string);
    }
}

const char *narwhal_next_lines(const char *string, size_t lines)
{
    const char *next_line = string;

    for (size_t i = 0; i < lines; i++)
    {
        next_line = narwhal_next_line(next_line) + 1;
    }

    return next_line;
}
// #include "diff.h"


#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

// #include "../utils.h"
#ifndef NARWHAL_UTILS_H
#define NARWHAL_UTILS_H

#include <stdbool.h>
#include <stdio.h>

size_t narwhal_util_read_stream(FILE *stream, char **buffer);
bool narwhal_is_short_string(const char *string);
int narwhal_min_int(int a, int b);
size_t narwhal_min_size_t(size_t a, size_t b);
size_t narwhal_count_chars(const char *string, char chr);
const char *narwhal_next_line(const char *string);
const char *narwhal_next_lines(const char *string, size_t lines);

#endif


/*
 * Diff matrix initialization
 */

static void initialize_diff_matrix(NarwhalDiffMatrix *diff_matrix, size_t rows, size_t columns)
{
    diff_matrix->rows = rows;
    diff_matrix->columns = columns;
    diff_matrix->content = malloc(rows * columns * sizeof(int));
}

NarwhalDiffMatrix *narwhal_new_diff_matrix(size_t rows, size_t columns)
{
    NarwhalDiffMatrix *diff_matrix = malloc(sizeof(NarwhalDiffMatrix));
    initialize_diff_matrix(diff_matrix, rows, columns);

    return diff_matrix;
}

/*
 * Diff matrix operations
 */

NarwhalDiffMatrix *narwhal_new_diff_matrix_from_lengths(size_t original_length,
                                                        size_t modified_length)
{
    NarwhalDiffMatrix *diff_matrix =
        narwhal_new_diff_matrix(modified_length + 1, original_length + 1);

    for (size_t i = 0; i < diff_matrix->rows; i++)
    {
        narwhal_diff_matrix_set(diff_matrix, i, 0, (int)i);
    }

    for (size_t j = 0; j < diff_matrix->columns; j++)
    {
        narwhal_diff_matrix_set(diff_matrix, 0, j, (int)j);
    }

    return diff_matrix;
}

static void fill_different(NarwhalDiffMatrix *diff_matrix, size_t i, size_t j)
{
    narwhal_diff_matrix_set(
        diff_matrix,
        i,
        j,
        narwhal_min_int(narwhal_diff_matrix_get(diff_matrix, i - 1, j - 1),
                        narwhal_min_int(narwhal_diff_matrix_get(diff_matrix, i - 1, j),
                                        narwhal_diff_matrix_get(diff_matrix, i, j - 1))) +
            1);
}

static void fill_equal(NarwhalDiffMatrix *diff_matrix, size_t i, size_t j)
{
    narwhal_diff_matrix_set(diff_matrix, i, j, narwhal_diff_matrix_get(diff_matrix, i - 1, j - 1));
}

void narwhal_diff_matrix_fill_from_strings(NarwhalDiffMatrix *diff_matrix,
                                           const char *original,
                                           const char *modified)
{
    for (size_t i = 1; i < diff_matrix->rows; i++)
    {
        for (size_t j = 1; j < diff_matrix->columns; j++)
        {
            if (original[j - 1] == modified[i - 1])
            {
                fill_equal(diff_matrix, i, j);
            }
            else
            {
                fill_different(diff_matrix, i, j);
            }
        }
    }
}

void narwhal_diff_matrix_fill_from_lines(NarwhalDiffMatrix *diff_matrix,
                                         const char *original,
                                         const char *modified)
{
    const char *modified_pos;
    const char *modified_line = modified;

    for (size_t i = 1; i < diff_matrix->rows; i++)
    {
        modified_pos = narwhal_next_line(modified_line);
        size_t modified_line_length = (size_t)(modified_pos - modified_line);

        const char *original_pos;
        const char *original_line = original;

        for (size_t j = 1; j < diff_matrix->columns; j++)
        {
            original_pos = narwhal_next_line(original_line);
            size_t original_line_length = (size_t)(original_pos - original_line);

            if (original_line_length == modified_line_length &&
                strncmp(original_line, modified_line, original_line_length) == 0)
            {
                fill_equal(diff_matrix, i, j);
            }
            else
            {
                fill_different(diff_matrix, i, j);
            }

            original_line = original_pos + 1;
        }

        modified_line = modified_pos + 1;
    }
}

NarwhalDiff narwhal_diff_matrix_get_diff(const NarwhalDiffMatrix *diff_matrix)
{
    if (diff_matrix->rows == 1 && diff_matrix->columns == 1)
    {
        NarwhalDiff diff = { .size = 0, .chunks = NULL };
        return diff;
    }

    size_t capacity = 32;
    size_t size = 0;
    NarwhalDiffChunk *backtrack = malloc(capacity * sizeof(NarwhalDiffChunk));

    size_t i = diff_matrix->rows - 1;
    size_t j = diff_matrix->columns - 1;

    while (i > 0 || j > 0)
    {
        if (size == capacity)
        {
            capacity *= 2;
            backtrack = realloc(backtrack, capacity * sizeof(NarwhalDiffChunk));
        }

        NarwhalDiffChunk *current_chunk = &backtrack[size];
        size++;

        int current = narwhal_diff_matrix_get(diff_matrix, i, j);

        if (i > 0 && j > 0 && current == narwhal_diff_matrix_get(diff_matrix, i - 1, j - 1) + 1)
        {
            current_chunk->type = NARWHAL_DIFF_CHUNK_TYPE_REPLACED;
            current_chunk->original_start = j - 1;
            current_chunk->original_end = j;
            current_chunk->modified_start = i - 1;
            current_chunk->modified_end = i;
            i--;
            j--;
        }
        else if (j > 0 && current == narwhal_diff_matrix_get(diff_matrix, i, j - 1) + 1)
        {
            current_chunk->type = NARWHAL_DIFF_CHUNK_TYPE_DELETED;
            current_chunk->original_start = j - 1;
            current_chunk->original_end = j;
            current_chunk->modified_start = i;
            current_chunk->modified_end = i;
            j--;
        }
        else if (i > 0 && current == narwhal_diff_matrix_get(diff_matrix, i - 1, j) + 1)
        {
            current_chunk->type = NARWHAL_DIFF_CHUNK_TYPE_ADDED;
            current_chunk->original_start = j;
            current_chunk->original_end = j;
            current_chunk->modified_start = i - 1;
            current_chunk->modified_end = i;
            i--;
        }
        else if (i > 0 && j > 0 && current == narwhal_diff_matrix_get(diff_matrix, i - 1, j - 1))
        {
            current_chunk->type = NARWHAL_DIFF_CHUNK_TYPE_MATCHED;
            current_chunk->original_start = j - 1;
            current_chunk->original_end = j;
            current_chunk->modified_start = i - 1;
            current_chunk->modified_end = i;
            i--;
            j--;
        }
    }

    NarwhalDiff diff = { size, malloc(size * sizeof(NarwhalDiffChunk)) };

    ssize_t backtrack_index = (ssize_t)size - 1;
    size_t chunk_index = 0;

    diff.chunks[chunk_index] = backtrack[backtrack_index];

    for (backtrack_index--; backtrack_index >= 0; backtrack_index--)
    {
        NarwhalDiffChunk *chunk = &backtrack[backtrack_index];
        NarwhalDiffChunk *previous_chunk = &diff.chunks[chunk_index];

        if (chunk->type == previous_chunk->type)
        {
            previous_chunk->original_end = chunk->original_end;
            previous_chunk->modified_end = chunk->modified_end;
        }
        else if ((chunk->type == NARWHAL_DIFF_CHUNK_TYPE_REPLACED &&
                  previous_chunk->type != NARWHAL_DIFF_CHUNK_TYPE_MATCHED) ||
                 (chunk->type != NARWHAL_DIFF_CHUNK_TYPE_MATCHED &&
                  previous_chunk->type == NARWHAL_DIFF_CHUNK_TYPE_REPLACED))
        {
            previous_chunk->type = NARWHAL_DIFF_CHUNK_TYPE_REPLACED;
            previous_chunk->original_end = chunk->original_end;
            previous_chunk->modified_end = chunk->modified_end;
        }
        else
        {
            chunk_index++;
            diff.chunks[chunk_index] = *chunk;
        }
    }

    free(backtrack);

    diff.size = chunk_index + 1;
    diff.chunks = realloc(diff.chunks, diff.size * sizeof(NarwhalDiffChunk));

    return diff;
}

size_t narwhal_diff_matrix_index(const NarwhalDiffMatrix *diff_matrix, size_t row, size_t column)
{
    return row * diff_matrix->columns + column;
}

int narwhal_diff_matrix_get(const NarwhalDiffMatrix *diff_matrix, size_t row, size_t column)
{
    return diff_matrix->content[narwhal_diff_matrix_index(diff_matrix, row, column)];
}

void narwhal_diff_matrix_set(const NarwhalDiffMatrix *diff_matrix,
                             size_t row,
                             size_t column,
                             int value)
{
    diff_matrix->content[narwhal_diff_matrix_index(diff_matrix, row, column)] = value;
}

/*
 * Higher-level wrappers
 */

NarwhalDiff narwhal_diff_strings_lengths(const char *original,
                                         size_t original_length,
                                         const char *modified,
                                         size_t modified_length)
{
    NarwhalDiffMatrix *diff_matrix =
        narwhal_new_diff_matrix_from_lengths(original_length, modified_length);

    narwhal_diff_matrix_fill_from_strings(diff_matrix, original, modified);

    NarwhalDiff diff = narwhal_diff_matrix_get_diff(diff_matrix);

    narwhal_free_diff_matrix(diff_matrix);

    return diff;
}

NarwhalDiff narwhal_diff_strings(const char *original, const char *modified)
{
    return narwhal_diff_strings_lengths(original, strlen(original), modified, strlen(modified));
}

NarwhalDiff narwhal_diff_lines(const char *original, const char *modified)
{
    size_t original_length = narwhal_count_chars(original, '\n') + 1;
    size_t modified_length = narwhal_count_chars(modified, '\n') + 1;

    NarwhalDiffMatrix *diff_matrix =
        narwhal_new_diff_matrix_from_lengths(original_length, modified_length);

    narwhal_diff_matrix_fill_from_lines(diff_matrix, original, modified);

    NarwhalDiff diff = narwhal_diff_matrix_get_diff(diff_matrix);

    narwhal_free_diff_matrix(diff_matrix);

    return diff;
}

/*
 * Cleanup
 */

void narwhal_free_diff_matrix(NarwhalDiffMatrix *diff_matrix)
{
    free(diff_matrix->content);
    free(diff_matrix);
}
// #include "hexdump.h"


#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

char *narwhal_hexdump(const uint8_t *buffer, size_t size, size_t bytes_per_row)
{
    size_t dump_size;
    char *dump;
    FILE *stream = open_memstream(&dump, &dump_size);

    size_t offset = 0;

    while (offset < size)
    {
        fprintf(stream, "%06lX  ", offset);

        for (size_t i = 0; i < bytes_per_row; i++)
        {
            if (offset + i < size)
            {
                fprintf(stream, "%02X ", buffer[offset + i]);
            }
            else
            {
                fprintf(stream, "-- ");
            }
        }

        fprintf(stream, " ");

        for (size_t i = 0; i < bytes_per_row; i++)
        {
            if (offset + i < size)
            {
                uint8_t byte = buffer[offset + i];
                fprintf(stream, "%c", isprint(byte) ? byte : '.');
            }
            else
            {
                fprintf(stream, " ");
            }
        }

        offset += bytes_per_row;

        if (offset < size)
        {
            fprintf(stream, "\n");
        }
    }

    fclose(stream);

    return dump;
}

size_t narwhal_optimal_bytes_per_row(size_t element_size, size_t target, size_t range)
{
    size_t min = target - range;
    size_t max = target + range;

    if (element_size < min)
    {
        return (size_t)((double)target / (double)element_size + 0.5) * element_size;
    }

    if (element_size > max)
    {
        if (element_size % target == 0)
        {
            return target;
        }

        size_t div_min = target;
        size_t div_max = target;

        while (div_min > min || div_max < max)
        {
            if (div_min > min)
            {
                div_min--;

                if (element_size % div_min == 0)
                {
                    return div_min;
                }
            }

            if (div_max < max)
            {
                div_max++;

                if (element_size % div_max == 0)
                {
                    return div_max;
                }
            }
        }

        return target;
    }

    return element_size;
}
