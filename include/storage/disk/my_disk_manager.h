#ifndef BPT_PRO_DISK_MANAGER_H
#define BPT_PRO_DISK_MANAGER_H

#include <atomic>
#include <fstream>
#include <mutex>
#include <string>
#include "common/config.h"
#include "concurrent_queue.h"
#include "file_wrapper.h"
namespace CrazyDave {
const int QUEUE_CAPACITY = 8000;
class MyDiskManager {
 public:
  explicit MyDiskManager() {
    if (!garbage_file.IsNew()) {
      garbage_file.SetReadPointer(0);
      size_t size;
      garbage_file.ReadObj(size);
      page_id_t max_page_id;
      garbage_file.ReadObj(max_page_id);
      max_page_id_ = max_page_id;
      for (size_t i = 0; i < size; ++i) {
        page_id_t page_id;
        garbage_file.ReadObj(page_id);
        queue_.push(page_id);
      }
    }
  }
  ~MyDiskManager() {
    garbage_file.SetWritePointer(0);
    size_t size = (queue_.get_tail() - queue_.get_head() + QUEUE_CAPACITY) % QUEUE_CAPACITY;
    garbage_file.WriteObj(size);
    page_id_t max_page_id = max_page_id_.load();
    garbage_file.WriteObj(max_page_id);
    for (size_t i = 0; i < size; ++i) {
      page_id_t page_id;
      queue_.pop(page_id);
      garbage_file.WriteObj(page_id);
    }
  }
  void WritePage(page_id_t page_id, const char *page_data) {
    //    std::scoped_lock scoped_io_latch(io_latch_);
    int offset = page_id * BUSTUB_PAGE_SIZE;
    data_file_.SetWritePointer(offset);
    data_file_.Write(page_data, BUSTUB_PAGE_SIZE);
  }
  void ReadPage(page_id_t page_id, char *page_data) {
    //    std::scoped_lock scoped_db_io_latch(io_latch_);
    int offset = page_id * BUSTUB_PAGE_SIZE;
    data_file_.SetReadPointer(offset);
    data_file_.Read(page_data, BUSTUB_PAGE_SIZE);
  }
  auto AllocatePage() -> page_id_t {
    page_id_t new_page_id;
    if (queue_.pop(new_page_id)) {
      return new_page_id;
    }
    return ++max_page_id_;
  }

  void DeallocatePage(page_id_t page_id) { queue_.push(page_id); }
  auto IsNew() -> bool { return garbage_file.IsNew(); }

 private:
  MyFile data_file_{"data"};
  MyFile garbage_file{"garbage"};  // 第一位size_，第二位max_page_id_
  ConcurrentQueue<page_id_t, QUEUE_CAPACITY> queue_;
  std::atomic<page_id_t> max_page_id_{0};
  std::mutex io_latch_;
};
}  // namespace CrazyDave
#endif  // BPT_PRO_DISK_MANAGER_H
