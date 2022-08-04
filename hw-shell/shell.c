#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "tokenizer.h"

/* Convenience macro to silence compiler warnings about unused function parameters. */
#define unused __attribute__((unused))

/* Whether the shell is connected to an actual terminal or not. */
bool shell_is_interactive;

/* File descriptor for the shell input */
int shell_terminal;

/* Terminal mode settings for the shell */
struct termios shell_tmodes;

/* Process group id for the shell */
pid_t shell_pgid;

int cmd_exit(struct tokens* tokens);
int cmd_help(struct tokens* tokens);
int cmd_pwd(struct tokens* tokens);
int cmd_cd(struct tokens* tokens);

/* Built-in command functions take token array (see parse.h) and return int */
typedef int cmd_fun_t(struct tokens* tokens);

/* Built-in command struct and lookup table */
typedef struct fun_desc {
  cmd_fun_t* fun;
  char* cmd;
  char* doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {{cmd_help, "?", "show this help menu"},
                          {cmd_exit, "exit", "exit the command shell"},
                          {cmd_pwd, "pwd", "print current diractory"},
                          {cmd_cd, "cd", "change working directory"}};

/* Prints a helpful description for the given command */
int cmd_help(unused struct tokens* tokens) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    printf("%s - %s\n", cmd_table[i].cmd, cmd_table[i].doc);
  return 1;
}

/* Exits this shell */
int cmd_exit(unused struct tokens* tokens) { exit(0); }

/* Prints the current working directory */
int cmd_pwd(unused struct tokens* tokens) {
  int SIZE = 256;
  char buffer[SIZE];

  char* result_buffer = getcwd(buffer, SIZE);
  if (result_buffer == NULL) {
    printf("pwd failed\n");
    return 0;
  }

  printf("%s\n", buffer);

  return 1;
}

/* Changes working directory */
int cmd_cd(struct tokens* tokens) {
  if (chdir(tokens_get_token(tokens, 1)) < 0) {
    printf("failed to change directory\n");
    return 0;
  }

  return 1;
}

/* Looks up the built-in command, if it exists. */
int lookup(char cmd[]) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0))
      return i;
  return -1;
}

/* Intialization procedures for this shell */
void init_shell() {
  /* Our shell is connected to standard input. */
  shell_terminal = STDIN_FILENO;

  /* Check if we are running interactively */
  shell_is_interactive = isatty(shell_terminal);

  if (shell_is_interactive) {
    /* If the shell is not currently in the foreground, we must pause the shell until it becomes a
     * foreground process. We use SIGTTIN to pause the shell. When the shell gets moved to the
     * foreground, we'll receive a SIGCONT. */
    while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
      kill(-shell_pgid, SIGTTIN);

    /* Saves the shell's process id */
    shell_pgid = getpid();

    /* Take control of the terminal */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* Save the current termios to a variable, so it can be restored later. */
    tcgetattr(shell_terminal, &shell_tmodes);
  }
}

/*
  resolves path to the program
  if given path doesn't exist tries to search the program in the PATH
 */
char* resolve_program_path(char* program_name) {
  if (access(program_name, F_OK) == 0) {
    return program_name;
  }

  char* path_dirs = malloc(1024);
  strcpy(path_dirs, getenv("PATH"));

  if (path_dirs == NULL) {
    return NULL;
  }

  char* token = strtok(path_dirs, ":");
  char* program_path = malloc(128);

  while (token != NULL) {
    strcpy(program_path, token);
    strcat(program_path, "/");
    strcat(program_path, program_name);

    if (access(program_path, F_OK) == 0) {
      free(path_dirs);
      return program_path;
    }

    token = strtok(NULL, ":");
  }

  free(path_dirs);

  return NULL;
}

void print_args(char** args) {
  for (int i = 0; i < 10; i++) {
    printf("arg [%d]: >>>%s<<<\n", i, args[i]);
    if (args[i] == NULL) {
      puts("=================================\n");
      return;
    }
  }
}

void run_program(char** args, int in_fd, int out_fd) {
  pid_t forked_pid = fork();

  if (forked_pid == -1) {
    printf("Failed to create new process: %s\n", strerror(errno));
    return;
  }

  if (forked_pid > 0) {
    int status;

    waitpid(forked_pid, &status, 0);

    if (status != EXIT_SUCCESS) {
      printf("Program failed: %d\n", status);
    }

    return;
  }

  if (forked_pid == 0) {
    if (in_fd) {
      dup2(in_fd, STDIN_FILENO);
    }

    if (out_fd) {
      dup2(out_fd, STDOUT_FILENO);
    }

    execv(resolve_program_path(args[0]), args);
    exit(EXIT_FAILURE);
  }
}

/*
  processes programs passed in form of tokens
  supports multiple pipelines, and in/out file redirects

  doesn't support a query mixed with pipelines and redirects
  the behavior is undefined
*/
void process_programs(struct tokens* tokens) {
  char* args[30];
  char* token;
  int pipe_fds[2];
  int token_i, arg_i = 0;
  bool piped = false;
  int in = 0, out = 0;

  if (pipe(pipe_fds) == -1) {
    printf("Failed to create new pipe\n");
    return;
  }

  for (token_i = 0; token_i < tokens_get_length(tokens); token_i++, arg_i++) {
    token = tokens_get_token(tokens, token_i);

    if (token[0] == '|') {
      pipe(pipe_fds);

      // save pipe's FDs to separate variables
      // since in case of multiple pipelines
      // we need to pass FDs from different pipes to some programs:
      // for example if we have two pipelines:
      //   - first program receives: stdin:stdin, stdout:pipe1[1]
      //   - second program receives: stdin:pipe1[0], stdout:pipe2[1]
      //   - third program receives: stdin:pipe2[0], stdout:stdout
      out = pipe_fds[1];

      args[arg_i] = NULL;
      run_program(args, in, out);
      close(out);

      if (in) {
        close(in);
      }
      in = pipe_fds[0];

      // reset arg counter to run next program
      arg_i = -1;
      piped = true;
    } else if (token[0] == '>') {
      // ignore redirect token and filepath
      token_i++;
      arg_i--;

      out = open(tokens_get_token(tokens, token_i), O_WRONLY | O_APPEND | O_CREAT, 0644);

      if (out == -1) {
        exit(EXIT_FAILURE);
      }
    } else if (token[0] == '<') {
      // ignore redirect token and filepath
      token_i++;
      arg_i--;

      in = open(tokens_get_token(tokens, token_i), O_RDWR | O_APPEND | O_CREAT, 0644);

      if (in == -1) {
        exit(EXIT_FAILURE);
      }
    } else {
      args[arg_i] = token;
    }
  }

  args[arg_i] = NULL;

  run_program(args, in, out);
}

int main(unused int argc, unused char* argv[]) {
  init_shell();

  static char line[4096];
  int line_num = 0;

  /* Please only print shell prompts when standard input is not a tty */
  if (shell_is_interactive)
    fprintf(stdout, "%d: ", line_num);

  while (fgets(line, 4096, stdin)) {
    /* Split our line into words. */
    struct tokens* tokens = tokenize(line);

    /* Find which built-in function to run. */
    int fundex = lookup(tokens_get_token(tokens, 0));

    if (fundex >= 0) {
      cmd_table[fundex].fun(tokens);
    } else {
      process_programs(tokens);
    }

    if (shell_is_interactive)
      /* Please only print shell prompts when standard input is not a tty */
      fprintf(stdout, "%d: ", ++line_num);

    /* Clean up memory */
    tokens_destroy(tokens);
  }

  return 0;
}
