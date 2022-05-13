#include <stdio.h>
#include <sys/resource.h>

int main() {
  struct rlimit lim;
  getrlimit(RLIMIT_STACK, &lim);
  printf("stack size. cur: %llu; max: %llu\n", lim.rlim_cur, lim.rlim_max);
  getrlimit(RLIMIT_NPROC, &lim);
  printf("process limit: %llu\n", lim.rlim_max);
  getrlimit(RLIMIT_NOFILE, &lim);
  printf("max file descriptors: %llu\n", lim.rlim_max);
  return 0;
}