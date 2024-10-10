// Observable Library
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "obs/signal.h"
#include "test.h"

#include <atomic>
#include <vector>

int main() {
  std::vector<std::thread> threads;
  obs::signal<void()> sig;

  // We need and atomic operator++()
  std::atomic<int> counter(0);

  for (int i=0; i<100; ++i) {
    sig.connect([&](){ ++counter; });
  }

  for (int i=0; i<100; ++i) {
    threads.push_back(
      std::thread(
        [&](){
          for (int i=0; i<100; ++i)
            sig();
        }));
  }

  for (auto& thread : threads)
    thread.join();

  EXPECT_EQ(100*100*100, counter);
}
