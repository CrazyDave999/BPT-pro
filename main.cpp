#include <iostream>
#include "buffer/buffer_pool_manager.h"
#include "common/utils.h"
#include "storage/disk/my_disk_manager.h"
#include "storage/index/b_plus_tree.h"
#include "data_structures/vector.h"

auto main() -> int {
  auto *disk_manager = new CrazyDave::MyDiskManager;
  auto *bpm = new CrazyDave::BufferPoolManager(5000, disk_manager, 30);
  CrazyDave::BPlusTree<CrazyDave::pair<size_t ,int>, int, CrazyDave::Comparator<size_t,int,int>> bpt("my_bpt", 0, bpm);
  std::ios::sync_with_stdio(false);
  int n;
  std::cin >> n;
  while (n--) {
    CrazyDave::String<65> op, index;
    int value;
    std::cin >> op;
    if (op[0] == 'i') {
      std::cin >> index >> value;
      auto index_hs=CrazyDave::HashBytes(index.c_str());
      bpt.Insert({index_hs, value}, 0);
    } else if (op[0] == 'd') {
      std::cin >> index >> value;
      auto index_hs=CrazyDave::HashBytes(index.c_str());
      bpt.Remove({index_hs, value});
    } else {
      std::cin >> index;
      auto index_hs=CrazyDave::HashBytes(index.c_str());
      CrazyDave::vector<CrazyDave::pair<size_t ,int>> res;
      bpt.Find({index_hs, 0}, &res);
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