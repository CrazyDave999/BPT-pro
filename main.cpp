#include <iostream>
#include "buffer/buffer_pool_manager.h"
#include "common/utils.h"
#include "storage/disk/my_disk_manager.h"
#include "storage/index/b_plus_tree.h"
#include "data_structures/vector.h"

auto main() -> int {
  auto *disk_manager = new CrazyDave::MyDiskManager;
  auto *bpm = new CrazyDave::BufferPoolManager(4000, disk_manager, 30);
  CrazyDave::Comparator comparator;
  CrazyDave::BPlusTree<CrazyDave::key_t, CrazyDave::page_id_t, CrazyDave::Comparator> bpt("my_bpt", 0, bpm, comparator);
  std::ios::sync_with_stdio(false);
  int n;
  std::cin >> n;
  while (n--) {
    CrazyDave::String<65> op, index;
    int value;
    std::cin >> op;
    if (op[0] == 'i') {
      std::cin >> index >> value;
      bpt.Insert({index, value}, 0);
    } else if (op[0] == 'd') {
      std::cin >> index >> value;
      bpt.Remove({index, value});
    } else {
      std::cin >> index;
      CrazyDave::vector<CrazyDave::key_t> res;
      bpt.Find({index, 0}, &res);
      for (auto x : res) {
        std::cout << x.second << ' ';
      }
      if (res.empty()) {
        std::cout << "null";
      }
      std::cout << std::endl;
    }
  }
  delete bpm;
  delete disk_manager;
  return 0;
}