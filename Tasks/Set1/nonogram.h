#ifndef NONOGRAM_H
#define NONOGRAM_H

typedef struct run_t run_t;
typedef struct nonogram_t nonogram_t;

struct run_t {
  int length;
  int* run;
};

struct nonogram_t {
  int x_runs_length;
  int y_runs_length;
  run_t* x_runs;
  run_t* y_runs;
};

void print_solution(nonogram_t *png);
void solve_nonogram(nonogram_t *png, char *data);

nonogram_t *create_nonogram(char *data, int width, int height);
void free_nonogram(nonogram_t *png);

#endif