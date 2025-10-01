#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>

#include "record.h"
#include "id_query.h"

struct index_record {
  int64_t osm_id;
  const struct record *record;
};

struct binsort_data {
  struct index_record *irs;
  int n;
};

struct binsort_data* mk_binsort(struct record* rs, int n);
void sort_binsort(struct index_record* records, int n);
int compare_osm_id(const void *a, const void *b);
void free_binsort(struct binsort_data* data);
const struct record* lookup_binsort(struct binsort_data *data, int64_t needle);

int main(int argc, char** argv) {
  return id_query_loop(argc, argv,
                    (mk_index_fn)mk_binsort,
                    (free_index_fn)free_binsort,
                    (lookup_fn)lookup_binsort);
}

struct binsort_data* mk_binsort(struct record* rs, int n) {
  if (rs == NULL || n <= 0) {
    return NULL;
  }

  struct binsort_data* data = malloc(sizeof(struct binsort_data));

  if (data == NULL) {
    return NULL;
  }

  struct index_record* records = malloc(sizeof(struct index_record) * n);
  
  if (records == NULL) {
    free(data);
    return NULL;
  }

  for (int i = 0; i < n; i++) {
    records[i].osm_id = rs[i].osm_id;
    records[i].record = &rs[i];
  }

  sort_binsort(records, n);

  data->irs = records;
  data->n = n;

  return data;
}

void sort_binsort(struct index_record* records, int n) {
  qsort(records, n, sizeof(struct index_record), compare_osm_id);
}

int compare_osm_id(const void *a, const void *b) {
  const struct index_record *rec_a = (const struct index_record *)a;
  const struct index_record *rec_b = (const struct index_record *)b;

  if (rec_a->osm_id < rec_b->osm_id) {
    return -1;
  } else if (rec_a->osm_id > rec_b->osm_id) {
    return 1;
  } else {
    return 0;
  }
}

void free_binsort(struct binsort_data* data) {
  if (data == NULL) {
    return;
  }
  
  if (data->irs != NULL) {
    free(data->irs);
  }

  free(data);
}

const struct record* lookup_binsort(struct binsort_data *data, int64_t needle) {
  if (data == NULL || data->irs == NULL || data->n <= 0) {
    return NULL;
  }

  int i = 0, j = data->n - 1, mid;
  
  while (i <= j) {
    mid = i + (j - i) / 2;

    if (data->irs[mid].osm_id == needle) {
      return data->irs[mid].record;
    } else if (data->irs[mid].osm_id < needle) {
      i = mid + 1;
    } else {
      j = mid - 1;
    }
  }

  return NULL;
}