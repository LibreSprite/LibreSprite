// Clip Library
// Copyright (c) 2015 David Capello

#include "clip.h"
#include <iostream>

int main() {
  if (clip::has(clip::text_format())) {
    std::string value;
    clip::get_text(value);

    std::cout << "Clipboard content is '" << value << "'\n";
  }
  else {
    std::cout << "Clipboard doesn't contain text\n";
  }
}
