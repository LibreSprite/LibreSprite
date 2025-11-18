// Clip Library
// Copyright (c) 2016 David Capello

#include "clip.h"
#include <iostream>

int main(int argc, char* argv[]) {
  std::string new_content;
  for (int i=1; i<argc; ++i) {
    if (!new_content.empty())
      new_content += " ";
    new_content += argv[i];
  }
  clip::set_text(new_content);

  std::string clipboard;
  clip::get_text(clipboard);
  std::cout << "'" << clipboard << "' copied\n";
}
