#ifndef LIBNONOTES_H
#define LIBNONOTES_H

#ifndef LIBNONOTES_BUF_SIZE
#define LIBNONOTES_BUF_SIZE 128
#endif /* LIBNONOTES_BUF_SIZE */

#ifndef LIBNONOTES_MAX_ARGS
#define LIBNONOTES_MAX_ARGS 16
#endif /* LIBNONOTES_MAX_ARGS */

#include <stdio.h>

typedef struct libnonotes_ParseState {
    enum {
        RULES_NOOP,
        RULES_ARGS,
        RULES_BODY
    } rules_state;

    char acc[LIBNONOTES_BUF_SIZE];
    int acc_len;
    enum {
        ACC_NOOP,
        ACC_COMPLETE,
        ACC_INCOMPLETE,
        ACC_SUBMIT
    } acc_state;

    /* to allow for recursion: this will save both the arg and its scope */
    struct {
        char arg[LIBNONOTES_BUF_SIZE];
        int arg_scope;
    } args_stack[LIBNONOTES_MAX_ARGS];
    int args_len;

    FILE* fp;
    int loc;

    char buf[LIBNONOTES_BUF_SIZE];
    int buf_idx;

    int body_len;
    int bracket_level;

    int tag_count;
} libnonotes_ParseState;

void libnonotes_parseC(libnonotes_ParseState* parse_state, char c);

#endif /* LIBNONOTES_H */
