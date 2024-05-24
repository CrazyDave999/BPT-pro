#pragma once
#include <string>
#include "storage/page/b_plus_tree_page.h"

namespace CrazyDave {

#define B_PLUS_TREE_INTERNAL_PAGE_TYPE BPlusTreeInternalPage<KeyType, ValueType, KeyComparator>
#define INTERNAL_PAGE_HEADER_SIZE 12
#define INTERNAL_PAGE_SIZE ((BUSTUB_PAGE_SIZE - INTERNAL_PAGE_HEADER_SIZE) / (sizeof(MappingType)))
/**
 * Store n indexed keys and n+1 child pointers (page_id) within internal page.
 * Pointer PAGE_ID(i) points to a subtree in which all keys K satisfy:
 * K(i) <= K < K(i+1).
 * NOTE: since the number of keys does not equal to number of child pointers,
 * the first key always remains invalid. That is to say, any search/lookup
 * should ignore the first key.
 *
 * Internal page format (keys are stored in increasing order):
 *  --------------------------------------------------------------------------
 * | HEADER | KEY(1)+PAGE_ID(1) | KEY(2)+PAGE_ID(2) | ... | KEY(n)+PAGE_ID(n) |
 *  --------------------------------------------------------------------------
 */
template <typename KeyType, typename ValueType, typename KeyComparator>
class BPlusTreeInternalPage : public BPlusTreePage {
 public:
  // Deleted to disallow initialization
  BPlusTreeInternalPage() = delete;

  BPlusTreeInternalPage(const BPlusTreeInternalPage &other) = delete;

  /**
   * Writes the necessary header information to a newly created page, must be called after
   * the creation of a new page to make a valid BPlusTreeInternalPage
   * @param max_size Maximal size of the page
   */
  void Init(int max_size = INTERNAL_PAGE_SIZE - 1);

  /**
   * @param index The index of the key to get. Index must be non-zero.
   * @return Key at index
   */
  auto KeyAt(int index) const -> KeyType;

  /**
   *
   * @param index The index of the key to set. Index must be non-zero.
   * @param key The new value for key
   */
  void SetKeyAt(int index, const KeyType &key);

  /**
   *
   * @param value the value to search for
   */
  auto ValueIndex(const ValueType &value) const -> int;

  /**
   *
   * @param index the index
   * @return the value at the index
   */
  auto ValueAt(int index) const -> ValueType;

  void InsertAt(int index, const KeyType &key, const ValueType &value);

  void RemoveAt(int index);

  auto PairAt(int index) const -> const MappingType &;

  void InsertAt(int index, const MappingType &pair);

  auto LowerBoundByFirst(const KeyType &key, const KeyComparator &cmp) const -> int;

  auto UpperBoundByFirst(const KeyType &key, const KeyComparator &cmp) const -> int;

 private:
  // Flexible array member for page data.
  MappingType array_[0];
};
}  // namespace CrazyDave
