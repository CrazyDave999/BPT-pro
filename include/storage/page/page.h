#pragma once

#include <cstring>
#include <iostream>

#include "common/config.h"
#include "common/rwlatch.h"

namespace CrazyDave {

/**
 * Page is the basic unit of storage within the database system. Page provides a wrapper for actual data pages being
 * held in main memory. Page also contains book-keeping information that is used by the buffer pool manager, e.g.
 * pin count, dirty flag, page id, etc.
 */
class Page {
  // There is book-keeping information inside the page that should only be relevant to the buffer pool manager.
  friend class BufferPoolManager;

 public:
  /** Constructor. Zeros out the page data. */
  Page() {
    //    data_ = new char[BUSTUB_PAGE_SIZE]{};
    //    ResetMemory();
  }

  /** Default destructor. */
  ~Page() {
    //    delete[] data_;
  }

  /** @return the actual data contained within this page */
  inline auto GetData() -> char * { return data_; }

  /** @return the page id of this page */
  inline auto GetPageId() const -> page_id_t { return page_id_; }

  /** @return the pin count of this page */
  inline auto GetPinCount() const -> int { return pin_count_; }

  /** @return true if the page in memory has been modified from the page on disk, false otherwise */
  inline auto IsDirty() const -> bool { return is_dirty_; }

  /** Acquire the page write latch. */
  inline void WLatch() { rwlatch_.WLock(); }

  /** Release the page write latch. */
  inline void WUnlatch() { rwlatch_.WUnlock(); }

  /** Acquire the page read latch. */
  inline void RLatch() { rwlatch_.RLock(); }

  /** Release the page read latch. */
  inline void RUnlatch() { rwlatch_.RUnlock(); }

 protected:
  static_assert(sizeof(page_id_t) == 4);

  static constexpr size_t OFFSET_PAGE_START = 0;

 private:
  /** Zeroes out the data that is held within the page. */
  inline void ResetMemory() { memset(data_, OFFSET_PAGE_START, BUSTUB_PAGE_SIZE); }

  /** The actual data that is stored within a page. */
  char data_[BUSTUB_PAGE_SIZE]{};
  /** The ID of this page. */
  page_id_t page_id_ = INVALID_PAGE_ID;
  /** The pin count of this page. */
  int pin_count_ = 0;
  /** True if the page is dirty, i.e. it is different from its corresponding page on disk. */
  bool is_dirty_ = false;
  /** Page latch. */
  ReaderWriterLatch rwlatch_;
};

}  // namespace CrazyDave
