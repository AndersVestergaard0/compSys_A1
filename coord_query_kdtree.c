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
  double lon;
  double lat;
};

struct kdtree_data* mk_kdtree(struct record* rs, int n);
struct node *build_kdtree(struct record *rs, int n, int depth);
struct record* find_median(struct record *rs, int n, int axis);
void print_kdtree(struct kdtree_data *data);
void print_kdtree_node(const struct node *node, int depth);
int compare_records_lon(const void *a, const void *b);
int compare_records_lat(const void *a, const void *b);
void free_kdtree(struct kdtree_data* data);
void free_kdtree_node(struct node *node);
const struct record* lookup_kdtree(struct kdtree_data *data, double lon, double lat);
double calculate_euclidean_distance(const struct record *node_r, const struct query query);
void search_kdtree(struct node **closest_node, double *clostest_dist, const struct query query, struct node *node);
double calculate_difference(const struct node *node, const struct query query);

int main(int argc, char** argv) {
  return coord_query_loop(argc, argv,
                          (mk_index_fn)mk_kdtree,
                          (free_index_fn)free_kdtree,
                          (lookup_fn)lookup_kdtree);
}

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

  print_kdtree(data);

  return data;
}

void print_kdtree(struct kdtree_data *data) {
    if (data == NULL || data->root == NULL) {
        printf("--- K-D Tree is empty ---\n");
        return;
    }
    printf("\n--- K-D Tree Structure (Right branch printed higher) ---\n");
    print_kdtree_node(data->root, 0);
    printf("------------------------------------------------------\n");
}

void print_kdtree_node(const struct node *node, int depth) {
    if (node == NULL) {
        return;
    }

    // 1. Traverse right branch (Printed higher up for visualization)
    print_kdtree_node(node->right, depth + 1);

    // 2. Print current node with indentation
    
    // Create indentation
    for (int i = 0; i < depth; i++) {
        printf("    "); 
    }

    // Determine the split axis label (0=LON, 1=LAT)
    const char *axis_label = (node->axis == 0) ? "LON" : "LAT";

    // Print node information
    printf("|- [D:%d | Split:%s] (%.6f, %.6f) ID: %ld\n",
           depth,
           axis_label,
           node->r->lon,
           node->r->lat,
           node->r->osm_id);

    // 3. Traverse left branch
    print_kdtree_node(node->left, depth + 1);
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

  node->left = build_kdtree(rs, median - rs, depth + 1);
  node->right = build_kdtree(median + 1, rs + n - (median + 1), depth + 1);

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
    lon = lon, 
    lat = lat,
  };

  struct node *closest_node = data->root;
  double closest_dist = calculate_euclidean_distance(closest_node->r, query);

  search_kdtree(&closest_node, &closest_dist, query, data->root);

  return closest_node->r;
}

void search_kdtree(struct node **closest_node, double *clostest_dist, const struct query query, struct node *node) {
  if (node == NULL) {
    return;
  }

  double dist = calculate_euclidean_distance(node->r, query);

  if (dist < *clostest_dist) {
    *closest_node = node;
    *clostest_dist = dist;
  }
 
  double diff = calculate_difference(node, query);
  double radius = *clostest_dist;

  if (diff >= 0 || radius > fabs(diff)) {
    search_kdtree(closest_node, clostest_dist, query, node->left);
  } 
  
  if (diff <= 0 || radius > fabs(diff)) {
    search_kdtree(closest_node, clostest_dist, query, node->right);
  }
}

double calculate_euclidean_distance(const struct record *node_r, const struct query query) {
  double dx = node_r->lon - query.lon;
  double dy = node_r->lat - query.lat;

  double dist = sqrt(dx * dx + dy * dy);

  return dist;
}

double calculate_difference(const struct node *node, const struct query query) {
  if (node->axis == 0) { 
    return node->r->lon - query.lon;
  } else {
    return node->r->lat - query.lat;
  }
}