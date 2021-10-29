A Basic Scapegoat Tree implementation in C
==========================================

This implements a "scapegoat tree:" a balanced binary tree with amortized
`O(log n)` insert, with `O(log n)` search.  This was done for fun, so it has
some shortcomings.

This library does *not* support deleting individual nodes from the tree.

Currently it's still kind of slow, especially after ~200k inserts. Not sure if
this is my fault, or simply the reason why I hadn't heard of scapegoat trees
until recently. 200k lookups is very quick, at least, so at least the tree is
being constructed properly.

Usage
-----

 - Include the `scapegoat_tree.h` header file in your C source.
 - The implementation is in `scapegoat_tree.c`, compile or link it into your
   program however you want.

Internally, the library uses arbitrary `void *` keys. The user must provide a
function to compare two keys, similar to what's required for the `qsort`
function in the C standard library:
```
static int MyComparator(const void *a, const void *b) {
  // <Provide your implementation here.>
  // Return -1 if a < b
  // Return 1 if a > b
  // Return 0 if a == b
}
```

First, create a tree using `CreateScapegoatTree`:

```
ScapegoatTree *tree = CreateScapegoatTree(MyComparator);
if (!tree) { /* An error occurred */ }
```

Inserting a key into the tree uses the `ScapegoatInsert` function:

```
int result = ScapegoatInsert(tree, (void *) key);
if (!result) { /* An error occurred */ }
```

Searching for a key uses the `ScapegoatSearch` function:

```
void *found = ScapegoatSearch(tree, (void *) key);
if (!found) { /* The key wasn't in the tree */ }
```

Note that the pointer returned by `ScapegoatSearch` will be the first key
inserted into the tree for which `MyComparator(key, found) == 0`, so
the `found` pointer may or may not equal the `key` pointer in the above
example.  Do *not* change any portion of the `found` key that would cause it to
be in the wrong location in the tree!

To destroy the entire tree, freeing its memory, call `ScapegoatDestroy`:

```
ScapegoatDestroy(tree);
tree = NULL;
```

