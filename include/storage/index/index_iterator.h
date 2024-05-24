/**
 * index_iterator.h
 * For range scan of b+ tree
 */
#pragma once
#include "storage/page/b_plus_tree_leaf_page.h"

namespace CrazyDave {

#define INDEXITERATOR_TYPE IndexIterator<KeyType, ValueType, KeyComparator>

template <typename KeyType, typename ValueType, typename KeyComparator>
class IndexIterator {
 public:
  // you may define your own constructor based on your member variables
  IndexIterator(BufferPoolManager *buffer_pool_manager, page_id_t page_id, int pos = 0);
  ~IndexIterator();  // NOLINT

  auto IsEnd() -> bool;

  auto operator*() -> const MappingType &;

  auto operator++() -> IndexIterator &;

  auto operator==(const IndexIterator &itr) const -> bool {
    if (bpm_ != itr.bpm_) {
      return false;
    }
    if (is_end_) {
      return itr.is_end_;
    }
    return page_id_ == itr.page_id_ && pos_ == itr.pos_;
  }

  auto operator!=(const IndexIterator &itr) const -> bool { return !(this->operator==(itr)); }

 private:
  // add your own private member variables here
  BufferPoolManager *bpm_;
  ReadPageGuard guard_;
  page_id_t page_id_;
  int pos_{0};
  bool is_end_{false};
};

}  // namespace CrazyDave
