/*

Copyright © 2019 University of California, Berkeley

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

word_count provides lists of words and associated count

Functional methods take the head of a list as first arg.
Mutators take a reference to a list as first arg.
*/

#include "word_count.h"

/* Basic utililties */

char *new_string(char *str) {
  return strcpy((char *)malloc(strlen(str) + 1), str);
}

void init_words(WordCount **wclist) {
  /* Initialize word count.  */
  *wclist = NULL;
}

size_t len_words(WordCount *wchead) {
  size_t len = 0;
  WordCount *wc = wchead;

  while (wc != NULL) {
    len += 1;
    wc = wc->next;
  }

  return len;
}

WordCount *find_word(WordCount *wchead, char *word) {
  WordCount *wc = wchead;

  while (wc != NULL) {
    if (strcmp(wc->word, word) == 0) {
      return wc;
    }
    wc = wc->next;
  }

  return wc;
}

void add_word(WordCount **wclist, char *word) {
  /* If word is present in word_counts list, increment the count, otw insert
   * with count 1. */
  WordCount *wc = *wclist;

  if (wc == NULL) {
    wc = malloc(sizeof(WordCount));
    wc->count = 1;
    wc->word = new_string(word);
    wc->next = NULL;

    *wclist = wc;

    return;
  }

  wc = find_word(*wclist, word);
  if (wc != NULL) {
    wc->count += 1;

    return;
  }

  wc = *wclist;
  while (wc->next != NULL) {
    wc = wc->next;
  }

  wc->next = (WordCount *)malloc(sizeof(WordCount));
  wc->next->count = 1;
  wc->next->word = new_string(word);
  wc->next->next = NULL;

  return;
}

void fprint_words(WordCount *wchead, FILE *ofile) {
  /* print word counts to a file */
  WordCount *wc;
  for (wc = wchead; wc; wc = wc->next) {
    fprintf(ofile, "%i\t%s\n", wc->count, wc->word);
  }
}
