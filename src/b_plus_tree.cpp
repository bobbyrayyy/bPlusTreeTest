#include "b_plus_tree.h"
#include "memory_pool.h"
#include "types.h"

#include <tuple>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <cstring>

using namespace std;

Node::Node(int maxKeys)
{
  // Initialize empty array of keys and pointers.
  this->keys = new float[maxKeys];
  this->pointers = new Address[maxKeys];
}

BPlusTree::BPlusTree(std::size_t blockSize)
{
  // Get size left for keys and pointers in a node after accounting for node's isLeaf and numKeys attributes.
  size_t nodeBufferSize = blockSize - sizeof(bool) - sizeof(int);

  // Set max keys available in a node. Each key is a float, each pointer is a struct of {void *blockAddress, short int offset}.
  // Therefore, each key is 4 bytes. Each pointer is around 16 bytes.

  // Initialize node buffer with a pointer. P | K | P , always one more pointer than keys.
  size_t sum = sizeof(Address);
  maxKeys = 0;

  // Try to fit as many pointer key pairs as possible into the node block.
  while (sum + sizeof(Address) + sizeof(float) <= nodeBufferSize)
  {
    sum += (sizeof(Address) + sizeof(float));
    maxKeys += 1;
  }

  if (maxKeys == 0)
  {
    throw std::overflow_error("Error: Keys and pointers too large to fit into a node!");
  }

  // Initialize root to NULL
  root = nullptr;

  // Set node size to be equal to block size.
  nodeSize = blockSize;

  // Initialize initial variables
  levels = 0;
  numNodes = 0;

  // Initialize disk space for index.
  MemoryPool pool(150000000, 100);
  index = &pool;
}

vector<Record> BPlusTree::select(float lowerBoundKey, float upperBoundKey)
{
  // Tree is empty.
  if (root == nullptr)
  {
    throw std::logic_error("Tree is empty!");
  }
  // Else iterate through root node and follow the keys to find the correct key.
  else
  {
    // Set cursor to root (root must be in main memory to access B+ Tree).
    Node *cursor = root;
    int indexNodesAccessed = 1; // Count number of index nodes accessed. Assume root already accessed.
    int dataBlocksAccessed = 0; // Count number of data blocks accessed.

    // While we haven't hit a leaf node, search for the corresponding lowerBoundKey within the cursor node's keys.
    while (cursor->isLeaf == false)
    {
      // Iterate through each key in the current node. We need to load nodes from the disk whenever we want to traverse to another node.
      for (int i = 0; i < cursor->numKeys; i++)
      {
        // If lowerBoundKey is lesser than current key, go to the left pointer's node to continue searching.
        if (lowerBoundKey < cursor->keys[i])
        {
          // Load node from disk to main memory.
          void *mainMemoryNode = operator new(nodeSize);

          // We just need to load the whole block from disk since block size = node size.
          void *blockAddress = cursor->pointers[i].blockAddress;
          std::memcpy(mainMemoryNode, blockAddress, nodeSize);

          // Set cursor to the child node, now loaded in main memory.
          cursor = (Node *)mainMemoryNode;
          indexNodesAccessed += 1;
          break;
        }
        // If we reached the end of all keys in this node (larger than all), then go to the right pointer's node to continue searching.
        if (i == cursor->numKeys - 1)
        {
          // Load node from disk to main memory.
          void *mainMemoryNode = operator new(nodeSize);

          // In this case, we load pointer + 1 (next pointer in node since the lowerBoundKey is larger than all keys in this node).
          void *blockAddress = cursor->pointers[i + 1].blockAddress;
          std::memcpy(mainMemoryNode, blockAddress, nodeSize);

          // Set cursor to the child node, now loaded in main memory.
          cursor = (Node *)mainMemoryNode;
          indexNodesAccessed += 1;
          break;
        }
      }
    }

    // When we reach here, we have hit a leaf node corresponding to the lowerBoundKey.
    // Again, search each of the leaf node's keys to find a match.
    vector<Record> results;
    unordered_map<void *, void *> loadedBlocks; // Maintain a reference to all loaded blocks in main memory.

    // Keep searching whole range until we find a key that is out of range.
    bool stop = false;
    while (!stop)
    {
      for (int i = 0; i < cursor->numKeys; i++)
      {
        // Found a key within range, now we need to iterate through the entire range until the upperBoundKey.
        if (cursor->keys[i] >= lowerBoundKey && cursor->keys[i] <= upperBoundKey)
        {
          // Check if this block hasn't been loaded from disk yet.
          void *blockAddress = (void *)(cursor->pointers[i]).blockAddress;
          short int offset = cursor->pointers[i].offset;

          if (loadedBlocks.find(blockAddress) == loadedBlocks.end())
          {
            // Load block into main memory.
            void *mainMemoryBlock = operator new(nodeSize);
            std::memcpy(mainMemoryBlock, blockAddress, nodeSize);

            // Keep track of loaded blocks so we don't have to reload them from disk.
            loadedBlocks[blockAddress] = mainMemoryBlock;

            dataBlocksAccessed += 1;
          }

          // Here, we can access the loaded block (in main memory) to get the record that fits our search range.
          // Add the corresponding record to the results list using its main memory block address + offset.
          Record result = *(Record *)(loadedBlocks[blockAddress] + offset);
          results.push_back(result);
        }
        // If we find a key that is out of range, stop searching.
        else
        {
          stop = true;
          break;
        }

        // If we reached the end of all keys in this node, go to the next linked leaf node to keep searching.
        if (i == cursor->numKeys - 1)
        {
          // Load next node from disk to main memory.
          void *mainMemoryNode = operator new(nodeSize);

          // Load pointer + 1 (next pointer in node).
          void *blockAddress = cursor->pointers[i + 1].blockAddress;
          std::memcpy(mainMemoryNode, blockAddress, nodeSize);

          // Set cursor to the child node, now loaded in main memory.
          cursor = (Node *)mainMemoryNode;
          indexNodesAccessed += 1;
          break;
        }
      }
    }

    // Report on blocks searched.
    cout << "Number of index nodes accessed: " << indexNodesAccessed << '\n';
    cout << "Number of data blocks accessed: " << dataBlocksAccessed << '\n';

    // If nothing found, throw an error.
    if (results.size() < 1)
    {
      throw std::logic_error("Could not find any matching records within the given range.");
    }
    // Else return the list of records found corresponding to the search range.
    else
    {
      return results;
    }
  }
}

// Display a node and its contents in the B+ Tree.
void BPlusTree::displayNode(Node *node)
{
  // Print out all contents in the node as such |pointer|key|pointer|
  int i = 0;
  cout << "|";
  while (i < node->numKeys)
  {
    cout << node->pointers[i].blockAddress << "|";
    cout << node->keys[i] << "|";
    i++;
  }

  // Print last filled pointer
  cout << node->pointers[i + 1].blockAddress << "|";

  while (i < maxKeys)
  {
    cout << " x |";      // Remaining empty keys
    cout << "  Null  |"; // Remaining empty pointers
    i++;
  }

  cout << '\n';
}

// Display a block and its contents in the disk. Assume it's already loaded in main memory.
void BPlusTree::displayBlock(void *block)
{
  cout << "--------------- Start block -----------------" << '\n';
  if (*(unsigned char *)&block == '\0')
  {
    cout << "Empty block!" << '\n';
  }
  else
  {
    void *endOfBlock = &block + nodeSize;
    while (*(unsigned char *)&block != '\0' && block < endOfBlock)
    {
      Record *record = (Record *)block;

      cout << "|" << record->tconst << "|" << record->averageRating << "|" << record->numVotes << "|" << '\n';
      block = &block + sizeof(Record);
    }
  }
  cout << "---------------- End block ------------------" << '\n';
}

// Insert a record into the B+ Tree index. Key: Record's avgRating, Value: {blockAddress, offset}.
void BPlusTree::insert(Address address, float key)
{
  // If no root exists, create a new B+ Tree root.
  if (root == nullptr)
  {
    // Create new node in main memory, set it to root, and add the key and values to it.
    root = new Node(maxKeys);
    root->keys[0] = key;

    root->isLeaf = true; // It is both the root and a leaf.
    root->numKeys = 1;
    root->pointers[0] = address; // Add record's disk address to pointer.

    // Write the node into disk.
    void *diskNode = index->allocate(nodeSize).blockAddress;
    std::memmove(&diskNode, root, nodeSize);

    // Keep track of root node's disk address.
    rootAddress = diskNode;

    // Update number of nodes and levels
    numNodes++;
    levels++;
  }
  // Else if root exists already, traverse the nodes to find the proper place to insert the key.
  else
  {
    Node *cursor = root;
    Node *parent;                          // Keep track of the parent as we go deeper into the tree in case we need to update it.
    void *parentDiskAddress = rootAddress; // Keep track of parent's disk address as well so we can update parent in disk.
    void *cursorDiskAddress = rootAddress; // Store current node's disk address in case we need to update it in disk.

    // While not leaf, keep following the nodes to correct key.
    while (cursor->isLeaf == false)
    {
      // Set the parent of the node (in case we need to assign new child later), and its disk address.
      parent = cursor;
      parentDiskAddress = cursorDiskAddress;

      // Check through all keys of the node to find key and pointer to follow downwards.
      for (int i = 0; i < cursor->numKeys; i++)
      {
        // If key is lesser than current key, go to the left pointer's node.
        if (key < cursor->keys[i])
        {
          // Load node in from disk to main memory.
          void *mainMemoryNode = operator new(nodeSize);
          std::memcpy(mainMemoryNode, cursor->pointers[i].blockAddress, nodeSize);

          // Update cursorDiskAddress to maintain address in disk if we need to update nodes.
          cursorDiskAddress = cursor->pointers[i].blockAddress;

          // Move to new node in main memory.
          cursor = (Node *)mainMemoryNode;

          break;
        }
        // Else if key larger than all keys in the node, go to last pointer's node (rightmost).
        if (i == cursor->numKeys - 1)
        {
          // Load node in from disk to main memory.
          void *mainMemoryNode = operator new(nodeSize);
          std::memcpy(mainMemoryNode, cursor->pointers[i + i].blockAddress, nodeSize);

          // Update diskAddress to maintain address in disk if we need to update nodes.
          cursorDiskAddress = cursor->pointers[i + i].blockAddress;

          // Move to new node in main memory.
          cursor = (Node *)mainMemoryNode;
          break;
        }
      }
    }

    // When we reach here, it means we have hit a leaf node. Let's find a place to put our new record in.
    // If this leaf node still has space to insert a key, then find out where to put it.
    if (cursor->numKeys < maxKeys)
    {
      // Update the last pointer to point to the previous last pointer's node. Aka maintain cursor -> Y linked list.
      cursor->pointers[cursor->numKeys + 1] = cursor->pointers[cursor->numKeys];

      int i = 0;
      // While we haven't reached the last key and the key we want to insert is larger than current key, keep moving forward.
      while (key > cursor->keys[i] && i < cursor->numKeys)
      {
        i++;
      }

      // Now i represents the index we want to put our key in. We need to shift all keys in the node back to fit it in.
      // Swap from number of keys + 1 (empty key) backwards, moving our last key back and so on. We also need to swap pointers.
      for (int j = cursor->numKeys; j > i; j--)
      {
        // Just do a simple bubble swap from the back to preserve index order.
        cursor->keys[j] = cursor->keys[j - 1];
        cursor->pointers[j] = cursor->pointers[j - 1];
      }

      // Insert our new key and pointer into this node.
      cursor->keys[i] = key;
      cursor->pointers[i] = address;
      cursor->numKeys++;

      cout << cursor->keys[i] << endl;

      // Now insert operation is complete, we need to store this updated node to disk.
      // cursorDiskAddress is the address of node in disk, cursor is the address of node in main memory.
      // In this case, we count read/writes as 1 I/O only (Assume block remains in main memory).
      std::memcpy(cursorDiskAddress, cursor, nodeSize);
    }
    // Overflow: If there's no space to insert new key, we have to split this node into two and update the parent if required.
    else
    {
      // Create a new leaf node to put half the keys and pointers in.
      Node *newLeaf = new Node(maxKeys);
      newLeaf->isLeaf = true; // New node is a leaf node.

      // Update nodes count
      numNodes++;

      // Copy all current keys and pointers (including new key to insert) to a temporary list.
      float tempKeyList[maxKeys + 1];

      // We only need to store pointers corresponding to records (ignore those that points to other nodes).
      // Those that point to other nodes can be manipulated by themselves without this array later.
      Address tempPointerList[maxKeys + 1];

      // Copy all keys and pointers to the temporary lists.
      int i = 0;
      for (i = 0; i < maxKeys; i++)
      {
        tempKeyList[i] = cursor->keys[i];
        tempPointerList[i] = cursor->pointers[i];
      }

      // Insert the new key into the temp key list, making sure that it remains sorted. Here, we find where to insert it.
      i = 0;
      while (key > tempKeyList[i] && i < maxKeys)
      {
        i++;
      }

      // The key should be inserted at index i in the temporary lists. Move all elements back.
      for (int j = maxKeys; j > i; j--)
      {
        // Bubble swap all elements (keys and pointers) backwards by one index.
        tempKeyList[j] = tempKeyList[j - 1];
        tempPointerList[j] = tempPointerList[j - 1];
      }

      // Insert the new key and pointer into the temporary lists.
      tempKeyList[i] = key;
      tempPointerList[i] = address;

      // Split the two new nodes into two. ⌊(n+1)/2⌋ keys for left, n+1 - ⌊(n+1)/2⌋ (aka remaining) keys for right.
      cursor->numKeys = (maxKeys + 1) / 2;
      newLeaf->numKeys = (maxKeys + 1) - (maxKeys + 1) / 2;

      // Set the last pointer of the new leaf node to point to the previous last pointer of the existing node (cursor).
      // Essentially newLeaf -> Y, where Y is some other leaf node pointer wherein cursor -> Y previously.
      // We use maxKeys since cursor was previously full, so last pointer's index is maxKeys.
      newLeaf->pointers[newLeaf->numKeys] = cursor->pointers[maxKeys];

      // Set the new last pointer of the existing cursor to point to the new leaf node (linked list).
      // Effectively, it was cursor -> Y, now it's cursor -> newLeaf -> Y, where Y is some other leaf node.
      // We need to save the new leaf node to the disk and store that disk address in the pointer.
      Address newLeafAddress = index->allocate(nodeSize);
      cursor->pointers[cursor->numKeys] = newLeafAddress;

      // Now we need to deal with the rest of the keys and pointers.
      // Note that since we are at a leaf node, pointers point directly to records on disk.

      // Add in keys and pointers in both the existing node, and the new leaf node.
      // First, the existing node (cursor).
      for (i = 0; i < cursor->numKeys; i++)
      {
        cursor->keys[i] = tempKeyList[i];
        cursor->pointers[i] = tempPointerList[i];
      }

      // Then, the new leaf node. Note we keep track of the i index, since we are using the remaining keys and pointers.
      for (int j = 0; j < newLeaf->numKeys; i++, j++)
      {
        newLeaf->keys[j] = tempKeyList[i];
        newLeaf->pointers[j] = tempPointerList[i];
      }

      // Now that we have finished updating the two new leaf nodes, we need to write them to disk.
      std::memcpy(cursorDiskAddress, cursor, nodeSize);
      std::memcpy(newLeafAddress.blockAddress, newLeaf, nodeSize);

      // If we are at root (aka root == leaf), then we need to make a new parent root.
      if (cursor == root)
      {
        Node *newRoot = new Node(maxKeys);

        // We need to set the new root's key to be the left bound of the right child.
        newRoot->keys[0] = newLeaf->keys[0];

        // Point the new root's children as the existing node and the new node.
        newRoot->pointers[0] = {cursorDiskAddress, 0};
        newRoot->pointers[1] = newLeafAddress;

        // Update new root's variables.
        newRoot->isLeaf = false;
        newRoot->numKeys = 1;
        root = newRoot;

        // Add new node and level to the tree.
        numNodes++;
        levels++;

        // Write the new root node to disk and update the root disk address stored in B+ Tree.
        Address newRootAddress = index->allocate(nodeSize);

        rootAddress = newRootAddress.blockAddress;
        std::memcpy(rootAddress, root, nodeSize);
      }
      // If we are not at the root, we need to insert a new parent in the middle levels of the tree.
      else
      {
        //insertInternal(newLeaf->keys[0], parent, (Node *)parentDiskAddress, newLeaf, (Node *)newLeafAddress.blockAddress);
      }
    }
  }
}

// // Updates the parent node to point at both child nodes, and adds a parent node if needed.
// // Takes the lower bound of the right child, and the main memory address of the parent and the new child,
// // as well as disk address of parent and new child.
// void BPlusTree::insertInternal(float key, Node *cursor, Node *cursorDiskAddress, Node *newChild, Node *newChildDiskAddress)
// {
//   // If parent (cursor) still has space, we can simply add the child node as a pointer.
//   // We don't have to load parent from the disk again since we still have a main memory pointer to it.
//   if (cursor->numKeys < maxKeys)
//   {
//     // Iterate through the parent to see where to put in the lower bound key for the new child.
//     int i = 0;
//     while (key > cursor->keys[i] && i < cursor->numKeys)
//     {
//       i++;
//     }

//     // Now we have i, the index to insert the key in. Bubble swap all keys back to insert the new child's key.
//     for (int j = cursor->numKeys; j > i; j--)
//     {
//       cursor->keys[j] = cursor->keys[j - 1];
//     }

//     // Shift all pointers one step right (right pointer of key points to lower bound of key).
//     for (int j = cursor->numKeys + 1; j > i + 1; j--)
//     {
//       cursor->pointers[j] = cursor->pointers[j - 1];
//     }

//     // Add in new child's lower bound key and pointer to the parent.
//     cursor->keys[i] = key;
//     cursor->numKeys++;

//     // Right side pointer of key will point to the new child node.
//     cursor->pointers[i + 1] = {newChildDiskAddress, 0};
//   }
//   // If parent node doesn't have space, we need to recursively split parent node and insert more parent nodes.
//   else
//   {
//     // Make new internal node (split this parent node into two).
//     // Note: We DO NOT add a new key, just a new pointer!
//     Node *newInternal = new Node(maxKeys);

//     // Increment nodes in the tree
//     numNodes++;

//     // Same logic as above, keep a temp list of keys and pointers to insert into the split nodes.
//     // Now, we have one extra pointer to keep track of (new child's pointer).
//     float tempKeyList[maxKeys + 1];
//     Address tempPointerList[maxKeys + 2];

//     // Copy all keys into a temp key list.
//     for (int i = 0; i < maxKeys; i++)
//     {
//       tempKeyList[i] = cursor->keys[i];
//     }

//     // Copy all pointers into a temp pointer list.
//     for (int i = 0; i < maxKeys + 1; i++)
//     {
//       tempPointerList[i] = cursor->pointers[i];
//     }

//     // Find index to insert key in.
//     int i = 0;
//     while (key > tempKeyList[i] && i < maxKeys)
//     {
//       i++;
//     }

//     // Swap all elements higher than index backwards to fit new key.
//     int j;
//     for (int j = maxKeys; j > i; j--)
//     {
//       tempKeyList[j] = tempKeyList[j - 1];
//     }

//     // Insert new key into array in the correct spot (sorted).
//     tempKeyList[i] = key;

//     //
//     for (int j = maxKeys + 1; j > i + 1; j--)
//     {
//       tempPointerList[j] = tempPointerList[j - 1];
//     }

//     tempPointerList[i + 1] = child;
//     newInternal->isLeaf = false;

//     // Split the two new nodes into two. ⌊(n+1)/2⌋ keys for left, n+1 - ⌊(n+1)/2⌋ (aka remaining) keys for right.
//     cursor->numKeys = (maxKeys + 1) / 2;
//     newLeaf->numKeys = (maxKeys + 1) - (maxKeys + 1) / 2;

//     for (i = 0, j = cursor->num_keys + 1; i < newInternal->num_keys; i++, j++)
//     {
//       newInternal->keys[i] = virtualKey[j];
//     }
//     for (i = 0, j = cursor->num_keys + 1; i < newInternal->num_keys + 1; i++, j++)
//     {
//       newInternal->pointers[i] = virtualPtr[j];
//     }
//     if (cursor == root)
//     {
//       Node *newRoot = new Node;
//       newRoot->keys[0] = cursor->keys[cursor->num_keys];
//       newRoot->pointers[0] = cursor;
//       newRoot->pointers[1] = newInternal;
//       newRoot->IS_LEAF = false;
//       newRoot->num_keys = 1;
//       root = newRoot;
//     }
//     else
//     {
//       insertInternal(cursor->keys[cursor->num_keys], findParent(root, cursor), newInternal);
//     }
//   }
// }

// // Find the parent
// Node *BPTree::findParent(Node *cursor, Node *child)
// {
//   Node *parent;
//   if (cursor->IS_LEAF || (cursor->pointers[0])->IS_LEAF)
//   {
//     return NULL;
//   }
//   for (int i = 0; i < cursor->num_keys + 1; i++)
//   {
//     if (cursor->pointers[i] == child)
//     {
//       parent = cursor;
//       return parent;
//     }
//     else
//     {
//       parent = findParent(cursor->pointers[i], child);
//       if (parent != NULL)
//         return parent;
//     }
//   }
//   return parent;
// }

// Print the tree
void BPlusTree::display(Node *cursor, int level)
{
  if (cursor != nullptr)
  {
    cout << cursor;
    for (int i = 0; i < level; i++)
    {
      cout << "   ";
    }
    cout << " level " << level << ": ";

    for (int i = 0; i < cursor->numKeys; i++)
    {
      cout << cursor->keys[i] << " ";
    }

    for (int i = cursor->numKeys; i < maxKeys; i++)
    {
      cout << "x ";
    }

    for (int i = 0; i < maxKeys + 1; i++)
    {
      if (cursor->pointers[i].blockAddress == NULL)
      {
        cout << "|       |";
      }
      else
      {
        cout << cursor->pointers[i].blockAddress << " ";
      }
    }

    cout << "\n";
    if (cursor->isLeaf != true)
    {
      for (int i = 0; i < cursor->numKeys + 1; i++)
      {
        // Load node in from disk to main memory.
        void *mainMemoryNode = operator new(nodeSize);
        std::memmove(mainMemoryNode, cursor->pointers[i].blockAddress, nodeSize);

        display((Node *)mainMemoryNode, level + 1);
      }
    }
  }
}

void b_plus_tree_test()
{
  // Create memory pools for the disk.
  BPlusTree tree = BPlusTree(100);

  unsigned char testt[100];

  for (int i = 0; i < 3; i++)
  {
    tree.insert({&testt, 0}, i);
  }

  tree.displayNode(tree.getRoot());
  //tree.display(tree.getRoot(), 1);
}