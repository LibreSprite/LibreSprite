// Observable Library
// Copyright (c) 2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "obs.h"
#include <benchmark/benchmark.h>

#include <atomic>
#include <condition_variable>
#include <future>
#include <thread>
#include <vector>

static void BM_ObsCreation(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    {
      obs::signal<void()> sig;
      state.ResumeTiming();
    }
  }
}
BENCHMARK(BM_ObsCreation);

static void BM_ObsConnect(benchmark::State& state) {
  obs::signal<void()> sig;
  for (auto _ : state)
    sig.connect([]{ });
}
BENCHMARK(BM_ObsConnect);

static void BM_ObsDisconnect(benchmark::State& state) {
  obs::signal<void()> sig;
  for (auto _ : state) {
    state.PauseTiming();
    obs::connection c = sig.connect([]{ });
    state.ResumeTiming();
    c.disconnect();
  }
}
BENCHMARK(BM_ObsDisconnect);

static void BM_ObsSignal(benchmark::State& state) {
  obs::signal<void()> sig;
  std::vector<obs::scoped_connection> conns(state.range(0));
  for (auto& c : conns)
    c = sig.connect([]{ });
  for (auto _ : state) {
    sig();
  }
}
BENCHMARK(BM_ObsSignal)->Range(1, 1024);

static void BM_ObsThreads(benchmark::State& state) {
  obs::signal<void()> sig;
  for (auto _ : state) {
    state.PauseTiming();
    std::vector<std::thread> threads;
    std::atomic<int> count = { 0 };
    for (int i=0; i<state.range(0); ++i) {
      threads.emplace_back(
        [&sig, &count]{
          std::mutex m;
          std::unique_lock<std::mutex> l(m);
          std::condition_variable cv;
          obs::scoped_connection c =
            sig.connect(
              [&m, &cv]{
                std::unique_lock<std::mutex> l(m);
                cv.notify_one();
              });
          ++count;
          cv.wait(l);
        });
    }

    // Wait that all threads are created and waiting for the signal.
    while (count < state.range(0))
      std::this_thread::yield();
    state.ResumeTiming();

    sig();

    state.PauseTiming();
    for (auto& t : threads)
      t.join();
    state.ResumeTiming();
  }
}
BENCHMARK(BM_ObsThreads)->Range(1, 1024);

BENCHMARK_MAIN();
