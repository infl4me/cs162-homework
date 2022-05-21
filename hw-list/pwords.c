/*
 * Word count application with one thread per input file.
 *
 * You may modify this file in any way you like, and are expected to modify it.
 * Your solution must read each input file from a separate thread. We encourage
 * you to make as few changes as necessary.
 */

/*
 * Copyright © 2021 University of California, Berkeley
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <ctype.h>
#include <dirent.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "word_count.h"
#include "word_helpers.h"

struct thread_info {
  char *filepath;
  int id;
  pthread_t pth;
  word_count_list_t *wc_list;
};

void *count_words_thread(void *arg) {
  struct thread_info *thread_info = (struct thread_info *)arg;
  FILE *fp = fopen(thread_info->filepath, "r");
  count_words(thread_info->wc_list, fp);
  fclose(fp);
  pthread_exit(NULL);
}

/*
 * main - handle command line, spawning one thread per file.
 */
int main(int argc, char *argv[]) {
  /* Create the empty data structure. */
  word_count_list_t word_counts;
  init_words(&word_counts);

  if (argc <= 1) {
    /* Process stdin in a single thread. */
    count_words(&word_counts, stdin);
  } else {
    int MAX_FILES_NUM = 50;
    int t;
    char filepath[100];
    struct thread_info *threads[MAX_FILES_NUM];
    DIR *d = opendir(argv[1]);
    struct dirent *dir = readdir(d);
    for (t = 0; dir != NULL; dir = readdir(d), t++) {
      if (dir->d_name[0] != '.') {
        strcpy(filepath, argv[1]);
        strcat(filepath, "/");
        strcat(filepath, dir->d_name);
        threads[t] = malloc(sizeof(struct thread_info));
        threads[t]->filepath = strcpy((char *)malloc(strlen(filepath) + 1), filepath);
        threads[t]->id = t;
        threads[t]->wc_list = &word_counts;
        pthread_create(&(threads[t]->pth), NULL, count_words_thread,
                            threads[t]);
      } else {
        t--;
      }
    }

    for (int i = 0; i < t; i++) {
      pthread_join(threads[i]->pth, NULL);
    }
  }

  /* Output final result of all threads' work. */
  wordcount_sort(&word_counts, less_count);
  fprint_words(&word_counts, stdout);
  return 0;
}
