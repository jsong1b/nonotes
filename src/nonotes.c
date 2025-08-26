#include <stdint.h>
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
    ParseState parse_state = NO_STATE;
    char buf[BUF_SIZE] = "";
    char buf_idx = 0;

    int tag_body_level = 0;
    char c = '\0';
    while ((c = fgetc(stdin)) != EOF) {
        buf[buf_idx] = c;
        buf_idx = (buf_idx + 1 >= BUF_SIZE) ? 0 : buf_idx + 1;
        buf[buf_idx] = '\0';

        switch (parse_state) {
        case NO_STATE:
            if (c != '(') continue;

            char in_tag = 1;
            int nntag_check_idx = 0;
            while (nntag_str[nntag_check_idx] != '\0') {
                int peek_behind_idx = buf_idx - (NNTAG_STR_LEN + nntag_check_idx);
                peek_behind_idx = (nntag_check_idx < 0)
                    ? BUF_SIZE - nntag_check_idx : nntag_check_idx;
                char peek_behind = buf[peek_behind_idx];

                if (nntag_str[nntag_check_idx] != peek_behind) {
                    in_tag = 0;
                    break;
                }
                nntag_check_idx++;
            }
            if (in_tag == 1) parse_state = IN_TAG_ARGS;
            break;
        case IN_TAG_ARGS:
            if (c == '(') parse_state = NO_STATE;
            else if (c == ')' || c == ',') {
                int peek_behind_len = 2; /* 2 because buf_idx is the index for
                                            \0, not the index for `c` */
                int peek_behind_idx = (buf_idx - peek_behind_len == -1) ?
                    BUF_SIZE - 1 : buf_idx - peek_behind_len;
                char peek_behind = buf[peek_behind_idx];

                int valid_arg = 1;
                while (peek_behind != ',' && peek_behind != '(') {
                    if (peek_behind == '\0') {
                        valid_arg = 0;
                        break;
                    }

                    peek_behind_len++;
                    peek_behind_idx = (buf_idx - peek_behind_len == -1)
                        ? BUF_SIZE - 1 : buf_idx - peek_behind_len;
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
                }

                if (c == ')') parse_state = BEFORE_TAG_BODY;
            }

            break;
        case BEFORE_TAG_BODY:
            if (c == ' ' || c == '\n' || c == '\t') continue;
            else if (c == '{') parse_state = IN_TAG_BODY;
            else parse_state = NO_STATE;
            break;
        case IN_TAG_BODY:
            if (c == '}' && tag_body_level <= 0) parse_state = NO_STATE;
            else if (c == '{') tag_body_level++;
            else if (c == '}') tag_body_level--;
            break;
        }
    }

    return 0;
}
