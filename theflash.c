#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define CITIES_LENGTH 7
#define NUM_CITIES (CITIES_LENGTH - 1)

static const char *cities[] = {"Central City", "Starling City",
                               "Gotham City",  "Metropolis",
                               "Coast City",   "National City"};

const int distances[CITIES_LENGTH - 1][CITIES_LENGTH - 1] = {
    {0, 793, 802, 254, 616, 918}, {793, 0, 197, 313, 802, 500},
    {802, 197, 0, 496, 227, 198}, {254, 313, 496, 0, 121, 110},
    {616, 802, 227, 121, 0, 127}, {918, 500, 198, 110, 127, 0}};

int initial_vector[CITIES_LENGTH] = {0, 1, 2, 3, 4, 5, 0};

typedef struct {
  int r;
  int w;
} pipe_t;

typedef struct {
  int cities[CITIES_LENGTH];
  int total_dist;
} route;

int pipe_constructor(pipe_t *p) {
  int file_descriptors[2];
  if (pipe(file_descriptors) == -1) {
    return -1;
  }
  p->r = file_descriptors[0];
  p->w = file_descriptors[1];
  return 0;
}

void print_route(route *r) {
  printf("Route: ");
  for (int i = 0; i < CITIES_LENGTH; i++) {
    if (i == CITIES_LENGTH - 1) {
      printf("%s\n", cities[r->cities[i]]);
    } else {
      printf("%s - ", cities[r->cities[i]]);
    }
  }
}

void calculate_distance(route *r) {
  if (r->cities[0] != 0) {
    printf("Route must start with %s (but was %s)!\n", cities[0],
           cities[r->cities[0]]);
    exit(-1);
  }
  if (r->cities[6] != 0) {
    printf("Route must end with %s (but was %s)!\n", cities[0],
           cities[r->cities[6]]);
    exit(-2);
  }
  int distance = 0;
  for (int i = 1; i < CITIES_LENGTH; i++) {
    int to_add = distances[r->cities[i - 1]][r->cities[i]];
    if (to_add == 0) {
      printf("Route cannot have a zero distance segment.\n");
      exit(-3);
    }
    distance += to_add;
  }
  r->total_dist = distance;
}

void swap(int *a, int *b) {
  int tmp = *a;
  *a = *b;
  *b = tmp;
}

int handle_parent_process(pipe_t p) {
  close(p.w);
  int distance;
  read(p.r, &distance, sizeof(distance));
  close(p.r);
  return distance;
}

void handle_child_process(route *r, pipe_t p) {
  close(p.r);
  calculate_distance(r);
  if (write(p.w, &r->total_dist, sizeof(r->total_dist)) == -1) {
    exit(EXIT_FAILURE);
  }
  close(p.w);
  exit(EXIT_SUCCESS);
}

void permute(route *r, int left, int right, route *best) {
  if (left == right) {
    pipe_t p;
    int pipe_status = pipe_constructor(&p);
    if (pipe_status == -1) {
      exit(EXIT_FAILURE);
    }
    switch (fork()) {
    case 0:
      handle_child_process(r, p);
      break;
    case -1:
      exit(EXIT_FAILURE);
    default:
      wait(NULL);
      r->total_dist = handle_parent_process(p);
      if (r->total_dist < best->total_dist) {
        memcpy(best, r, sizeof(route));
        best->total_dist = r->total_dist;
      }
      break;
    }
    return;
  }

  for (int i = left; i <= right; i++) {
    swap(&r->cities[left], &r->cities[i]);
    permute(r, left + 1, right, best);
    swap(&r->cities[left], &r->cities[i]);
  }
}

void assign_best(route **best, route *candidate) {
  if (*best == NULL) {
    *best = candidate;
    return;
  }

  int a = candidate->total_dist;
  int b = (*best)->total_dist;

  if (a < b) {
    free(*best);
    *best = candidate;
  } else {
    free(candidate);
  }
}

route *find_best_route() {
  route *candidate = malloc(sizeof(route));
  memcpy(candidate->cities, initial_vector, CITIES_LENGTH * sizeof(int));
  candidate->total_dist = 0;

  route *best = malloc(sizeof(route));
  memset(best, 0, sizeof(route));
  best->total_dist = 999999;

  permute(candidate, 1, 5, best);

  free(candidate);
  return best;
}

int main(int argc, char **argv) {
  route *best = find_best_route();
  print_route(best);
  printf("Distance: %d\n", best->total_dist);
  free(best);
  return 0;
}
