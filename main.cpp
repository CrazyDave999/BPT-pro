#include <iostream>
#include "buffer/buffer_pool_manager.h"
#include "common/utils.h"
#include "data_structures/vector.h"
#include "storage/disk/my_disk_manager.h"
#include "storage/index/b_plus_tree.h"

auto main() -> int {
  CrazyDave::BPlusTree<CrazyDave::pair<uint64_t, int>, int, CrazyDave::Comparator<uint64_t, int, int>> bpt("my_bpt", 0,
                                                                                                           2000, 30);
  //  CrazyDave::BPlusTree<CrazyDave::pair<CrazyDave::String<65>, int>, int,
  //                       CrazyDave::Comparator<CrazyDave::String<65>, int, int>>
  //      bpt("my_bpt", 0, bpm);

  //  auto y = std::freopen("../Bpt_data/3.in", "r", stdin);
  //  y = std::freopen("../output.txt", "w", stdout);
  std::ios::sync_with_stdio(false);
  int n;
  std::cin >> n;
  while (n--) {
    CrazyDave::String<65> op, index;
    int value;
    std::cin >> op;
    if (op[0] == 'i') {
      std::cin >> index >> value;
      auto index_hs = CrazyDave::HashBytes(index.c_str());
      bpt.Insert({index_hs, value}, 0);
      //      bpt.Insert({index, value}, 0);
    } else if (op[0] == 'd') {
      std::cin >> index >> value;
      auto index_hs = CrazyDave::HashBytes(index.c_str());
      bpt.Remove({index_hs, value});
      //      bpt.Remove({index, value});
    } else {
      std::cin >> index;
      auto index_hs = CrazyDave::HashBytes(index.c_str());
      CrazyDave::vector<CrazyDave::pair<uint64_t, int>> res;
      bpt.Find({index_hs, 0}, &res);
      //      CrazyDave::vector<CrazyDave::pair<CrazyDave::String<65>, int>> res;
      //      bpt.Find({index, 0}, &res);
      for (auto x : res) {
        std::cout << x.second << ' ';
      }
      if (res.empty()) {
        std::cout << "null";
      }
      std::cout << std::endl;
    }
  }
  return 0;
}