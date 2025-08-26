#include <assert.h>
#include <stdio.h>

#define BUF_SIZE 128

#define NNTAG_STR_LEN 6
const char nntag_str[NNTAG_STR_LEN] = "nntag";

typedef enum ParseState {
    NO_STATE,
    IN_TAG_ARGS,
    BEFORE_TAG_BODY,
    IN_TAG_BODY
} ParseState;



int main(void) {
    char c = '\0';

    FILE* fp = tmpfile();
    if (fp == NULL) return 1;
    while ((c = fgetc(stdin)) != EOF) fputc(c, fp);
    fputc(EOF, fp);

    ParseState parse_state = NO_STATE;
    char buf[BUF_SIZE] = "";
    int buf_idx = 0;
    int loc = 0;
    int tag_count = 0;
    int tag_body_level = 0;

    fseek(fp, 0, 0);
    while ((c = fgetc(fp)) != EOF) {
        loc++;

        buf[buf_idx] = c;
        buf_idx = (buf_idx + 1 >= BUF_SIZE) ? 0 : buf_idx + 1;
        buf[buf_idx] = '\0';

        switch (parse_state) {
        case NO_STATE:
            if (c != '(') continue;
            int tag_valid = 0;

            /* check behind the "(" to see if it is preceded by "nntag" */
            int nntag_check_idx = 0;
            while (nntag_str[nntag_check_idx] != '\0') {
                int peek_behind_idx = buf_idx
                    - (NNTAG_STR_LEN + nntag_check_idx);
                peek_behind_idx = (nntag_check_idx < 0)
                    ? BUF_SIZE - nntag_check_idx
                    : nntag_check_idx;
                char peek_behind = buf[peek_behind_idx];

                if (nntag_str[nntag_check_idx] != peek_behind) {
                    tag_valid = 0;
                    break;
                }

                tag_valid = 1;
                nntag_check_idx++;
            }

            if (tag_valid != 1) continue;
            tag_valid = 0;
            /* check after the ")" to see if the tag is formatted properly */
            ParseState peek_ahead_state = IN_TAG_ARGS;
            int peek_ahead_level = 0;
            char peek_ahead;
            while ((peek_ahead = fgetc(fp)) != EOF) {
                /* using an if tree because a switch would need goto to break
                   out of the loop */
                if (peek_ahead_state == IN_TAG_ARGS) {
                    if (peek_ahead == '(') break;
                    else if (peek_ahead == ')')
                        peek_ahead_state = BEFORE_TAG_BODY;

                    /* TODO: check if arg is longer than the buffer size */
                } else if (peek_ahead_state == BEFORE_TAG_BODY) {
                    if (peek_ahead == '{') peek_ahead_state = IN_TAG_BODY;
                    else if (peek_ahead == ' '
                        || peek_ahead == '\t'
                        || peek_ahead == '\n')
                    continue;
                    else break;
                } else if (peek_ahead_state == IN_TAG_BODY) {
                    if (peek_ahead == '}' && peek_ahead_level <= 0) {
                        tag_valid = 1;
                        break;
                    } else if (peek_ahead == '{') {
                        peek_ahead_level++;
                    } else if (peek_ahead == '}') {
                        peek_ahead_level--;
                    }
                }
            }
            fseek(fp, loc, 0);
            if (tag_valid != 1) continue;

            tag_count++;
            fprintf(stdout, "========== nntag %d ==========\ntags: [", tag_count);
            parse_state = IN_TAG_ARGS;

            break;
        case IN_TAG_ARGS:
            if (c == ')' || c == ',') {
                int peek_behind_len = 2; /* 2 because buf_idx is the index for
                                            \0, not the index for `c` */
                int peek_behind_idx = (buf_idx - peek_behind_len < 0)
                    ? BUF_SIZE + buf_idx - peek_behind_len
                    : buf_idx - peek_behind_len;
                char peek_behind = buf[peek_behind_idx];

                int valid_arg = 1;
                while (peek_behind != ',' && peek_behind != '(') {
                    peek_behind_len++;
                    peek_behind_idx = (buf_idx - peek_behind_len < 0)
                        ? BUF_SIZE + buf_idx - peek_behind_len
                        : buf_idx - peek_behind_len;
                    peek_behind = buf[peek_behind_idx];
                }
                if (valid_arg != 1) {
                    parse_state = NO_STATE;
                    continue;
                }

                peek_behind_len--; /* remove first char from peek_behind */
                while (peek_behind_len > 1) {
                    int peek_behind_idx = (buf_idx - peek_behind_len < 0)
                    ? BUF_SIZE + (buf_idx - peek_behind_len)
                    : buf_idx - peek_behind_len;
                    char peek_behind = buf[peek_behind_idx];
                    peek_behind_len--;

                    if (peek_behind == ' ' || peek_behind == '\t'
                        || peek_behind == '\n') continue;
                    fprintf(stdout, "%c", peek_behind);
                }

                if (c == ',') {
                    fprintf(stdout, ", ");
                } else if (c == ')') {
                    fprintf(stdout, "]\ncontent:");
                    parse_state = BEFORE_TAG_BODY;
                }
            }
            break;
        case BEFORE_TAG_BODY:
            if (c == ' ' || c == '\n' || c == '\t') continue;
            else if (c == '{') parse_state = IN_TAG_BODY;
            else parse_state = NO_STATE;
            break;
        case IN_TAG_BODY:
            if (c == '}' && tag_body_level <= 0) {
                parse_state = NO_STATE;
                fprintf(stdout, "\n");
            } else {
                if (c == '\n') fprintf(stdout, "\n  ");
                else fprintf(stdout, "%c", c);

                if (c == '{') tag_body_level++;
                else if (c == '}') tag_body_level--;
            }

            break;
        }
    }

    fclose(fp);
    return 0;
}
