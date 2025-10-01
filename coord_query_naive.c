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

struct naive_data* mk_naive(struct record* rs, int n) {
  if (rs == NULL || n <= 0)
  {
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

double distance(struct record destination, double goal_lon, double goal_lat ){
  double dx = destination.lon - goal_lon;
  double dy = destination.lat - goal_lat;

  double dist = sqrt(dx * dx + dy * dy);

  return dist;
}

const struct record* lookup_naive(struct naive_data *data, double lon, double lat) {
  if (data == NULL || data->rs == NULL || data->n <= 0) {
    return NULL;
  }

  struct record* closest = &data->rs[0];
  
  for (int i = 0; i < data->n; i++) {
    if (distance(data->rs[i], lon, lat) < distance(*closest, lon, lat))
    {
      closest = &data->rs[i];
    }

    return closest;

    }
    return NULL;
}

int main(int argc, char** argv) {
  return coord_query_loop(argc, argv,
                          (mk_index_fn)mk_naive,
                          (free_index_fn)free_naive,
                          (lookup_fn)lookup_naive);
}
