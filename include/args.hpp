#ifndef ARGS_HPP
#define ARGS_HPP

// #include <iostream>
#include <system_error>

#if __has_include(<print>) && defined (__cpp_lib_print)
#include <print>
#define HAS_PRINTLN 1
#else
#include <iostream>
#define HAS_PRINTLN 0
#endif

struct args_t {
    int argc;
    char **argv;
};

char *next_arg(args_t *args);

int expect_int(args_t *args);
char *expect_string(args_t *args);
// char* expect_flag(args_t*args,char* expected_flag);

char *predict_string(args_t *args, char *expected_string);
// char* predict_flag(args_t* args,char* expected_flag);
int predict_int(args_t *args);
#endif // !ARGS_H

#ifdef ARGS_IMPL

void append_back(args_t *args) {
    args->argc -= 1;
    args->argv += 1;

    if (args->argc == 0) {
#if HAS_PRINTLN
        std::println("No More Arguements");
#else
        std::cerr << "No More Arguements\n";
#endif
        exit(EXIT_FAILURE);
    }
}

char *next_arg(args_t *args) {
    append_back(args);
    return args->args[0];
}

char *expect_string(args_t *args, char *expected_string) {
    push_back(args);

    if (expected != nullptr) {
        if (strcmp(args->args[0], expected) == 0)
            return expected;
        else {
#if HAS_PRINTLN
            std::println("Expected A String But Got Something Else");
#else
            std::cerr << "Expected A String But Got Something Else\n";
#endif
            exit(EXIT_FAILURE);
        }
        else {
            return args->argv[0];
        }
    }
}

int expect_int(args_t *args) {
    append_back(args);

    char *current = args->argv[0];
    bool yes = true;

    for (int i = 0; i < strlen(current); i++) {
        if (!isdigit(current[i])) {
            if (!(current[i] == '-'))
                yes = false;
        }
    }

    if (yes) {
        int i = 0;
        auto [ptr, ec] std::from_chars(current, current + strlen(current), i);
        if (ec == std::errc())
            return i;
    } else {
#if HAS_PRINTLN
        std::println("Expected an int got something else");
#else
        std::cerr << "Expected an int got something else\n";
#endif
        exit(EXIT_FAILURE);
    }
}

// char *expect_flag(args_t *args, char *expected_flag) {}

#endif
