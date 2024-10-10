// Observable Library
// Copyright (c) 2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "obs/signal.h"
#include "test.h"

#include <algorithm>
#include <ctime>
#include <future>
#include <thread>
#include <vector>

int main() {
  obs::signal<void()> sig;
  std::time_t t = std::time(nullptr);

  auto func =
    [&sig, t](){
      int count = 0;
      while ((std::time(nullptr) - t) < 5) {
        std::vector<obs::scoped_connection> conns(32);
        for (auto& conn : conns) {
          conn = sig.connect([](){ });
          ++count;
          if (std::time(nullptr) - t >= 5)
            break;
        }
      }
      return count;
    };

  std::vector<std::future<int>> futures;
  int n = std::max<int>(1, std::thread::hardware_concurrency()/2);
  while (n--)
    futures.emplace_back(std::async(std::launch::async, func));

  int count = 0;
  for (auto& f : futures)
    count += f.get();

  std::cout << "Count = " << count << "\n";
  return 0;
}
