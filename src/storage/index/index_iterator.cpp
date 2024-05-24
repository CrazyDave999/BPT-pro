/**
 * index_iterator.cpp
 */

#include "storage/index/index_iterator.h"
#include "common/utils.h"

namespace CrazyDave {

/*
 * NOTE: you can change the destructor/constructor method here
 * set your own input parameters
 */
INDEX_TEMPLATE_ARGUMENTS
INDEXITERATOR_TYPE::IndexIterator(BufferPoolManager *buffer_pool_manager, page_id_t page_id, int pos)
    : bpm_(buffer_pool_manager), page_id_(page_id), pos_(pos) {
  if (page_id == INVALID_PAGE_ID) {
    is_end_ = true;
  } else {
    guard_ = bpm_->FetchPageRead(page_id);
  }
}

INDEX_TEMPLATE_ARGUMENTS
INDEXITERATOR_TYPE::~IndexIterator() = default;  // NOLINT

INDEX_TEMPLATE_ARGUMENTS
auto INDEXITERATOR_TYPE::IsEnd() -> bool { return is_end_; }

INDEX_TEMPLATE_ARGUMENTS
auto INDEXITERATOR_TYPE::operator*() -> const MappingType & {
  auto *page = guard_.As<B_PLUS_TREE_LEAF_PAGE_TYPE>();
  return page->PairAt(pos_);
}

INDEX_TEMPLATE_ARGUMENTS
auto INDEXITERATOR_TYPE::operator++() -> INDEXITERATOR_TYPE & {
  if (is_end_) {
    return *this;
  }
  auto *page = guard_.As<B_PLUS_TREE_LEAF_PAGE_TYPE>();
  ++pos_;
  if (pos_ == page->GetSize()) {
    auto next_page_id = page->GetNextPageId();
    page_id_ = next_page_id;
    guard_.Drop();
    pos_ = 0;
    if (next_page_id == INVALID_PAGE_ID) {
      is_end_ = true;
    } else {
      guard_ = bpm_->FetchPageRead(next_page_id);
    }
  }
  return *this;
}
//template class IndexIterator<key_t, page_id_t, Comparator>;
template class IndexIterator<pair<size_t, int>, int,Comparator<size_t ,int,int>>;
template class IndexIterator<pair<String<65>, int>, int, Comparator<String<65>, int, int>>;
}  // namespace CrazyDave
