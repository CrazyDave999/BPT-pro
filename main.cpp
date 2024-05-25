#include <iostream>
#include "common/utils.h"
#include "data_structures/vector.h"
#include "storage/index/b_plus_tree.h"

auto main() -> int {
  std::ios::sync_with_stdio(false);
  //  CrazyDave::BPlusTree<CrazyDave::pair<uint64_t, int>, int, CrazyDave::Comparator<uint64_t, int, int>> bpt("my_bpt",
  //  0,
  //                                                                                                           3000,
  //                                                                                                           30);
  CrazyDave::BPT<CrazyDave::String<65>, int> bpt("my_bpt", 0, 300, 30);
  //  CrazyDave::BPlusTree<CrazyDave::pair<CrazyDave::String<65>, int>, int,
  //                       CrazyDave::Comparator<CrazyDave::String<65>, int, int>>
  //      bpt("my_bpt", 0, 300, 30);

  //  auto y = std::freopen("../Bpt_data/28-29", "r", stdin);
  //  y = std::freopen("../output.txt", "w", stdout);

  int n;
  std::cin >> n;
  while (n--) {
    CrazyDave::String<65> op, index;
    int value;
    std::cin >> op;
    if (op[0] == 'i') {
      std::cin >> index >> value;
      //      auto index_hs = CrazyDave::HashBytes(index.c_str());
      //      bpt.Insert({index_hs, value}, 0);
      bpt.insert(index,value);
    } else if (op[0] == 'd') {
      std::cin >> index >> value;
      //      auto index_hs = CrazyDave::HashBytes(index.c_str());
      //      bpt.Remove({index_hs, value});
      bpt.remove(index,value);
    } else {
      std::cin >> index;
      //      auto index_hs = CrazyDave::HashBytes(index.c_str());
      //      CrazyDave::vector<CrazyDave::pair<uint64_t, int>> res;
      //      bpt.Find({index_hs, 0}, &res);
      CrazyDave::vector<int> res;
      bpt.find(index, res);
      for (auto x : res) {
        std::cout << x << ' ';
      }
      if (res.empty()) {
        std::cout << "null";
      }
      std::cout << std::endl;
    }
  }
  return 0;
}