#include <stdio.h>

#define BUF_SIZE 128

#define NNTAG_STR_LEN 6
const char nntag_str[NNTAG_STR_LEN] = "nntag";

typedef enum ParseStateOld {
    NO_STATE,
    IN_TAG_ARGS,
    BEFORE_TAG_BODY,
    IN_TAG_BODY
} ParseStateOld;

typedef enum {
    PEEK_AHEAD_TAGS,
    PEEK_AHEAD_BEFORE_BODY,
    PEEK_AHEAD_BODY
} PeekAheadStage;

typedef struct ParseState {
    enum {
        RULES_NOOP,
        RULES_ARGS,
        RULES_BODY
    } rules_state;

    char acc[BUF_SIZE];
    int acc_idx;
    enum {
        ACC_NOOP,
        ACC_COMPLETE,
        ACC_INCOMPLETE
    } acc_state;

    FILE* fp;
    int loc;

    char buf[BUF_SIZE];
    int buf_idx;

    int bracket_level;
} ParseState;

void parseC(ParseState *parse_state, char c);



int main(void) {
    char c = '\0';

    FILE* fp = tmpfile();
    if (fp == NULL) return 1;
    while ((c = fgetc(stdin)) != EOF) fputc(c, fp);
    fputc(EOF, fp);

    ParseStateOld parse_state_old = NO_STATE;
    char buf[BUF_SIZE] = "";
    int buf_idx = 0;
    int loc = 0;
    int tag_count = 0;
    int tag_body_level = 0;


    ParseState parse_state = { 0 };
    parse_state.fp = fp;
    fseek(fp, 0, 0);
    while ((c = fgetc(fp)) != EOF) {
        parseC(&parse_state, c);

        printf("\n==========");
        printf("char: %c", c);
        printf("\nfile_loc: %d", parse_state.loc);
        printf("\nacc_idx: %d", parse_state.acc_idx);
        printf("\nbuf_idx: %d", parse_state.buf_idx);
        printf("\nrules: ");
        switch (parse_state.rules_state) {
        case RULES_NOOP: printf("noop"); break;
        case RULES_ARGS: printf("args"); break;
        case RULES_BODY: printf("body"); break;
        }
        printf("\nacc state: ");
        switch (parse_state.acc_state) {
        case ACC_NOOP: printf("noop"); break;
        case ACC_COMPLETE: printf("complete"); break;
        case ACC_INCOMPLETE: printf("incomplete"); break;
        }
        printf("\nacc:\n%s\n", parse_state.acc);
    }

    fclose(fp);
    return 0;
}

void parseC(ParseState* parse_state, char c) {
    parse_state->loc++;
    parse_state->buf[parse_state->buf_idx] = c;
    parse_state->buf_idx = (parse_state->buf_idx + 1 >= BUF_SIZE) ? 0 : parse_state->buf_idx + 1;
    parse_state->buf[parse_state->buf_idx] = '\0';

    switch (parse_state->rules_state) {
    case RULES_NOOP:
        if (c != '(') {
            return;
        }
        int tag_valid = 0;

        /* check behind '(' for the "nntag" string */
        int nntag_check_idx = 0;
        while (nntag_str[nntag_check_idx] != '\0') {
            int peek_behind_idx = parse_state->buf_idx - (NNTAG_STR_LEN + nntag_check_idx);
            peek_behind_idx = (nntag_check_idx < 0) ? BUF_SIZE - nntag_check_idx : nntag_check_idx;
            char peek_behind = parse_state->buf[peek_behind_idx];

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

        parse_state->rules_state = RULES_ARGS;

        break;
    case RULES_ARGS:
        switch (parse_state->acc_state) {
        case ACC_NOOP:
            if (c == ' ' || c == '\t' || c == '\n') {
                return;
            }
            parse_state->acc[0] = c;
            parse_state->acc[1] = '\0';
            parse_state->acc_idx = 1;
            parse_state->acc_state = ACC_INCOMPLETE;

            break;
        case ACC_INCOMPLETE:
            if (c == ',' || c == ')') {
                if (c == ',') {
                    parse_state->acc_state = ACC_NOOP;
                } else if (c == ')') {
                    parse_state->acc_state = ACC_COMPLETE;
                }
                return;
            }
            parse_state->acc[parse_state->acc_idx] = c;
            parse_state->acc_idx++;
            parse_state->acc[parse_state->acc_idx] = '\0';

            break;
        case ACC_COMPLETE:
            if (c == '{') {
                parse_state->rules_state = RULES_BODY;
                parse_state->acc_idx = 0;
                parse_state->acc_state = ACC_NOOP;
            }

            break;
        }
        break;
    case RULES_BODY:
        if (c == '{' && parse_state->bracket_level == 0) {
            parse_state->rules_state = RULES_NOOP;
            parse_state->acc_state = ACC_COMPLETE;
            return;
        } else if (c == '{') {
            parse_state->bracket_level++;
        } else if ('}') {
            parse_state->bracket_level--;
        }

        break;
    }
}
