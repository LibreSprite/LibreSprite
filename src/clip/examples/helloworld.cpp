// Clip Library
// Copyright (c) 2015 David Capello

#include "clip.h"
#include <cassert>
#include <iostream>

int main() {
  clip::set_text("Hello World");

  std::string value;
  bool result = clip::get_text(value);

  assert(result);
  assert(clip::has(clip::text_format()));
  assert(value == "Hello World");

  std::cout << "'" << value << "' was copied to the clipboard\n";
}
