#include "flyuxc/utils/cli.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void print_help(void) {
    printf("FLYUXC AOT Compiler\n");
    printf("Usage: flyuxc [options] <input file>\n\n");
    printf("Options:\n");
    printf("  -h, --help            Display this help message\n");
    printf("  -v, --version         Display compiler version\n");
    printf("  -o, --output <file>   Specify output file\n");
}

void print_version(void) {
    printf("FLYUXC AOT Compiler version 0.1.0\n");
}

CliOptions parse_arguments(int argc, char *argv[]) {
    CliOptions options = {
        .help = false,
        .version = false,
        .output = NULL,
        .input = NULL
    };

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            options.help = true;
        }
        else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            options.version = true;
        }
        else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 < argc) {
                options.output = argv[++i];
            }
        }
        else if (argv[i][0] != '-' && options.input == NULL) {
            options.input = argv[i];
        }
    }

    return options;
}