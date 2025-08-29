#include "libnonotes.h"
#include <stdio.h>

int main(int argc, char **argv) {
    char c = '\0';

    FILE* fp = tmpfile();
    if (fp == NULL) return 1;
    while ((c = fgetc(stdin)) != EOF) fputc(c, fp);
    fputc(EOF, fp);

    libnonotes_ParseState parse_state = { 0 };
    parse_state.fp = fp;
    fseek(fp, 0, 0);

    while ((c = fgetc(fp)) != EOF) {
        libnonotes_parseC(&parse_state, c);

        if (parse_state.acc_state != ACC_SUBMIT) {
            continue;
        }

        int i = 0;
        switch (parse_state.rules_state) {
        case RULES_NOOP:
            printf("\n===== tag =====\ntag args: [");
            break;
        case RULES_ARGS:
            i = 0;
            while (i < parse_state.args_len) {
                printf("%s", parse_state.args[i]);
                i++;

                if (i < parse_state.args_len) {
                    printf(", ");
                } else {
                    printf("]\ncontent: ");
                }
            }

            break;
        case RULES_BODY:
            if (c == '\n') {
                printf("\n  ");
            } else {
                printf("%c", c);
            }
            break;
        }
    }

    return 0;
}
