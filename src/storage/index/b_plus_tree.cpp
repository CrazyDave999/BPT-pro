#include <string>

#include "storage/index/b_plus_tree.h"

namespace CrazyDave {

INDEX_TEMPLATE_ARGUMENTS
BPLUSTREE_TYPE::BPlusTree(std::string name, page_id_t header_page_id, BufferPoolManager *buffer_pool_manager,
                          int leaf_max_size, int internal_max_size)
    : index_name_(std::move(name)),
      bpm_(buffer_pool_manager),
      leaf_max_size_(leaf_max_size),
      internal_max_size_(internal_max_size),
      header_page_id_(header_page_id) {
  //  std::cout << "Hello from asshole debugger CrazyDave.\nConstructing BPlusTree.\nleaf_max_size: " << leaf_max_size_
  //            << ", internal_max_size: " << internal_max_size_ << "\n";  // debug
  if (bpm_->IsNew()) {
    WritePageGuard guard = bpm_->FetchPageWrite(header_page_id_);
    auto root_page = guard.AsMut<BPlusTreeHeaderPage>();
    root_page->root_page_id_ = INVALID_PAGE_ID;
  }
}

/*
 * Helper function to decide whether current b+tree is empty
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::IsEmpty() const -> bool {
  auto guard = bpm_->FetchPageRead(header_page_id_);
  auto root_page = guard.As<BPlusTreeHeaderPage>();
  return root_page->root_page_id_ == INVALID_PAGE_ID;
}

INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::Find(const KeyType &key, vector<KeyType> *result) {
  auto header_page_guard = bpm_->FetchPageRead(header_page_id_);
  auto header_page = header_page_guard.As<BPlusTreeHeaderPage>();
  if (header_page->root_page_id_ == INVALID_PAGE_ID) {
    header_page_guard.Drop();
    return;
  }
  auto guard = bpm_->FetchPageRead(header_page->root_page_id_);
  header_page_guard.Drop();
  Find(key, result, guard);
}

INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::Find(const KeyType &key, vector<KeyType> *result, ReadPageGuard &guard) {
  auto *page = guard.template As<BPlusTreePage>();
  if (page->IsLeafPage()) {
    auto leaf_page = reinterpret_cast<const LeafPage *>(page);
    int l = leaf_page->LowerBoundByFirst(key, comparator_);
    int r = leaf_page->UpperBoundByFirst(key, comparator_);
    for (int i = l; i < r; ++i) {
      result->push_back(leaf_page->KeyAt(i));
    }
    guard.Drop();
    return;
  }
  auto internal_page = reinterpret_cast<const InternalPage *>(page);
  int l = internal_page->LowerBoundByFirst(key, comparator_) - 1;
  int r = internal_page->UpperBoundByFirst(key, comparator_) - 1;
  std::vector<ReadPageGuard> n_guards;
  for (int i = l; i <= r; ++i) {
    n_guards.push_back(bpm_->FetchPageRead(internal_page->ValueAt(i)));
  }
  guard.Drop();
  for (auto &grd : n_guards) {
    Find(key, result, grd);
  }
}
/*****************************************************************************
 * INSERTION
 *****************************************************************************/
/*
 * Insert constant key & value pair into b+ tree
 * if current tree is empty, start new tree, update root page id and insert
 * entry, otherwise insert into leaf page.
 * @return: since we only support unique key, if user try to insert duplicate
 * keys return false, otherwise return true.
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Insert(const KeyType &key, const ValueType &value) -> bool {
  auto pr = Insert(key, value, Protocol::Optimistic);
  if (pr.first) {
    return true;
  }
  if (pr.second) {
    return Insert(key, value, Protocol::Pessimistic).first;
  }
  return false;
}

/*****************************************************************************
 * REMOVE
 *****************************************************************************/
/*
 * Delete key & value pair associated with input key
 * If current tree is empty, return immediately.
 * If not, User needs to first find the right leaf page as deletion target, then
 * delete entry from leaf page. Remember to deal with redistribute or merge if
 * necessary.
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::Remove(const KeyType &key) {
  auto pr = Remove(key, Protocol::Optimistic);
  if (!pr.first && pr.second) {
    Remove(key, Protocol::Pessimistic);
  }
}

/*****************************************************************************
 * INDEX ITERATOR
 *****************************************************************************/
/*
 * Input parameter is void, find the leftmost leaf page first, then construct
 * index iterator
 * @return : index iterator
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Begin() -> INDEXITERATOR_TYPE {
  auto header_page = bpm_->FetchPageRead(header_page_id_).As<BPlusTreeHeaderPage>();
  if (header_page->root_page_id_ == INVALID_PAGE_ID) {
    return End();
  }

  auto guard = bpm_->FetchPageRead(header_page->root_page_id_);
  auto bpt_page = guard.As<BPlusTreePage>();
  page_id_t page_id = header_page->root_page_id_;
  while (!bpt_page->IsLeafPage()) {
    auto internal_page = reinterpret_cast<const InternalPage *>(bpt_page);
    page_id = internal_page->ValueAt(0);
    guard = bpm_->FetchPageRead(page_id);
    bpt_page = guard.As<BPlusTreePage>();
  }
  return {bpm_, page_id};
}

/*
 * Input parameter is low key, find the leaf page that contains the input key
 * first, then construct index iterator
 * @return : index iterator
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Begin(const KeyType &key) -> INDEXITERATOR_TYPE {
  auto header_page = bpm_->FetchPageRead(header_page_id_).As<BPlusTreeHeaderPage>();
  if (header_page->root_page_id_ == INVALID_PAGE_ID) {
    return End();
  }

  auto guard = bpm_->FetchPageRead(header_page->root_page_id_);
  auto bpt_page = guard.As<BPlusTreePage>();
  page_id_t page_id = header_page->root_page_id_;
  while (!bpt_page->IsLeafPage()) {
    auto internal_page = reinterpret_cast<const InternalPage *>(bpt_page);
    auto l = UpperBound(internal_page, key) - 1;
    page_id = internal_page->ValueAt(l);
    guard = bpm_->FetchPageRead(page_id);
    bpt_page = guard.As<BPlusTreePage>();
  }
  auto leaf_page = reinterpret_cast<const LeafPage *>(bpt_page);
  auto l = BinarySearch(leaf_page, key);
  if (l != -1) {
    return {bpm_, page_id, l};
  }
  return End();
}

/*
 * Input parameter is void, construct an index iterator representing the end
 * of the key/value pair in the leaf node
 * @return : index iterator
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::End() -> INDEXITERATOR_TYPE { return {bpm_, INVALID_PAGE_ID}; }

/**
 * @return Page id of the root of this tree
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::GetRootPageId() -> page_id_t {
  auto guard = bpm_->FetchPageRead(header_page_id_);
  auto header_page = guard.As<BPlusTreeHeaderPage>();
  return header_page->root_page_id_;
}

INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::LowerBound(const LeafPage *page, const KeyType &key) const -> int {
  int l = 0;
  int r = page->GetSize();
  while (l < r) {
    int mid = (l + r) >> 1;
    if (comparator_(page->KeyAt(mid), key) == -1) {
      l = mid + 1;
    } else {
      r = mid;
    }
  }
  return l;
}

INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::LowerBound(const InternalPage *page, const KeyType &key) const -> int {
  int l = 0;
  int r = page->GetSize();
  while (l < r) {
    int mid = (l + r) >> 1;
    if (comparator_(page->KeyAt(mid), key) == -1) {
      l = mid + 1;
    } else {
      r = mid;
    }
  }
  return l;
}

INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::BinarySearch(const LeafPage *page, const KeyType &key) const -> int {
  auto l = LowerBound(page, key);
  if (l == page->GetSize() || comparator_(key, page->KeyAt(l)) != 0) {
    return -1;
  }
  return l;
}

INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::UpperBound(const BPlusTree::LeafPage *page, const KeyType &key) const -> int {
  int l = 0;
  int r = page->GetSize();
  while (l < r) {
    int mid = (l + r) >> 1;
    if (comparator_(key, page->KeyAt(mid)) == -1) {
      r = mid;
    } else {
      l = mid + 1;
    }
  }
  return l;
}

INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::UpperBound(const InternalPage *page, const KeyType &key) const -> int {
  int l = 1;
  int r = page->GetSize();
  while (l < r) {
    int mid = (l + r) >> 1;
    if (comparator_(key, page->KeyAt(mid)) == -1) {
      r = mid;
    } else {
      l = mid + 1;
    }
  }
  return l;
}

INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::InsertKeyValue(LeafPage *page, const KeyType &key, const ValueType &value) const -> bool {
  int l = 0;
  int r = page->GetSize();
  while (l < r) {
    int mid = (l + r) >> 1;
    if (comparator_(page->KeyAt(mid), key) == -1) {
      l = mid + 1;
    } else {
      r = mid;
    }
  }
  if (l == page->GetSize() || comparator_(key, page->KeyAt(l)) != 0) {
    page->InsertAt(l, key, value);
    return true;
  }
  return false;
}

INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::SplitLeafPage(LeafPage *page, page_id_t *n_page_id, Context &ctx) -> LeafPage * {
  auto n_page_guard = bpm_->NewPageGuarded(n_page_id);
  auto *n_page = n_page_guard.AsMut<LeafPage>();
  n_page->Init(leaf_max_size_);
  auto size = page->GetSize();
  for (int i = size >> 1; i < size; ++i) {
    n_page->InsertAt(n_page->GetSize(), page->PairAt(i));
  }
  page->SetSize(size >> 1);
  n_page->SetNextPageId(page->GetNextPageId());
  page->SetNextPageId(*n_page_id);
  if (ctx.IsRootPage(ctx.write_set_.back().PageId())) {  // 根是叶子，新根
    page_id_t n_root_page_id;
    auto n_root_guard = bpm_->NewPageGuarded(&n_root_page_id);
    auto *n_root_page = n_root_guard.AsMut<InternalPage>();
    n_root_page->Init(internal_max_size_);
    if (ctx.header_write_guard_.has_value()) {
      ctx.header_write_guard_->AsMut<BPlusTreeHeaderPage>()->root_page_id_ = n_root_page_id;
    }
    n_root_page->InsertAt(0, KeyType(), ctx.root_page_id_);
    InsertKeyValue(n_root_page, n_page->KeyAt(0), *n_page_id);
    ctx.write_set_.pop_back();
    return n_page;
  }
  ctx.write_set_.pop_back();
  auto *p_page = ctx.write_set_.back().template AsMut<InternalPage>();
  InsertKeyValue(p_page, n_page->KeyAt(0), *n_page_id);
  return n_page;
}

INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::InsertKeyValue(InternalPage *page, const KeyType &key, const page_id_t &value) {
  auto l = UpperBound(page, key);
  page->InsertAt(l, key, value);
}

INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::SplitInternalPage(InternalPage *page, page_id_t *n_page_id, Context &ctx) -> InternalPage * {
  auto n_page_guard = bpm_->NewPageGuarded(n_page_id);
  auto *n_page = n_page_guard.AsMut<InternalPage>();
  n_page->Init(internal_max_size_);
  auto size = page->GetSize();
  for (int i = size >> 1; i < size; ++i) {
    n_page->InsertAt(n_page->GetSize(), page->PairAt(i));
  }
  page->SetSize(size >> 1);
  if (ctx.IsRootPage(ctx.write_set_.back().PageId())) {  // 新根
    page_id_t n_root_page_id;
    auto n_root_guard = bpm_->NewPageGuarded(&n_root_page_id);
    auto *n_root_page = n_root_guard.AsMut<InternalPage>();
    n_root_page->Init(internal_max_size_);
    if (ctx.header_write_guard_.has_value()) {
      ctx.header_write_guard_->AsMut<BPlusTreeHeaderPage>()->root_page_id_ = n_root_page_id;
    }
    n_root_page->InsertAt(0, KeyType(), ctx.root_page_id_);
    InsertKeyValue(n_root_page, n_page->KeyAt(0), *n_page_id);
    ctx.write_set_.pop_back();
    return n_page;
  }
  ctx.write_set_.pop_back();
  auto *p_page = ctx.write_set_.back().template AsMut<InternalPage>();
  InsertKeyValue(p_page, n_page->KeyAt(0), *n_page_id);
  return n_page;
}

INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::RemoveKeyValue(LeafPage *page, const KeyType &key) {
  int l = BinarySearch(page, key);
  if (l != -1) {
    page->RemoveAt(l);
  }
}

INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::TryAdoptFromNeighbor(LeafPage *page, Context &ctx) -> bool {
  //  std::cout << "Trying to adopt a child from neighbor. Type: leaf_page\n Before: " << page->ToString()
  //            << "\n";  // debug
  //  auto *p_page = ctx.write_set_[ctx.write_set_.size() - 2].AsMut<InternalPage>();
  auto it = ctx.write_set_.end();
  --it, --it;
  auto *p_page = it->AsMut<InternalPage>();
  int l = ctx.index_set_.back();
  if (l < p_page->GetSize() - 1) {
    auto r_page_id = p_page->ValueAt(l + 1);
    auto r_page_guard = bpm_->FetchPageWrite(r_page_id);
    auto *r_page = r_page_guard.template AsMut<LeafPage>();
    if (r_page->GetSize() > r_page->GetMinSize()) {
      page->InsertAt(page->GetSize(), r_page->PairAt(0));
      r_page->RemoveAt(0);
      p_page->SetKeyAt(l + 1, r_page->KeyAt(0));
      //      std::cout << "Successfully adopted " << key << ", " << value
      //                << "from right neighbor.\n After: " << page->ToString() << "\n";  // debug
      ctx.write_set_.pop_back();
      ctx.index_set_.pop_back();
      return true;
    }
  }
  if (l > 0) {
    auto l_page_id = p_page->ValueAt(l - 1);
    auto l_page_guard = bpm_->FetchPageWrite(l_page_id);
    auto *l_page = l_page_guard.template AsMut<LeafPage>();
    if (l_page->GetSize() > l_page->GetMinSize()) {
      page->InsertAt(0, l_page->PairAt(l_page->GetSize() - 1));
      l_page->RemoveAt(l_page->GetSize() - 1);
      p_page->SetKeyAt(l, page->KeyAt(0));
      //      std::cout << "Successfully adopted " << key << ", " << value
      //                << "from left neighbor.\n After: " << page->ToString() << "\n";  // debug
      ctx.write_set_.pop_back();
      ctx.index_set_.pop_back();
      return true;
    }
  }
  //  std::cout << "Adoption failed. Consider merging leaf page.\n";  // debug
  return false;
}
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::MergeLeafPage(LeafPage *page, Context &ctx) {
  // 必须先 TryAdoptFromNeighbor，再考虑 MergeLeafPage。领养失败则必定能合并
  //  std::cout << "Merging a page. Type: leaf_page.\n Before: " << page->ToString() << "\n";  // debug
  //  auto *p_page = ctx.write_set_[ctx.write_set_.size() - 2].AsMut<InternalPage>();
  auto it = ctx.write_set_.end();
  --it, --it;
  auto *p_page = it->AsMut<InternalPage>();
  int l = ctx.index_set_.back();
  if (l < p_page->GetSize() - 1) {
    auto r_page_id = p_page->ValueAt(l + 1);
    auto r_page_guard = bpm_->FetchPageWrite(r_page_id);
    auto *r_page = r_page_guard.template AsMut<LeafPage>();
    //    std::cout << "Merging r_page: " << r_page->ToString() << " to page: " << page->ToString() << "\n";  // debug
    for (int i = 0; i < r_page->GetSize(); ++i) {
      page->InsertAt(page->GetSize(), r_page->PairAt(i));
    }
    r_page->SetSize(0);
    page->SetNextPageId(r_page->GetNextPageId());
    p_page->RemoveAt(l + 1);
    bpm_->DeletePage(r_page_id);
    ctx.write_set_.pop_back();
    ctx.index_set_.pop_back();
    //    std::cout << "Successfully merged. After merging, page: " << page->ToString() << "\n";  // debug
    return;
  }
  auto l_page_id = p_page->ValueAt(l - 1);
  auto l_page_guard = bpm_->FetchPageWrite(l_page_id);
  auto *l_page = l_page_guard.template AsMut<LeafPage>();
  //  std::cout << "Merging page: " << page->ToString() << " to l_page: " << l_page->ToString() << "\n";  // debug
  for (int i = 0; i < page->GetSize(); ++i) {
    l_page->InsertAt(l_page->GetSize(), page->PairAt(i));
  }
  page->SetSize(0);
  l_page->SetNextPageId(page->GetNextPageId());
  p_page->RemoveAt(l);
  bpm_->DeletePage(p_page->ValueAt(l));
  ctx.write_set_.pop_back();
  ctx.index_set_.pop_back();
  //  std::cout << "Successfully merged. After merging, l_page: " << l_page->ToString() << "\n";  // debug
}
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::TryAdoptFromNeighbor(InternalPage *page, Context &ctx) -> bool {
  //  std::cout << "Trying to adopt a child from neighbor. Type: leaf_page\n Before: " << page->ToString()
  //            << "\n";  // debug
  //  auto *p_page = ctx.write_set_[ctx.write_set_.size() - 2].AsMut<InternalPage>();
  auto it = ctx.write_set_.end();
  --it, --it;
  auto *p_page = it->AsMut<InternalPage>();
  int l = ctx.index_set_.back();
  if (l < p_page->GetSize() - 1) {
    auto r_page_id = p_page->ValueAt(l + 1);
    auto r_page_guard = bpm_->FetchPageWrite(r_page_id);
    auto *r_page = r_page_guard.template AsMut<InternalPage>();
    if (r_page->GetSize() > r_page->GetMinSize()) {
      page->InsertAt(page->GetSize(), r_page->PairAt(0));
      r_page->RemoveAt(0);
      p_page->SetKeyAt(l + 1, r_page->KeyAt(0));
      //      std::cout << "Successfully adopted " << key << ", " << value
      //                << "from right neighbor.\n After: " << page->ToString() << "\n";  // debug
      ctx.write_set_.pop_back();
      ctx.index_set_.pop_back();
      return true;
    }
  }
  if (l > 0) {
    auto l_page_id = p_page->ValueAt(l - 1);
    auto l_page_guard = bpm_->FetchPageWrite(l_page_id);
    auto *l_page = l_page_guard.template AsMut<InternalPage>();
    if (l_page->GetSize() > l_page->GetMinSize()) {
      page->InsertAt(0, l_page->PairAt(l_page->GetSize() - 1));
      l_page->RemoveAt(l_page->GetSize() - 1);
      p_page->SetKeyAt(l, page->KeyAt(0));
      //      std::cout << "Successfully adopted " << key << ", " << value
      //                << "from left neighbor.\n After: " << page->ToString() << "\n";  // debug
      ctx.write_set_.pop_back();
      ctx.index_set_.pop_back();
      return true;
    }
  }
  //  std::cout << "Adoption failed. Consider merging leaf page.\n";  // debug
  return false;
}
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::MergeInternalPage(InternalPage *page, Context &ctx) {
  // 必须先 TryAdoptFromNeighbor，再考虑 MergeLeafPage。领养失败则必定能合并
  //  std::cout << "Merging a page. Type: leaf_page.\n Before: " << page->ToString() << "\n";  // debug
  //  auto *p_page = ctx.write_set_[ctx.write_set_.size() - 2].AsMut<InternalPage>();
  auto it = ctx.write_set_.end();
  --it, --it;
  auto *p_page = it->AsMut<InternalPage>();
  int l = ctx.index_set_.back();
  if (l < p_page->GetSize() - 1) {
    auto r_page_id = p_page->ValueAt(l + 1);
    auto r_page_guard = bpm_->FetchPageWrite(r_page_id);
    auto *r_page = r_page_guard.template AsMut<InternalPage>();
    //    std::cout << "Merging r_page: " << r_page->ToString() << " to page: " << page->ToString() << "\n";  // debug
    for (int i = 0; i < r_page->GetSize(); ++i) {
      page->InsertAt(page->GetSize(), r_page->PairAt(i));
    }
    r_page->SetSize(0);
    p_page->RemoveAt(l + 1);
    bpm_->DeletePage(r_page_id);
    ctx.write_set_.pop_back();
    ctx.index_set_.pop_back();
    //    std::cout << "Successfully merged. After merging, page: " << page->ToString() << "\n";  // debug
    return;
  }
  auto l_page_id = p_page->ValueAt(l - 1);
  auto l_page_guard = bpm_->FetchPageWrite(l_page_id);
  auto *l_page = l_page_guard.template AsMut<InternalPage>();
  //  std::cout << "Merging page: " << page->ToString() << " to l_page: " << l_page->ToString() << "\n";  // debug
  for (int i = 0; i < page->GetSize(); ++i) {
    l_page->InsertAt(l_page->GetSize(), page->PairAt(i));
  }
  page->SetSize(0);
  p_page->RemoveAt(l);
  bpm_->DeletePage(p_page->ValueAt(l));
  ctx.write_set_.pop_back();
  ctx.index_set_.pop_back();
  //  std::cout << "Successfully merged. After merging, l_page: " << l_page->ToString() << "\n";  // debug
}

INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Insert(const KeyType &key, const ValueType &value,
                            BPlusTree::Protocol protocol) -> pair<bool, bool> {
  Context ctx;
  if (protocol == Protocol::Pessimistic) {
    ctx.header_write_guard_ = bpm_->FetchPageWrite(header_page_id_);
    ctx.root_page_id_ = ctx.header_write_guard_->AsMut<BPlusTreeHeaderPage>()->root_page_id_;
    if (ctx.root_page_id_ == INVALID_PAGE_ID) {
      page_id_t n_root_page_id;
      auto n_root_guard = bpm_->NewPageGuarded(&n_root_page_id);
      auto *n_root_page = n_root_guard.AsMut<LeafPage>();
      n_root_page->Init(leaf_max_size_);
      auto *header_page = ctx.header_write_guard_->AsMut<BPlusTreeHeaderPage>();
      header_page->root_page_id_ = n_root_page_id;
      n_root_page->InsertAt(0, key, value);
      return {true, true};
    }

    ctx.write_set_.push_back(bpm_->FetchPageWrite(ctx.root_page_id_));
    auto bpt_page = ctx.write_set_.back().AsMut<BPlusTreePage>();
    while (!bpt_page->IsLeafPage()) {
      if (bpt_page->GetSize() < bpt_page->GetMaxSize()) {  // safe
        while (ctx.write_set_.size() > 1) {
          ctx.write_set_.pop_front();
        }
      }
      auto *internal_page = reinterpret_cast<InternalPage *>(bpt_page);
      auto l = UpperBound(internal_page, key) - 1;
      ctx.write_set_.push_back(bpm_->FetchPageWrite(internal_page->ValueAt(l)));
      bpt_page = ctx.write_set_.back().AsMut<BPlusTreePage>();
    }
    auto *leaf_page = reinterpret_cast<LeafPage *>(bpt_page);
    if (InsertKeyValue(leaf_page, key, value)) {
      if (leaf_page->GetSize() == leaf_page->GetMaxSize()) {
        page_id_t n_page_id;
        SplitLeafPage(leaf_page, &n_page_id, ctx);
        InternalPage *internal_page;
        while (ctx.write_set_.size() > 1) {
          internal_page = ctx.write_set_.back().AsMut<InternalPage>();
          SplitInternalPage(internal_page, &n_page_id, ctx);
        }
        // 三种可能
        // 1. 根是leaf，在Split操作中已经完成根的更新，ctx.write_set_为空
        // 2. 在根以下没有遇到过safe node，ctx.write_set_中有一个元素，是根的写锁，根有可能需要分裂
        // 3. 在根以下遇到过safe node，ctx.write_set_中有一个元素，是safe node
        if (!ctx.write_set_.empty()) {
          internal_page = ctx.write_set_.back().template AsMut<InternalPage>();
          if (internal_page->GetSize() > internal_page->GetMaxSize()) {
            SplitInternalPage(internal_page, &n_page_id, ctx);
          }
        }
      }
      return {true, false};
    }
    return {false, false};
  }
  ctx.header_read_guard_ = bpm_->FetchPageRead(header_page_id_);
  ctx.root_page_id_ = ctx.header_read_guard_->As<BPlusTreeHeaderPage>()->root_page_id_;
  if (ctx.root_page_id_ == INVALID_PAGE_ID) {
    return {false, true};
  }

  auto page_id = ctx.root_page_id_;
  ctx.read_set_.push_back(bpm_->FetchPageRead(page_id));
  auto bpt_page = ctx.read_set_.back().As<BPlusTreePage>();
  while (!bpt_page->IsLeafPage()) {
    auto *internal_page = reinterpret_cast<const InternalPage *>(bpt_page);
    auto l = UpperBound(internal_page, key) - 1;
    page_id = internal_page->ValueAt(l);
    ctx.read_set_.push_back(bpm_->FetchPageRead(page_id));
    bpt_page = ctx.read_set_.back().As<BPlusTreePage>();
  }
  ctx.read_set_.pop_back();
  ctx.write_set_.push_back(bpm_->FetchPageWrite(page_id));
  auto *leaf_page = ctx.write_set_.back().template AsMut<LeafPage>();
  auto l = LowerBound(leaf_page, key);
  if (l == leaf_page->GetSize() || comparator_(key, leaf_page->KeyAt(l)) != 0) {
    if (leaf_page->GetSize() < leaf_page->GetMaxSize() - 1) {
      leaf_page->InsertAt(l, key, value);
      return {true, false};
    }
    return {false, true};
  }
  return {false, false};
}

INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Remove(const KeyType &key, BPlusTree::Protocol protocol) -> pair<bool, bool> {
  Context ctx;
  if (protocol == Protocol::Pessimistic) {
    // 用栈模拟递归
    ctx.header_write_guard_ = bpm_->FetchPageWrite(header_page_id_);
    ctx.root_page_id_ = ctx.header_write_guard_->AsMut<BPlusTreeHeaderPage>()->root_page_id_;
    if (ctx.root_page_id_ == INVALID_PAGE_ID) {  // 空树
      return {true, false};
    }

    ctx.write_set_.push_back(bpm_->FetchPageWrite(ctx.root_page_id_));
    auto bpt_page = ctx.write_set_.back().AsMut<BPlusTreePage>();
    while (!bpt_page->IsLeafPage()) {
      if (bpt_page->GetSize() > bpt_page->GetMinSize()) {  // safe
        while (ctx.write_set_.size() > 1) {
          ctx.write_set_.pop_front();
          ctx.index_set_.pop_front();
        }
      }
      auto *internal_page = reinterpret_cast<InternalPage *>(bpt_page);
      auto l = UpperBound(internal_page, key) - 1;
      ctx.write_set_.push_back(bpm_->FetchPageWrite(internal_page->ValueAt(l)));
      ctx.index_set_.push_back(l);
      bpt_page = ctx.write_set_.back().AsMut<BPlusTreePage>();
    }
    auto leaf_page = reinterpret_cast<LeafPage *>(bpt_page);
    RemoveKeyValue(leaf_page, key);

    if (leaf_page->GetSize() >= leaf_page->GetMinSize()) {
      return {true, false};
    }
    if (ctx.IsRootPage(ctx.write_set_.back().PageId())) {  // 根就是叶子
      if (leaf_page->GetSize() == 0) {
        ctx.header_write_guard_->AsMut<BPlusTreeHeaderPage>()->root_page_id_ = INVALID_PAGE_ID;
        bpm_->DeletePage(ctx.root_page_id_);
      }
      return {true, false};
    }
    if (TryAdoptFromNeighbor(leaf_page, ctx)) {
      return {true, false};
    }
    MergeLeafPage(leaf_page, ctx);
    auto *page = ctx.write_set_.back().AsMut<InternalPage>();
    while (ctx.write_set_.size() > 1) {
      if (TryAdoptFromNeighbor(page, ctx)) {
        return {true, false};
      }
      MergeInternalPage(page, ctx);
      page = ctx.write_set_.back().AsMut<InternalPage>();
    }
    // 两种可能：
    // 1. ctx.write_set_中仅剩根的写锁，这时有可能根仅剩一个儿子，需要换根
    // 2. ctx.write_set_中仅剩安全节点的写锁，什么都不用做
    if (page->GetSize() == 1) {
      ctx.header_write_guard_->AsMut<BPlusTreeHeaderPage>()->root_page_id_ = page->ValueAt(0);
      bpm_->DeletePage(ctx.root_page_id_);
    }
    return {true, false};
  }
  ctx.header_read_guard_ = bpm_->FetchPageRead(header_page_id_);
  ctx.root_page_id_ = ctx.header_read_guard_->As<BPlusTreeHeaderPage>()->root_page_id_;
  if (ctx.root_page_id_ == INVALID_PAGE_ID) {  // 空树
    return {true, false};
  }
  auto page_id = ctx.root_page_id_;
  ctx.read_set_.push_back(bpm_->FetchPageRead(page_id));
  auto *bpt_page = ctx.read_set_.back().As<BPlusTreePage>();
  while (!bpt_page->IsLeafPage()) {
    auto *internal_page = reinterpret_cast<const InternalPage *>(bpt_page);
    auto l = UpperBound(internal_page, key) - 1;
    page_id = internal_page->ValueAt(l);
    ctx.read_set_.push_back(bpm_->FetchPageRead(page_id));
    ctx.index_set_.push_back(l);
    bpt_page = ctx.read_set_.back().As<BPlusTreePage>();
  }
  ctx.read_set_.pop_back();
  ctx.write_set_.push_back(bpm_->FetchPageWrite(page_id));
  auto *leaf_page = ctx.write_set_.back().template AsMut<LeafPage>();
  auto l = BinarySearch(leaf_page, key);
  if (l == -1) {
    return {true, false};
  }

  if (ctx.IsRootPage(ctx.write_set_.back().PageId())) {  // 根就是叶子
    if (leaf_page->GetSize() == 1) {
      return {false, true};
    }
    leaf_page->RemoveAt(l);
    return {true, false};
  }
  if (leaf_page->GetSize() > leaf_page->GetMinSize()) {
    leaf_page->RemoveAt(l);
    return {true, false};
  }
  return {false, true};
}
// template class BPlusTree<key_t, page_id_t, Comparator>;
template class BPlusTree<pair<size_t, int>, int, Comparator<size_t, int, int>>;
template class BPlusTree<pair<String<65>, int>, int, Comparator<String<65>, int, int>>;
}  // namespace CrazyDave
