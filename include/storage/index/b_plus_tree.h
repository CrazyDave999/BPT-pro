#pragma once
#include <iostream>
#include <optional>
#include <shared_mutex>
#include <string>

#include "common/config.h"
#include "common/utils.h"
#include "data_structures/list.h"
#include "data_structures/vector.h"
#include "storage/index/index_iterator.h"
#include "storage/page/b_plus_tree_header_page.h"
#include "storage/page/b_plus_tree_internal_page.h"
#include "storage/page/b_plus_tree_leaf_page.h"
#include "storage/page/page_guard.h"

namespace CrazyDave {

/**
 * @brief Definition of the Context class.
 *
 * Hint: This class is designed to help you keep track of the pages
 * that you're modifying or accessing.
 */
class Context {
 public:
  // When you insert into / remove from the B+ tree, store the write guard of header page here.
  // Remember to drop the header page guard and set it to nullopt when you want to unlock all.
  std::optional<WritePageGuard> header_write_guard_{std::nullopt};

  std::optional<ReadPageGuard> header_read_guard_{std::nullopt};

  // Save the root page id here so that it's easier to know if the current page is the root page.
  page_id_t root_page_id_{INVALID_PAGE_ID};

  // Store the write guards of the pages that you're modifying here.
  list<WritePageGuard> write_set_;

  // You may want to use this when getting value, but not necessary.
  list<ReadPageGuard> read_set_;

  // Record the index of key in the path.
  list<int> index_set_;

  auto IsRootPage(page_id_t page_id) const -> bool { return page_id == root_page_id_; }
};

#define BPLUSTREE_TYPE BPlusTree<KeyType, ValueType, KeyComparator>

// Main class providing the API for the Interactive B+ Tree.
template <typename KeyType, typename ValueType, typename KeyComparator>
class BPlusTree {
  using InternalPage = BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>;
  using LeafPage = BPlusTreeLeafPage<KeyType, ValueType, KeyComparator>;
  enum class Protocol { Optimistic, Pessimistic };

 public:
  explicit BPlusTree(std::string name, page_id_t header_page_id, BufferPoolManager *buffer_pool_manager,
                     int leaf_max_size = LEAF_PAGE_SIZE, int internal_max_size = INTERNAL_PAGE_SIZE);

  // Returns true if this B+ tree has no keys and values.
  auto IsEmpty() const -> bool;

  // Insert a key-value pair into this B+ tree.
  auto Insert(const KeyType &key, const ValueType &value) -> bool;

  // Remove a key and its value from this B+ tree.
  void Remove(const KeyType &key);

  // Return the value associated with a given key
  void Find(const KeyType &key, vector<KeyType> *result);

  // Return the page id of the root node
  auto GetRootPageId() -> page_id_t;

  // Index iterator
  auto Begin() -> INDEXITERATOR_TYPE;

  auto End() -> INDEXITERATOR_TYPE;

  auto Begin(const KeyType &key) -> INDEXITERATOR_TYPE;

 private:
  auto LowerBound(const LeafPage *page, const KeyType &key) const -> int;

  auto LowerBound(const InternalPage *page, const KeyType &key) const -> int;

  // binary search，找不到返回-1
  auto BinarySearch(const LeafPage *page, const KeyType &key) const -> int;

  auto UpperBound(const LeafPage *page, const KeyType &key) const -> int;
  // upper bound. 返回第一个大于key的index
  auto UpperBound(const InternalPage *page, const KeyType &key) const -> int;

  auto InsertKeyValue(LeafPage *page, const KeyType &key, const ValueType &value) const -> bool;

  auto SplitLeafPage(LeafPage *page, page_id_t *n_page_id, Context &ctx) -> LeafPage *;

  void InsertKeyValue(InternalPage *page, const KeyType &key, const page_id_t &value);

  auto SplitInternalPage(InternalPage *page, page_id_t *n_page_id, Context &ctx) -> InternalPage *;

  void RemoveKeyValue(LeafPage *page, const KeyType &key);

  auto TryAdoptFromNeighbor(LeafPage *page, Context &ctx) -> bool;

  void MergeLeafPage(LeafPage *page, Context &ctx);

  auto TryAdoptFromNeighbor(InternalPage *page, Context &ctx) -> bool;

  void MergeInternalPage(InternalPage *page, Context &ctx);

  /**
   * @return whether insert successfully and if false, whether it is because leaf node unsafe.
   */
  auto Insert(const KeyType &key, const ValueType &value, Protocol protocol) -> pair<bool, bool>;

  /**
   * @return whether insert successfully and if false, whether it is because leaf node unsafe.
   */
  auto Remove(const KeyType &key, Protocol protocol) -> pair<bool, bool>;

  void Find(const KeyType &key, vector<KeyType> *result, ReadPageGuard &guard);

  // member variable
  std::string index_name_;
  BufferPoolManager *bpm_;
  KeyComparator comparator_;
  int leaf_max_size_;
  int internal_max_size_;
  page_id_t header_page_id_;
};

}  // namespace CrazyDave
