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
  returns full path to the program
  if program_name already exists returns it
  otherwise looks for the program in PATH
 */
char* get_program_path(char* program_name) {
  if (access(program_name, F_OK) == 0) {
    return program_name;
  }

  char* path_dirs = getenv("PATH");
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
      return program_path;
    }

    token = strtok(NULL, ":");
  }

  return NULL;
}

/* forks, execs and waits for passed program  */
void exec_program(struct tokens* tokens) {
  pid_t pid = fork();
  int status;

  if (pid == -1) {
    printf("Program failed: %s\n", strerror(errno));
  } else if (pid == 0) {
    int ARGS_MAX_SIZE = 30, i, tokens_length = tokens_get_length(tokens);
    char* args[ARGS_MAX_SIZE];
    char* program_path = get_program_path(tokens_get_token(tokens, 0));

    if (program_path == NULL) {
      printf("File not found\n");
      exit(EXIT_FAILURE);
    }

    int has_stdin_redirect =
        tokens_length > 2 && tokens_get_token(tokens, tokens_length - 2)[0] == '<';
    int has_stdout_redirect =
        tokens_length > 2 && tokens_get_token(tokens, tokens_length - 2)[0] == '>';
    int has_redirect = has_stdin_redirect || has_stdout_redirect;

    if (has_redirect) {
      int fd = open(tokens_get_token(tokens, tokens_length - 1), O_RDWR);
      if (fd == -1) {
        exit(EXIT_FAILURE);
      }

      if (has_stdout_redirect) {
        dup2(fd, STDOUT_FILENO);
      } else if (has_stdin_redirect) {
        args[0] = tokens_get_token(tokens, 0);
        args[1] = NULL;
        dup2(fd, STDIN_FILENO);
      }
    }

    if (!has_stdin_redirect) {
      for (i = 0; i < tokens_length; i++) {
        args[i] = tokens_get_token(tokens, i);
      }
      args[i] = NULL;
    }

    execv(program_path, args);
    exit(EXIT_FAILURE);
  } else {
    waitpid(pid, &status, 0);
    if (status != EXIT_SUCCESS) {
      printf("Program failed: %d\n", status);
    }
  }
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
      exec_program(tokens);
    }

    if (shell_is_interactive)
      /* Please only print shell prompts when standard input is not a tty */
      fprintf(stdout, "%d: ", ++line_num);

    /* Clean up memory */
    tokens_destroy(tokens);
  }

  return 0;
}
