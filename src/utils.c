/* This program is protected under the GNU GPL (See COPYING) */

#include "defs.h"

void cat_sprintf(char *dest, char *fmt, ...) {
  char buf[BUFFER_SIZE * 2];
  va_list args;

  va_start(args, fmt);
  vsprintf(buf, fmt, args);
  va_end(args);

  strcat(dest, buf);
}

void convert_meta(char *input, char *output) {
  char *pti = input, *pto = output;

  while (*pti) {
    switch (*pti) {
    case ESCAPE:
      *pto++ = '\\';
      *pto++ = 'e';
      pti++;
      break;
    case 127:
      *pto++ = '\\';
      *pto++ = 'b';
      pti++;
      break;
    case '\a':
      *pto++ = '\\';
      *pto++ = 'a';
      pti++;
      break;
    case '\b':
      *pto++ = '\\';
      *pto++ = 'b';
      pti++;
      break;
    case '\t':
      *pto++ = '\\';
      *pto++ = 't';
      pti++;
      break;
    case '\r':
      *pto++ = '\\';
      *pto++ = 'r';
      pti++;
      break;
    case '\n':
      *pto++ = *pti++;
      break;
    default:
      if (*pti > 0 && *pti < 32) {
        *pto++ = '\\';
        *pto++ = 'c';
        if (*pti <= 26) {
          *pto++ = 'a' + *pti - 1;
        } else {
          *pto++ = 'A' + *pti - 1;
        }
        pti++;
        break;
      } else {
        *pto++ = *pti++;
      }
      break;
    }
  }
  *pto = 0;
}

void display_printf(char *format, ...) {
  char buf[BUFFER_SIZE * 4];
  va_list args;

  va_start(args, format);
  vsprintf(buf, format, args);
  va_end(args);

  buf[strlen(buf)] = '\r';
  buf[strlen(buf)] = '\n';

  write(STDERR_FILENO, buf, strlen(buf));
}

/* The outer-most braces (if any) are stripped; all else left as is */
char *get_arg(char *string, char *result) {
  char *pti, *pto, output[BUFFER_SIZE];

  /* advance to the next none-space character */
  pti = string;
  pto = output;

  while (isspace((int)*pti)) {
    pti++;
  }

  /* Use a space as the separator if not wrapped with braces */
  if (*pti != DEFAULT_OPEN) {
    while (*pti) {
      if (isspace((int)*pti)) {
        pti++;
        break;
      }
      *pto++ = *pti++;
    }
  } else {
    int nest = 1;

    pti++; /* Advance past the DEFAULT_OPEN (nest is 1 for this reason) */

    while (*pti) {
      if (*pti == DEFAULT_OPEN) {
        nest++;
      } else if (*pti == DEFAULT_CLOSE) {
        nest--;

        /* Stop once we've met the closing backet for the openning we advanced
         * past before this loop */
        if (nest == 0) {
          break;
        }
      }
      *pto++ = *pti++;
    }

    if (*pti == 0) {
      display_printf("ERROR: Missing closing bracket");
    } else {
      pti++;
    }
  }

  *pto = '\0';

  strcpy(result, output);
  return pti;
}

/* TRUE if s1 is an abbrevation of s2 (case-insensitive) */
int is_abbrev(char *s1, char *s2) {
  if (*s1 == 0) {
    return FALSE;
  }
  return !strncasecmp(s2, s1, strlen(s1));
}

/* if wait_for_new_line, will process all lines until the one without \n at the
 * end */
void process_input(int wait_for_new_line) {
  char *line, *next_line;

  gd.input_buffer[gd.input_buffer_length] = 0;

  /* separate into lines and print away */
  for (line = gd.input_buffer; line && *line; line = next_line) {
    char linebuf[INPUT_MAX];

    next_line = strchr(line, '\n');

    if (next_line) {
      *next_line = 0; /* Replace \n with a null-terminator */
      next_line++;    /* Move the pointer to just after that \n */
    } else {          /* Reached the last line */
      if (wait_for_new_line) {
        char temp[INPUT_MAX];

        strcpy(temp, line);
        strcpy(gd.input_buffer, temp);
        gd.input_buffer_length = (int)strlen(temp);

        /* Leave and wait until called again without having to wait */
        return;
      }
    }

    /* Print the output after processing it */
    strcpy(linebuf, line);

    if (HAS_BIT(gd.flags, SES_FLAG_HIGHLIGHT)) {
      check_all_highlights(linebuf);
    }

    if (HAS_BIT(gd.flags, SES_FLAG_CONVERTMETA)) {
      char wrapped_str[BUFFER_SIZE * 2];

      convert_meta(linebuf, wrapped_str);
      printf("%s", wrapped_str);
    } else {
      printf("%s", linebuf);
    }

    if (next_line) {
      printf("\n");
    }

    fflush(stdout);
  }

  /* If we reached this point, then there's no more output in the buffer; reset
   * the length */
  gd.input_buffer_length = 0;
}

void script_driver(char *str) {
  char *pti = str;

  /* Skip any unnecessary command chars or spaces before the actual command */
  while (*pti == gd.command_char || isspace((int)*pti)) {
    pti++;
  }

  if (*pti != 0) {
    char args[BUFFER_SIZE], command[BUFFER_SIZE];
    int cmd;

    strcpy(args, get_arg(pti, command));

    for (cmd = 0; *command_table[cmd].name != 0; cmd++) {
      if (is_abbrev(command, command_table[cmd].name)) {
        (*command_table[cmd].command)(args);
        return;
      }
    }

    display_printf("ERROR: Unknown command '%s'", command);
  }
}
