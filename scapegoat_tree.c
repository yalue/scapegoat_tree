#include <stdlib.h>
#include "scapegoat_tree.h"

// Determines how unbalanced a node's subtrees are allowed to be, before
// requiring a rebuild of its subtrees. Must be between 0.5 and 1.0, but seems
// a bit better if it's slightly over 2/3rds.
#define BALANCE_FACTOR (0.7)

ScapegoatTree* CreateScapegoatTree(ScapegoatComparatorFunction comparator) {
  ScapegoatTree *to_return = NULL;
  to_return = (ScapegoatTree *) calloc(sizeof(ScapegoatTree), 1);
  if (!to_return) return NULL;
  to_return->comparator = comparator;
  return to_return;
}

// Gets a list capable of holding a number of node pointers specified by
// required_capacity. Returns NULL on error. Used to avoid unnecessary
// reallocations.
static ScapegoatTreeNode** GetOrAllocateNodePtrCache(ScapegoatTree *tree,
    int required_capacity) {
  ScapegoatTreeNode **new_list = NULL;
  if (required_capacity <= tree->node_ptr_cache_size) {
    // We're able to use the previous allocation.
    return tree->node_ptr_cache;
  }
  // We need to expand the cache. Rather than expanding it one-by-one, we
  // always grow it to a size that can hold all the nodes in the tree, since
  // we assume that will always work.
  if (required_capacity > tree->tree_size) {
    // This would be a fatal internal error. Checking just for sanity.
    return NULL;
  }
  // I'm not sure if realloc is any better. We don't care about the old data,
  // so calloc should be fine, but would it occasionally be faster to try
  // expanding the old location instead? For now, I'll just do calloc, but free
  // the old data first so maybe calloc can use the space.
  free(tree->node_ptr_cache);
  tree->node_ptr_cache = NULL;
  tree->node_ptr_cache_size = 0;
  new_list = (ScapegoatTreeNode **) calloc(sizeof(ScapegoatTreeNode *),
    tree->tree_size);
  if (!new_list) return NULL;
  tree->node_ptr_cache = new_list;
  tree->node_ptr_cache_size = tree->tree_size;
  return new_list;
}

// Finds the node containing the given key if it's already in the tree. If the
// node is not already in the tree, this will instead return the leaf node that
// would be the new node's parent if it were to be inserted. Only returns NULL
// if the tree is empty.
static ScapegoatTreeNode* FindClosestNode(ScapegoatTree *tree,
    ScapegoatTreeNode *node, void *key) {
  int result;
  if (!node) return NULL;
  result = tree->comparator(key, node->key);
  if (result == 0) return node;
  if (result < 0) {
    if (!node->left) return node;
    return FindClosestNode(tree, node->left, key);
  }
  if (!node->right) return node;
  return FindClosestNode(tree, node->right, key);
}

// Allocates a new node. Sets its key, and sets parent and child pointers to
// NULL. Returns NULL on error.
static ScapegoatTreeNode* AllocateNewNode(void *key) {
  ScapegoatTreeNode *to_return = NULL;
  to_return = (ScapegoatTreeNode *) calloc(sizeof(ScapegoatTreeNode), 1);
  if (!to_return) return NULL;
  to_return->key = key;
  return to_return;
}

// Returns the size of the subtree rooted at the given node. Returns 0 if the
// node is NULL.
static int GetTreeSize(ScapegoatTreeNode *node) {
  if (!node) return 0;
  return 1 + GetTreeSize(node->left) + GetTreeSize(node->right);
}

// Traverses all nodes in the subtree rooted at the given node, and writes
// pointers to them, in order, into the sorted array. next_index is used to
// track where the next pointer should be written.
static void TraverseNodesInOrder(ScapegoatTreeNode *node,
    ScapegoatTreeNode **sorted, int *next_index) {
  if (node == NULL) return;
  TraverseNodesInOrder(node->left, sorted, next_index);
  sorted[*next_index] = node;
  *next_index += 1;
  TraverseNodesInOrder(node->right, sorted, next_index);
}

// Takes a list of pointers to pre-allocated nodes, sorted in the order of the
// nodes' keys. Modifies all nodes' connections to form a balanced binary tree,
// and returns the root of the tree.  This shouldn't fail, since it won't
// allocate anything. Must never be called with node_count equal to 0.
static ScapegoatTreeNode* BuildTreeFromSortedNodes(
    ScapegoatTreeNode **sorted_nodes, int node_count) {
  int median_index;
  ScapegoatTreeNode *median = NULL, *left = NULL, *right = NULL;

  median_index = node_count >> 1;
  median = sorted_nodes[median_index];
  // Break any old connections (the caller will set the parent)
  median->left = NULL;
  median->right = NULL;
  // Return now if we just consumed the last node in the list
  if (node_count == 1) return median;

  // Construct the left subtree with all nodes below the median
  left = BuildTreeFromSortedNodes(sorted_nodes, median_index);
  left->parent = median;
  median->left = left;

  // If we only had two nodes left, we won't have a right subtree.
  if (node_count == 2) return median;

  // Build the right subtree, including all nodes after the median.
  right = BuildTreeFromSortedNodes(sorted_nodes + median_index + 1,
    node_count - (median_index + 1));
  right->parent = median;
  median->right = right;
  return median;
}

// Rebuilds the subtree rooted at the given node. Returns 0 on error, such as
// running out of memory. Requires the size of the subtree rooted at the given
// node.
static int RebuildSubtree(ScapegoatTree *tree, ScapegoatTreeNode *node,
    int node_size) {
  ScapegoatTreeNode *new_root = NULL;
  ScapegoatTreeNode *original_parent = NULL;
  ScapegoatTreeNode **sorted_nodes = NULL;
  int was_left_child = 0, sorted_node_count = 0;

  // Before changing anything, record our original parent and whether we were
  // to the right or the left.
  if (node->parent != NULL) {
    original_parent = node->parent;
    was_left_child = original_parent->left == node;
  }

  // Obtain a sorted array of all node pointers, so we can rebuild everything
  // in a balanced manner without needing to reallocate all the nodes.
  sorted_nodes = GetOrAllocateNodePtrCache(tree, node_size);
  if (!sorted_nodes) return 0;
  TraverseNodesInOrder(node, sorted_nodes, &sorted_node_count);

  // Adjust all of the internal node pointers so that the tree is perfectly
  // balanced.
  new_root = BuildTreeFromSortedNodes(sorted_nodes, sorted_node_count);

  // We still need to connect the new root node to the rest of the tree.
  new_root->parent = original_parent;
  if (original_parent) {
    if (was_left_child) {
      original_parent->left = new_root;
    } else {
      original_parent->right = new_root;
    }
  } else {
    // We were originally rebalancing the root of the entire tree.
    tree->root = new_root;
  }
  return 1;
}

// Returns 1 if a node with the given left and right sizes is too unablanced.
static int IsUnbalanced(int left_size, int right_size) {
  double balance;
  double our_size = left_size + right_size + 1;

  // Is the left side too big?
  balance = ((double) left_size) / our_size;
  if (balance > BALANCE_FACTOR) return 1;

  // Is the right side too big?
  balance = ((double) right_size) / our_size;
  if (balance > BALANCE_FACTOR) return 1;

  return 0;
}

// Starts at a leaf node and proceeds towards the root. Rebalances the first
// node that's sufficiently unbalanced. Returns 0 on error.
static int FindAndRebalanceRecursive(ScapegoatTree *tree,
    ScapegoatTreeNode *node, int left_size, int right_size) {
  ScapegoatTreeNode *parent = NULL;
  int our_size = left_size + right_size + 1;

  // If we're at the root of an unbalanced subtree, then rebalance it and
  // return.
  if (IsUnbalanced(left_size, right_size)) {
    return RebuildSubtree(tree, node, our_size);
  }

  // We weren't at an unbalanced subtree, so continue the search upwards.
  parent = node->parent;
  // Return now if we're already at the root and haven't found something
  // needing to be rebalanced.
  if (!parent) return 1;

  // Get the sizes to pass to the recursive call. We only need to get our
  // sibling's size, since we already know our own size.
  if (parent->left == node) {
    right_size = GetTreeSize(parent->right);
    left_size = our_size;
  } else {
    left_size = GetTreeSize(parent->left);
    right_size = our_size;
  }
  return FindAndRebalanceRecursive(tree, parent, left_size, right_size);
}

int ScapegoatInsert(ScapegoatTree *tree, void *key) {
  ScapegoatTreeNode *parent = NULL;
  ScapegoatTreeNode *new_node = NULL;
  int result;

  // If the tree is empty, then simply create the root.
  if (!tree->root) {
    new_node = AllocateNewNode(key);
    if (!new_node) return 0;
    tree->root = new_node;
    tree->tree_size = 1;
    return 1;
  }

  // Find either the matching node, or its parent.
  parent = FindClosestNode(tree, tree->root, key);
  result = tree->comparator(key, parent->key);
  // Quit now if we found a node with an identical key.
  if (result == 0) return 1;

  // The node wasn't in the tree yet, so insert it.
  tree->tree_size += 1;
  new_node = AllocateNewNode(key);
  if (!new_node) return 0;
  new_node->parent = parent;
  if (result < 0) {
    parent->left = new_node;
  } else {
    parent->right = new_node;
  }

  // Traverse upwards from the new node, rebalancing the first unbalanced node
  // we find.
  return FindAndRebalanceRecursive(tree, new_node, 0, 0);
}

static void* SearchTreeRecursive(ScapegoatTree *tree, ScapegoatTreeNode *root,
    void *key) {
  int result;
  if (!root) return NULL;
  result = tree->comparator(key, root->key);
  if (result == 0) return root->key;
  if (result < 0) return SearchTreeRecursive(tree, root->left, key);
  return SearchTreeRecursive(tree, root->right, key);
}

void* ScapegoatSearch(ScapegoatTree *tree, void *key) {
  return SearchTreeRecursive(tree, tree->root, key);
}

static void DestroyTreeRecursive(ScapegoatTreeNode *root) {
  ScapegoatTreeNode *left, *right;
  if (root == NULL) return;
  left = root->left;
  right = root->right;
  free(root);
  root = NULL;
  DestroyTreeRecursive(left);
  DestroyTreeRecursive(right);
}

void DestroyScapegoatTree(ScapegoatTree *tree) {
  DestroyTreeRecursive(tree->root);
  tree->root = NULL;
  tree->tree_size = 0;
  free(tree->node_ptr_cache);
  tree->node_ptr_cache = NULL;
  tree->node_ptr_cache_size = 0;
  free(tree);
}
