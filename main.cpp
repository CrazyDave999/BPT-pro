#include <iostream>
#include "common/utils.h"
#include "data_structures/vector.h"
#include "storage/index/b_plus_tree.h"

auto main() -> int {
  // freopen("test.in","r",stdin);
  // freopen("test.out","w",stdout);
  std::ios::sync_with_stdio(false);

  CrazyDave::BPT<uint64_t, int> bpt("my_bpt", 0, 2500, 1);
  // CrazyDave::BPT<CrazyDave::String<65>, int> bpt("my_bpt", 0, 2500, 1);


  // auto hash_fn = [](CrazyDave::String<65> &str) {
  //   return std::hash<std::string_view>{}(str.c_str());
  // };

  int n;
  std::cin >> n;
  while (n--) {
    CrazyDave::String<65> op, index;
    int value;
    std::cin >> op;
    if (op[0] == 'i') {
      std::cin >> index >> value;
      auto index_hs = CrazyDave::HashBytes(index.c_str());
      bpt.insert(index_hs, value);
      // bpt.insert(index, value);
    } else if (op[0] == 'd') {
      std::cin >> index >> value;
      auto index_hs = CrazyDave::HashBytes(index.c_str());
      bpt.remove(index_hs, value);
      // bpt.remove(index, value);
    } else {
      std::cin >> index;
      auto index_hs = CrazyDave::HashBytes(index.c_str());
      CrazyDave::vector<int> res;
      bpt.find(index_hs, res);
      for (const auto x : res) {
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