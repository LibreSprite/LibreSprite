// Clip Library
// Copyright (C) 2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "test.h"

#include "clip.h"

#include <string>
#include <vector>

using namespace clip;

int main(int argc, char** argv)
{
  // High API
  {
    std::string value;
    set_text("hello");
    EXPECT_TRUE(get_text(value));
    EXPECT_EQ("hello", value);
    EXPECT_TRUE(has(text_format()));

    set_text("world");
    EXPECT_TRUE(get_text(value));
    EXPECT_EQ("world", value);

    clear();
    EXPECT_FALSE(has(text_format()));
  }

  // Lock API
  {
    lock l;
    EXPECT_TRUE(l.locked());
    l.set_data(text_format(), "hello world", 11);

    // get_data_length() must return the extra zero character at the end
    EXPECT_EQ(12, l.get_data_length(text_format()));

    // Get the whole data
    std::vector<char> buf(12);
    EXPECT_TRUE(l.get_data(text_format(), &buf[0], buf.size()));
    EXPECT_EQ("hello world", std::string(&buf[0]));
  }
}
