// This program is protected under the GNU GPL (See COPYING)

#include "defs.h"

DO_COMMAND(do_commands) {
  char buf[BUFFER_SIZE] = {0}, add[BUFFER_SIZE];
  int cmd;

  display_header(ses, " %s ", "COMMANDS");

  for (cmd = 0; *command_table[cmd].name != 0; cmd++) {
    if (*arg && !is_abbrev(arg, command_table[cmd].name)) {
      continue;
    }
    if ((int)strlen(buf) + 20 > ses->cols) {
      display_puts2(ses, buf);
      buf[0] = 0;
    }
    sprintf(add, "%20s", command_table[cmd].name);
    strcat(buf, add);
  }
  if (buf[0]) {
    display_puts2(ses, buf);
  }
  display_header(ses, "");

  return ses;
}

DO_COMMAND(do_exit) {
  if (*arg) {
    quitmsg(arg);
  } else {
    quitmsg(NULL);
  }
  return NULL;
}

DO_COMMAND(do_showme) {
  char left[BUFFER_SIZE], right[BUFFER_SIZE], temp[STRING_SIZE];
  int lnf;

  arg = get_arg_in_braces(ses, arg, left, TRUE);

  lnf = (strlen(left) >= strlen("\\") &&
         !strcasecmp(left + strlen(left) - strlen("\\"), "\\"))
            ? FALSE
            : TRUE;

  substitute(ses, left, temp, SUB_VAR | SUB_FUN);
  substitute(ses, temp, left, SUB_COL | SUB_ESC);

  do_one_line(left, ses);

  if (strip_vt102_strlen(ses, ses->more_output) != 0) {
    sprintf(right, "\n\033[0m%s\033[0m", left);
  } else {
    sprintf(right, "\033[0m%s\033[0m", left);
  }

  if (ses != gtd->ses) {
    return ses;
  }

  printline(ses, right, lnf);

  return ses;
}

DO_COMMAND(do_zap) {
  struct session *sesptr = ses;
  char left[BUFFER_SIZE];

  sub_arg_in_braces(ses, arg, left, GET_ALL, SUB_VAR | SUB_FUN);

  sesptr = ses;

  if (*left) {
    for (sesptr = gts->next; sesptr; sesptr = sesptr->next) {
      if (!strcmp(sesptr->name, left)) {
        break;
      }
    }

    if (sesptr == NULL) {
      display_puts2(ses, "#NO SESSION WITH THAT NAME!");

      return ses;
    }
  }

  if (sesptr == gts) {
    return do_exit(NULL, "");
  }
  cleanup_session(sesptr);

  return gtd->ses;
}
