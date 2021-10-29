#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../scapegoat_tree.h"

static double CurrentSeconds(void) {
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC_RAW, &ts) != 0) {
    printf("Error getting time.\n");
    exit(1);
  }
  return ((double) ts.tv_sec) + (((double) ts.tv_nsec) / 1e9);
}

static void PrintTree(ScapegoatTreeNode *node, int depth) {
  int i, value;
  if (node == NULL) return;
  value = *((const int *) node->key);
  for (i = 0; i < depth; i++) {
    printf("|   ");
  }
  printf("%d\n", value);
  PrintTree(node->left, depth + 1);
  PrintTree(node->right, depth + 1);
}

// Remember that the keys aren't ints, but pointers to ints.
static int IntComparator(const void *a, const void *b) {
  int i_a = *((const int *) a);
  int i_b = *((const int *) b);
  if (i_a < i_b) return -1;
  if (i_a > i_b) return 1;
  return 0;
}

// Returns 1 if all elements are found in the tree.
static int VerifyTreeContents(ScapegoatTree *tree, int *keys, int key_count) {
  int i;
  int *found_key = NULL;
  double start_time;
  start_time = CurrentSeconds();
  for (i = 0; i < key_count; i++) {
    found_key = ScapegoatSearch(tree, keys + i);
    if (!found_key) {
      printf("Key %d not found in tree.\n", keys[i]);
      return 0;
    }
    if (*((int *) found_key) != keys[i]) {
      printf("Key %d doesn't match the key returned by search (%d)\n",
        keys[i], *((int *) found_key));
      return 0;
    }
  }
  printf("Looking up %d keys took %.03f seconds.\n", key_count,
    CurrentSeconds() - start_time);
  printf("All keys found in tree!\n");
  return 1;
}

// Builds a tree, inserting the specified number of random elements.
static int TestTree(int tree_size) {
  int i = 0, result = 1;
  int *keys = NULL;
  ScapegoatTree *tree = NULL;

  tree = CreateScapegoatTree(IntComparator);
  if (!tree) {
    printf("Failed creating empty tree.\n");
    return 0;
  }

  keys = calloc(sizeof(int), tree_size);
  if (!keys) {
    printf("Failed allocating keys to insert into the tree.\n");
    DestroyScapegoatTree(tree);
    return 0;
  }
  // First, initialize the list of keys. (We keep this in a separate list so
  // we can make sure they all got inserted lated.) We'll put them in order,
  // since that would normally lead to a very unbalanced tree.
  for (i = 0; i < tree_size; i++) keys[i] = i;

  // Build the tree.
  for (i = 0; i < tree_size; i++) {
    printf("Inserting key %d/%d\n", i + 1, tree_size);
    if (!ScapegoatInsert(tree, keys + i)) {
      printf("Tree insert %d/%d failed.\n", i + 1, tree_size);
      result = 0;
      goto cleanup;
    }
  }

  if (!VerifyTreeContents(tree, keys, tree_size)) {
    result = 0;
    goto cleanup;
  }

  if (tree_size < 40) {
    PrintTree(tree->root, 0);
  } else {
    printf("Not printing the tree; it's too big.\n");
  }

cleanup:
  DestroyScapegoatTree(tree);
  free(keys);
  return result;
}

static void PrintUsage(const char *name) {
  printf("Usage: %s <# of elements to insert>\n", name);
  exit(1);
}

int main(int argc, char **argv) {
  int tree_size = 0;
  if (argc != 2) PrintUsage(argv[0]);
  tree_size = atoi(argv[1]);
  if (tree_size <= 0) PrintUsage(argv[0]);
  if (!TestTree(tree_size)) {
    printf("Encountered an error in the tree.\n");
    return 1;
  }
  printf("Tree constructed OK.\n");
  return 0;
}
