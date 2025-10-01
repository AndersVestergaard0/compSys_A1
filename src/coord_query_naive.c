#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>
#include <math.h>

#include "record.h"
#include "coord_query.h"

struct naive_data {
  struct record *rs;
  int n;
};

struct naive_data* mk_naive(struct record* rs, int n);
void free_naive(struct naive_data* data);
const struct record* lookup_naive(struct naive_data *data, double lon, double lat);
double calculate_euclidean_distance(struct record *start_r, double target_lon, double target_lat);

int main(int argc, char** argv) {
  return coord_query_loop(argc, argv,
                          (mk_index_fn)mk_naive,
                          (free_index_fn)free_naive,
                          (lookup_fn)lookup_naive);
}

struct naive_data* mk_naive(struct record* rs, int n) {
  if (rs == NULL || n <= 0) {
    return NULL;
  }

  struct naive_data *data = malloc(sizeof(struct naive_data));

  if (data == NULL) {
    return NULL;
  }

  data->rs = rs;
  data->n = n;
  
  return data;
}

void free_naive(struct naive_data* data) {
  if (data == NULL) {
    return;
  }

  free(data);
}

const struct record* lookup_naive(struct naive_data *data, double lon, double lat) {
  if (data == NULL || data->rs == NULL || data->n <= 0) {
    return NULL;
  }

  struct record* closest_r = &data->rs[0];
  double closest_dist = calculate_euclidean_distance(closest_r, lon, lat);
  
  for (int i = 1; i < data->n; i++) {
    struct record *current_r = &data->rs[i];
    double dist = calculate_euclidean_distance(current_r, lon, lat);

    if (dist < closest_dist) {
      closest_r = current_r;
      closest_dist = dist;
    }
  }

  return closest_r;
}

double calculate_euclidean_distance(struct record *start_r, double target_lon, double target_lat) {
  double dx = start_r->lon - target_lon;
  double dy = start_r->lat - target_lat;

  double dist = sqrt(dx * dx + dy * dy);

  return dist;
}