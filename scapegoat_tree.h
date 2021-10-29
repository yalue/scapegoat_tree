#ifndef SCAPEGOAT_TREE_H
#define SCAPEGOAT_TREE_H
#ifdef __cplusplus
extern "C" {
#endif

// A single node in the tree.
typedef struct ScapegoatTreeNode_s {
  struct ScapegoatTreeNode_s *parent, *left, *right;
  void *key;
} ScapegoatTreeNode;

// The type of the function used to compare keys a and b when inserting or
// searching for a key.
typedef int (*ScapegoatComparatorFunction)(const void *a, const void *b);

// The type of callback function required when traversing the tree. Receives
// the key of each visited node, as well as the user data pointer provided to
// the traversal function.
typedef void (*TraversalCallbackFunction)(void *key, void *user_data);

// Holds metadata about the entire tree, including its root node.
typedef struct {
  ScapegoatComparatorFunction comparator;
  // Will be NULL until the first insert.
  ScapegoatTreeNode *root;
  // The total number of nodes in the tree.
  int tree_size;
  // Holds a sorted list of node pointers, to avoid a large number of
  // reallocations. Starts as NULL. Used internally, do not modify.
  ScapegoatTreeNode **node_ptr_cache;
  // The number of pointers that can fit in node_ptr_cache. Do not modify this
  // either.
  int node_ptr_cache_size;
} ScapegoatTree;

// Creates an empty scapegoat tree. Requires the comparator function to be used
// when inserting keys. Returns NULL on error.
ScapegoatTree* CreateScapegoatTree(ScapegoatComparatorFunction comparator);

// Inserts the given key into the tree. Returns 0 if an error occurs. The key
// must not be NULL.
int ScapegoatInsert(ScapegoatTree *tree, void *key);

// Searches the tree for a key matching the provided argument. This will always
// return the first matching key that was inserted into the tree, as determined
// by the comparator function. Returns NULL if the key isn't found.
void* ScapegoatSearch(ScapegoatTree *tree, void *key);

// Visits every node in the tree in order. Calls the callback function with
// each node's key as it is visited, providing the user_data pointer to the
// callback.
void TraverseScapegoatTree(ScapegoatTree *tree,
    TraversalCallbackFunction callback, void *user_data);

// Frees any memory associated with the scapegoat tree. The tree pointer is
// invalid after calling this.
void DestroyScapegoatTree(ScapegoatTree *tree);

#ifdef __cplusplus
}  // extern "C"
#endif
#endif  // SCAPEGOAT_TREE_H
