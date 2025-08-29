#include "libnonotes.h"

#include <assert.h>
#include <stdio.h>

#define NNTAG_STR_LEN 6
const char nntag_str[NNTAG_STR_LEN] = "nntag";

typedef enum {
    PEEK_AHEAD_TAGS,
    PEEK_AHEAD_BEFORE_BODY,
    PEEK_AHEAD_BODY
} PeekAheadStage;

void libnonotes_parseC(libnonotes_ParseState* parse_state, char c) {
    int peek_behind_idx = 0;
    char peek_behind = '\0';

    if (parse_state->acc_state != ACC_SUBMIT) {
        parse_state->loc++;
        parse_state->buf[parse_state->buf_idx] = c;
        parse_state->buf_idx = (parse_state->buf_idx + 1 >= LIBNONOTES_BUF_SIZE) ? 0 : parse_state->buf_idx + 1;
        parse_state->buf[parse_state->buf_idx] = '\0';
    }

    int i = 0;

    switch (parse_state->rules_state) {
    case RULES_NOOP:
        if (parse_state->acc_state == ACC_SUBMIT) {
            parse_state->rules_state = RULES_ARGS;
            parse_state->acc_state = ACC_NOOP;
            parse_state->body_len = 0;
            libnonotes_parseC(parse_state, c);
            return;
        }

        if (c != '(') {
            return;
        }
        int tag_valid = 0;

        /* check behind '(' for the "nntag" string */
        int nntag_check_idx = 0;
        while (nntag_str[nntag_check_idx] != '\0') {
            peek_behind_idx = (parse_state->buf_idx + nntag_check_idx - NNTAG_STR_LEN < 0) ? LIBNONOTES_BUF_SIZE + nntag_check_idx - NNTAG_STR_LEN : parse_state->buf_idx + nntag_check_idx - NNTAG_STR_LEN;
            peek_behind = parse_state->buf[peek_behind_idx];

            if (nntag_str[nntag_check_idx] != peek_behind) {
                tag_valid = 0;
                break;
            }

            tag_valid = 1;
            nntag_check_idx++;
        }
        if (tag_valid != 1) {
            return;
        }
        tag_valid = 0;

        /* check ahead of ')' if the tag ends properly  */
        PeekAheadStage peek_ahead_stage = PEEK_AHEAD_TAGS;
        int peek_ahead_lvl = 0;
        char peek_ahead = '\0';
        while ((peek_ahead = fgetc(parse_state->fp)) != EOF) {
            if (peek_ahead_stage == PEEK_AHEAD_TAGS) {
                if (peek_ahead == ')') {
                    peek_ahead_stage = PEEK_AHEAD_BEFORE_BODY;
                } else if (peek_ahead == '(') {
                    break;
                }
            } else if (peek_ahead_stage == PEEK_AHEAD_BEFORE_BODY) {
                if (peek_ahead == '{') {
                    peek_ahead_stage = PEEK_AHEAD_BODY;
                } else if (peek_ahead != ' ' && peek_ahead != '\t' && peek_ahead != '\n') {
                    break;
                }
            } else if (peek_ahead_stage == PEEK_AHEAD_BODY) {
                if (peek_ahead == '}' && peek_ahead_lvl <= 0) {
                    tag_valid = 1;
                    break;
                } else if (peek_ahead == '}') {
                    peek_ahead_lvl--;
                } else if (peek_ahead == '{') {
                    peek_ahead_lvl++;
                }
            }
        }
        fseek(parse_state->fp, parse_state->loc, 0);
        if (tag_valid != 1) {
            return;
        }

        parse_state->acc_state = ACC_SUBMIT;
        parse_state->tag_count++;

        while (i < parse_state->args_len) {
            if (parse_state->args_stack[i].arg_scope > -1 && parse_state->loc > parse_state->args_stack[i].arg_scope) {
                parse_state->args_len = i;
                break;
            }
            i++;
        }

        break;
    case RULES_ARGS:
        switch (parse_state->acc_state) {
        case ACC_NOOP:
            if (c == ' ' || c == '\t' || c == '\n') {
                return;
            }

            parse_state->acc[0] = c;
            parse_state->acc[1] = '\0';
            parse_state->acc_len = 1;
            parse_state->acc_state = ACC_INCOMPLETE;

            break;
        case ACC_INCOMPLETE:
            if (c == ',' || c == ')') {
                while (i <= parse_state->acc_len) {
                    parse_state->args_stack[parse_state->args_len].arg[i] = parse_state->acc[i];
                    parse_state->args_stack[parse_state->args_len].arg_scope = -1;
                    i++;
                }
                parse_state->args_len++;
                parse_state->acc_len = 0;

                if (c == ',') {
                    parse_state->acc_state = ACC_NOOP;
                } else if (c == ')') {
                    parse_state->acc_state = ACC_SUBMIT;
                }

                return;
            }

            parse_state->acc[parse_state->acc_len] = c;
            parse_state->acc_len++;
            parse_state->acc[parse_state->acc_len] = '\0';

            break;
        case ACC_SUBMIT:

            parse_state->rules_state = RULES_BODY;
            parse_state->acc_state = ACC_NOOP;

            libnonotes_parseC(parse_state, c);
            return;

            break;
        case ACC_COMPLETE:
            assert(0 && "unreachable");
            break;
        }
        break;
    case RULES_BODY:
        switch (parse_state->acc_state) {
        case ACC_NOOP:
            if (c == '{') {
                parse_state->acc_state = ACC_INCOMPLETE;
                parse_state->bracket_level = 0;
            }
            break;
        case ACC_INCOMPLETE:
            parse_state->acc[0] = c;
            parse_state->acc[1] = '\0';
            parse_state->body_len++;
            if (c == '}' && parse_state->bracket_level <= 0) {
                while (i <= parse_state->args_len) {
                    if (parse_state->args_stack[i].arg_scope == -1) {
                        parse_state->args_stack[i].arg_scope = parse_state->loc;
                    }
                    i++;
                }

                parse_state->acc_state = ACC_COMPLETE;
                parse_state->acc[0] = '\0';
                parse_state->loc = parse_state->loc - parse_state->body_len;
                fseek(parse_state->fp, parse_state->loc, 0);
                return;
            } else if (c == '{') {
                parse_state->bracket_level++;
            } else if (c == '}') {
                parse_state->bracket_level--;
            }
            parse_state->acc_state = ACC_SUBMIT;
            break;
        case ACC_SUBMIT:
            parse_state->acc_state = ACC_INCOMPLETE;
            libnonotes_parseC(parse_state, c);
            break;
        case ACC_COMPLETE:
            parse_state->rules_state = RULES_NOOP;
            parse_state->acc_state = ACC_NOOP;
        }
    }
}
