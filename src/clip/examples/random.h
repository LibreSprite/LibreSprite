#include <random>
#pragma once

class RandomInt {
public:
  RandomInt(int a, int b)
    : m_mt(m_device())
    , m_dist(a, b) {
  }

  int generate() {
    return m_dist(m_mt);
  }

private:
  std::random_device m_device;
  std::mt19937 m_mt;
  std::uniform_int_distribution<int> m_dist;
};
