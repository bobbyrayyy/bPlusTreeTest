// B+ tree in C++

#include <iostream>

using namespace std;
int MAX_KEYS = 5;

// BP node
class Node {
  private: 
    bool IS_LEAF;
    int *key, num_keys;
    Node **ptr;
    friend class BPTree;

  public:
    Node();
};

// BP tree
class BPTree {
  private:
    Node *root;
    void insertInternal(int, Node *, Node *);
    Node *findParent(Node *, Node *);

  public:
    BPTree();
    void search(int);
    void insert(int);
    void display(Node *, int level);
    Node *getRoot();
};

Node::Node() {
  key = new int[MAX_KEYS];
  ptr = new Node *[MAX_KEYS + 1];
}

BPTree::BPTree() {
  root = NULL;
}

// Search operation
void BPTree::search(int x) {
  if (root == NULL) {
    cout << "Tree is empty\n";
  } else {
    Node *cursor = root;
    while (cursor->IS_LEAF == false) {
      for (int i = 0; i < cursor->num_keys; i++) {
        if (x < cursor->key[i]) {
          cursor = cursor->ptr[i];
          break;
        }
        if (i == cursor->num_keys - 1) {
          cursor = cursor->ptr[i + 1];
          break;
        }
      }
    }
    for (int i = 0; i < cursor->num_keys; i++) {
      if (cursor->key[i] == x) {
        cout << "Found\n";
        return;
      }
    }
    cout << "Not found\n";
  }
}

// Insert Operation
void BPTree::insert(int x) {
  if (root == NULL) {
    root = new Node;
    root->key[0] = x;
    root->IS_LEAF = true;
    root->num_keys = 1;
  } else {
    Node *cursor = root;
    Node *parent;
    while (cursor->IS_LEAF == false) {
      parent = cursor;
      for (int i = 0; i < cursor->num_keys; i++) {
        if (x < cursor->key[i]) {
          cursor = cursor->ptr[i];
          break;
        }
        if (i == cursor->num_keys - 1) {
          cursor = cursor->ptr[i + 1];
          break;
        }
      }
    }
    if (cursor->num_keys < MAX_KEYS) {
      int i = 0;
      while (x > cursor->key[i] && i < cursor->num_keys)
        i++;
      for (int j = cursor->num_keys; j > i; j--) {
        cursor->key[j] = cursor->key[j - 1];
      }
      cursor->key[i] = x;
      cursor->num_keys++;
      cursor->ptr[cursor->num_keys] = cursor->ptr[cursor->num_keys - 1];
      cursor->ptr[cursor->num_keys - 1] = NULL;
    } else {
      Node *newLeaf = new Node;
      int virtualNode[MAX_KEYS + 1];
      for (int i = 0; i < MAX_KEYS; i++) {
        virtualNode[i] = cursor->key[i];
      }
      int i = 0, j;
      while (x > virtualNode[i] && i < MAX_KEYS)
        i++;
      for (int j = MAX_KEYS + 1; j > i; j--) {
        virtualNode[j] = virtualNode[j - 1];
      }
      virtualNode[i] = x;
      newLeaf->IS_LEAF = true;
      cursor->num_keys = (MAX_KEYS + 1) / 2;
      newLeaf->num_keys = MAX_KEYS + 1 - (MAX_KEYS + 1) / 2;
      cursor->ptr[cursor->num_keys] = newLeaf;
      newLeaf->ptr[newLeaf->num_keys] = cursor->ptr[MAX_KEYS];
      cursor->ptr[MAX_KEYS] = NULL;
      for (i = 0; i < cursor->num_keys; i++) {
        cursor->key[i] = virtualNode[i];
      }
      for (i = 0, j = cursor->num_keys; i < newLeaf->num_keys; i++, j++) {
        newLeaf->key[i] = virtualNode[j];
      }
      if (cursor == root) {
        Node *newRoot = new Node;
        newRoot->key[0] = newLeaf->key[0];
        newRoot->ptr[0] = cursor;
        newRoot->ptr[1] = newLeaf;
        newRoot->IS_LEAF = false;
        newRoot->num_keys = 1;
        root = newRoot;
      } else {
        insertInternal(newLeaf->key[0], parent, newLeaf);
      }
    }
  }
}

// Insert Operation
void BPTree::insertInternal(int x, Node *cursor, Node *child) {
  if (cursor->num_keys < MAX_KEYS) {
    int i = 0;
    while (x > cursor->key[i] && i < cursor->num_keys)
      i++;
    for (int j = cursor->num_keys; j > i; j--) {
      cursor->key[j] = cursor->key[j - 1];
    }
    for (int j = cursor->num_keys + 1; j > i + 1; j--) {
      cursor->ptr[j] = cursor->ptr[j - 1];
    }
    cursor->key[i] = x;
    cursor->num_keys++;
    cursor->ptr[i + 1] = child;
  } else {
    Node *newInternal = new Node;
    int virtualKey[MAX_KEYS + 1];
    Node *virtualPtr[MAX_KEYS + 2];
    for (int i = 0; i < MAX_KEYS; i++) {
      virtualKey[i] = cursor->key[i];
    }
    for (int i = 0; i < MAX_KEYS + 1; i++) {
      virtualPtr[i] = cursor->ptr[i];
    }
    int i = 0, j;
    while (x > virtualKey[i] && i < MAX_KEYS)
      i++;
    for (int j = MAX_KEYS + 1; j > i; j--) {
      virtualKey[j] = virtualKey[j - 1];
    }
    virtualKey[i] = x;
    for (int j = MAX_KEYS + 2; j > i + 1; j--) {
      virtualPtr[j] = virtualPtr[j - 1];
    }
    virtualPtr[i + 1] = child;
    newInternal->IS_LEAF = false;
    cursor->num_keys = (MAX_KEYS + 1) / 2;
    newInternal->num_keys = MAX_KEYS - (MAX_KEYS + 1) / 2;
    for (i = 0, j = cursor->num_keys + 1; i < newInternal->num_keys; i++, j++) {
      newInternal->key[i] = virtualKey[j];
    }
    for (i = 0, j = cursor->num_keys + 1; i < newInternal->num_keys + 1; i++, j++) {
      newInternal->ptr[i] = virtualPtr[j];
    }
    if (cursor == root) {
      Node *newRoot = new Node;
      newRoot->key[0] = cursor->key[cursor->num_keys];
      newRoot->ptr[0] = cursor;
      newRoot->ptr[1] = newInternal;
      newRoot->IS_LEAF = false;
      newRoot->num_keys = 1;
      root = newRoot;
    } else {
      insertInternal(cursor->key[cursor->num_keys], findParent(root, cursor), newInternal);
    }
  }
}

// Find the parent
Node *BPTree::findParent(Node *cursor, Node *child) {
  Node *parent;
  if (cursor->IS_LEAF || (cursor->ptr[0])->IS_LEAF) {
    return NULL;
  }
  for (int i = 0; i < cursor->num_keys + 1; i++) {
    if (cursor->ptr[i] == child) {
      parent = cursor;
      return parent;
    } else {
      parent = findParent(cursor->ptr[i], child);
      if (parent != NULL)
        return parent;
    }
  }
  return parent;
}

// Print the tree
void BPTree::display(Node *cursor, int level) {
  if (cursor != NULL) {
    for (int i = 0; i < level; i++) {
      cout << "   ";
    }
    cout << "level " << level << ": ";

    for (int i = 0; i < cursor->num_keys; i++) {
        cout << cursor->key[i] << " ";
    }

    for (int i = cursor->num_keys; i < MAX_KEYS; i++) {
      cout << "x ";
    }

      cout << "\n";
    if (cursor->IS_LEAF != true) {
      for (int i = 0; i < cursor->num_keys + 1; i++) {
        display(cursor->ptr[i], level+1);
      }
    } 
  }
}

// Get the root
Node *BPTree::getRoot() {
  return root;
}

int bpt_2() {
  BPTree node;
  for (int i = 1; i < 50; i++) {
    node.insert(i);
  }
    
  node.display(node.getRoot(), 1);

  node.search(15);
}