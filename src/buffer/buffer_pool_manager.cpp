#include "buffer/buffer_pool_manager.h"
#include "storage/page/page_guard.h"

namespace CrazyDave {

BufferPoolManager::BufferPoolManager(size_t pool_size, MyDiskManager *disk_manager, size_t replacer_k)
    : pool_size_(pool_size), disk_manager_(disk_manager) {
  // we allocate a consecutive memory space for the buffer pool
  pages_ = new Page[pool_size_];
  replacer_ = new LRUKReplacer{pool_size, replacer_k};

  // Initially, every page is in the free list.
  for (size_t i = 0; i < pool_size_; ++i) {
    free_list_.push_back(static_cast<int>(i));
  }
}

BufferPoolManager::~BufferPoolManager() {
  FlushAllPages();
  delete[] pages_;
  delete replacer_;
}

auto BufferPoolManager::NewPage(page_id_t *page_id) -> Page * {
  frame_id_t fid;
  // latch_.lock();
  if (!free_list_.empty()) {
    fid = free_list_.front();
    free_list_.pop_front();
  } else {
    if (replacer_->Evict(&fid)) {
      if (pages_[fid].IsDirty()) {
        disk_manager_->WritePage(pages_[fid].page_id_, pages_[fid].GetData());
        pages_[fid].is_dirty_ = false;
      }
      page_table_.erase(page_table_.find(pages_[fid].page_id_));
    } else {
      // latch_.unlock();
      return nullptr;
    }
  }
  auto pid = disk_manager_->AllocatePage();
  auto &frame = pages_[fid];
  frame.page_id_ = pid;
  frame.pin_count_ = 0;
  frame.is_dirty_ = false;
  page_table_[pid] = fid;
  *page_id = pid;
  replacer_->RecordAccess(fid);
  replacer_->SetEvictable(fid, false);
  ++pages_[fid].pin_count_;
  return &pages_[fid];
}

auto BufferPoolManager::FetchPage(page_id_t page_id) -> Page * {
  // latch_.lock();
  auto it = page_table_.find(page_id);
  if (it != page_table_.end()) {
    auto fid = it->second;
    auto &frame = pages_[fid];
    ++frame.pin_count_;
    replacer_->RecordAccess(fid);
    replacer_->SetEvictable(fid, false);
    // latch_.unlock();
    return &frame;
  }
  // Not found in buffer pool. Read from the disk.
  frame_id_t fid;
  if (!free_list_.empty()) {
    fid = free_list_.front();
    free_list_.pop_front();
  } else {
    if (replacer_->Evict(&fid)) {
      if (pages_[fid].IsDirty()) {
        disk_manager_->WritePage(pages_[fid].page_id_, pages_[fid].GetData());
        pages_[fid].is_dirty_ = false;
      }
      page_table_.erase(it);
    } else {
      // latch_.unlock();
      return nullptr;
    }
  }
  auto &frame = pages_[fid];

  frame.page_id_ = page_id;
  frame.pin_count_ = 1;
  frame.is_dirty_ = false;
  //  frame.ResetMemory();
  page_table_[page_id] = fid;
  disk_manager_->ReadPage(page_id, frame.GetData());
  replacer_->RecordAccess(fid);
  replacer_->SetEvictable(fid, false);
  // latch_.unlock();
  return &frame;
}

auto BufferPoolManager::UnpinPage(page_id_t page_id, bool is_dirty) -> bool {
  // latch_.lock();
  auto it = page_table_.find(page_id);
  if (it == page_table_.end() || pages_[it->second].pin_count_ == 0) {
    // latch_.unlock();
    return false;
  }
  auto fid = it->second;
  auto &frame = pages_[fid];
  --frame.pin_count_;
  if (frame.pin_count_ == 0) {
    replacer_->SetEvictable(fid, true);
  }
  if (is_dirty) {
    frame.is_dirty_ = true;
  }
  // latch_.unlock();
  return true;
}

auto BufferPoolManager::FlushPage(page_id_t page_id) -> bool {
  if (page_id == INVALID_PAGE_ID) {
    return false;
  }
  // latch_.lock();
  auto it = page_table_.find(page_id);
  if (it == page_table_.end()) {
    // latch_.unlock();
    return false;
  }
  auto fid = it->second;
  auto &frame = pages_[fid];
  disk_manager_->WritePage(page_id, frame.GetData());
  frame.is_dirty_ = false;
  // latch_.unlock();
  return true;
}

void BufferPoolManager::FlushAllPages() {
  for (auto &pr : page_table_) {
    FlushPage(pr.first);
  }
}

auto BufferPoolManager::DeletePage(page_id_t page_id) -> bool {
  if (page_id == INVALID_PAGE_ID) {
    return false;
  }
  // latch_.lock();
  auto it = page_table_.find(page_id);
  if (it == page_table_.end()) {
    // latch_.unlock();
    return true;
  }
  auto fid = it->second;
  auto &frame = pages_[fid];
  if (frame.GetPinCount() > 0) {
    // latch_.unlock();
    return false;
  }
  if (frame.IsDirty()) {
    disk_manager_->WritePage(page_id, frame.GetData());
    frame.is_dirty_ = false;
  }
  page_table_.erase(it);
  replacer_->Remove(fid);
  free_list_.push_back(fid);
  //  frame.ResetMemory();
  frame.pin_count_ = 0;
  frame.page_id_ = INVALID_PAGE_ID;
  frame.is_dirty_ = false;
  disk_manager_->DeallocatePage(page_id);
  // latch_.unlock();
  return true;
}

auto BufferPoolManager::FetchPageBasic(page_id_t page_id) -> BasicPageGuard { return {this, FetchPage(page_id)}; }

auto BufferPoolManager::FetchPageRead(page_id_t page_id) -> ReadPageGuard {
  Page *page = FetchPage(page_id);
  if (page != nullptr) {
    //    page->RLatch();
  }
  return {this, page};
}

auto BufferPoolManager::FetchPageWrite(page_id_t page_id) -> WritePageGuard {
  Page *page = FetchPage(page_id);
  if (page != nullptr) {
    //    page->WLatch();
  }
  return {this, page};
}

auto BufferPoolManager::NewPageGuarded(page_id_t *page_id) -> BasicPageGuard { return {this, NewPage(page_id)}; }

}  // namespace CrazyDave
