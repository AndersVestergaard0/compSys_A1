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

#define K 2

struct node {
  int axis;
  const struct record *r;
  struct node *left, *right;
};

struct kdtree_data {
  struct node *root;
};

struct query {
  double longitude;
  double latitude;
};

struct kdtree_data* mk_kdtree(struct record* rs, int n);
struct node *build_kdtree(struct record *rs, int n, int depth);
struct record* find_median(struct record *rs, int n, int axis);
int compare_records_lon(const void *a, const void *b);
int compare_records_lat(const void *a, const void *b);
void free_kdtree(struct kdtree_data* data);
void free_kdtree_node(struct node *node);
const struct record* lookup_kdtree(struct kdtree_data *data, double lon, double lat);
double calculate_euclidean_distance(const struct record *node_r, const struct query query);
void search_kdtree(struct node **closest_node, double *closest_dist, const struct query query, struct node *node);
double calculate_difference(const struct node *node, const struct query query);

int main(int argc, char** argv) {
  return coord_query_loop(argc, argv,
                          (mk_index_fn)mk_kdtree,
                          (free_index_fn)free_kdtree,
                          (lookup_fn)lookup_kdtree);
}

// The record reference is assumed not to be used elsewhere and the order of them is mutated 
struct kdtree_data* mk_kdtree(struct record* rs, int n) {
  if (rs == NULL || n <= 0) {
    return NULL;
  }

  struct kdtree_data *data = malloc(sizeof(struct kdtree_data));
  if (data == NULL) {
    return NULL;
  }

  data->root = build_kdtree(rs, n, 0);
  if (data->root == NULL) {
    free(data);
    return NULL;
  }

  return data;
}

struct node *build_kdtree(struct record *rs, int n, int depth) {
  if (rs == NULL || n <= 0) {
    return NULL;
  }

  int axis = depth % K;

  struct record *median = find_median(rs, n, axis);
  if (median == NULL) {
    return NULL;
  }

  struct node *node = malloc(sizeof(struct node));
  if (node == NULL) {
    return NULL;
  }

  node->axis = axis;
  node->r = median;

  // Number of elements to the left of median (excluding the median itself)
  int l = median - rs;

  // Number of elements to the right of median (excluding the median itself)
  int r = n - l - 1;
  
  node->left = build_kdtree(rs, l, depth + 1);
  node->right = build_kdtree(median + 1, r, depth + 1);

  return node;
}

struct record* find_median(struct record *rs, int n, int axis) {
  if (rs == NULL || n <= 0 || (axis != 0 && axis != 1)) {
    return NULL;
  }

  qsort(rs, n, sizeof(struct record), axis == 0 ? compare_records_lon : compare_records_lat);

  return &rs[n / 2];
}

int compare_records_lon(const void *a, const void *b) {
  const struct record *record_a = (const struct record *)a;
  const struct record *record_b = (const struct record *)b;

  if (record_a->lon < record_b->lon) {
    return -1;
  } else if (record_a->lon > record_b->lon) {
    return 1;
  } else {
    return 0;
  }
}

int compare_records_lat(const void *a, const void *b) {
  const struct record *record_a = (const struct record *)a;
  const struct record *record_b = (const struct record *)b;

  if (record_a->lat < record_b->lat) {
    return -1;
  } else if (record_a->lat > record_b->lat) {
    return 1;
  } else {
    return 0;
  }
}

void free_kdtree(struct kdtree_data* data) {
  if (data == NULL) {
    return;
  }

  if (data->root != NULL) {
    free_kdtree_node(data->root);
  }
  
  free(data);
}

void free_kdtree_node(struct node *node) {
  if (node == NULL) {
    return;
  }

  if (node->left != NULL) {
    free_kdtree_node(node->left);
  }

  if (node->right != NULL) {
    free_kdtree_node(node->right);
  }

  free(node);
}

const struct record* lookup_kdtree(struct kdtree_data *data, double lon, double lat) {
  if (data == NULL || data->root == NULL) {
    return NULL;
  }

  struct query query = {
    .longitude = lon, 
    .latitude = lat,
  };

  struct node *closest_node = data->root;
  double closest_dist = calculate_euclidean_distance(closest_node->r, query);

  search_kdtree(&closest_node, &closest_dist, query, data->root);

  return closest_node->r;
}

void search_kdtree(struct node **closest_node, double *closest_dist, const struct query query, struct node *node) {
  if (node == NULL) {
    return;
  }

  double dist = calculate_euclidean_distance(node->r, query);

  if (dist < *closest_dist) {
    *closest_node = node;
    *closest_dist = dist;
  }
 
  double diff = calculate_difference(node, query);
  double radius = *closest_dist;

  if (diff >= 0 || radius > fabs(diff)) {
    search_kdtree(closest_node, closest_dist, query, node->left);
  } 
  
  if (diff <= 0 || radius > fabs(diff)) {
    search_kdtree(closest_node, closest_dist, query, node->right);
  }
}

double calculate_euclidean_distance(const struct record *node_r, const struct query query) {
  double dx = node_r->lon - query.longitude;
  double dy = node_r->lat - query.latitude;

  double dist = sqrt(dx * dx + dy * dy);

  return dist;
}

double calculate_difference(const struct node *node, const struct query query) {
  if (node->axis == 0) { 
    return node->r->lon - query.longitude;
  } else {
    return node->r->lat - query.latitude;
  }
}