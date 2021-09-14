// SHE library
// Copyright (C) 2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "base/path.h"

inline void generate_drop_files_from_nsarray(NSArray* filenames)
{
  std::vector<std::string> files;
  for (int i=0; i<[filenames count]; ++i) {
    NSString* fn = [filenames objectAtIndex: i];
    files.push_back(base::normalize_path([fn UTF8String]));
  }

  she::Event ev;
  ev.setType(she::Event::DropFiles);
  ev.setFiles(files);
  she::queue_event(ev);
}
