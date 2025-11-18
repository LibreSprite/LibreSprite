// Clip Library
// Copyright (C) 2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "test.h"

#include "clip.h"

#include <cstring>
#include <string>
#include <vector>

using namespace clip;

int main(int argc, char** argv)
{
  format intF = register_format("com.github.clip.int");
  format doubleF = register_format("com.github.clip.double");
  EXPECT_TRUE(intF != empty_format());
  EXPECT_TRUE(doubleF != empty_format());

  // Clear clipboard content
  {
    clear();
    EXPECT_FALSE(has(text_format()));
    EXPECT_FALSE(has(intF));
    EXPECT_FALSE(has(doubleF));
  }

  // Set int and double formats
  {
    lock l;
    int intV = 32;
    double doubleV = 32.48;
    EXPECT_TRUE(l.set_data(intF, (const char*)&intV, sizeof(int)));
    EXPECT_TRUE(l.set_data(doubleF, (const char*)&doubleV, sizeof(double)));
  }
  EXPECT_FALSE(has(text_format()));
  EXPECT_TRUE(has(intF));
  EXPECT_TRUE(has(doubleF));

  // Get int and double formats
  {
    lock l;
    int intV = 0;
    double doubleV = 0.0;
    EXPECT_TRUE(l.get_data(intF, (char*)&intV, sizeof(int)));
    EXPECT_TRUE(l.get_data(doubleF, (char*)&doubleV, sizeof(double)));
    EXPECT_EQ(32, intV);
    EXPECT_EQ(32.48, doubleV);
  }

  // Add text to clipboard
  {
    lock l;
    EXPECT_TRUE(l.set_data(text_format(), (const char*)"thirty-two", 10));
  }
  EXPECT_TRUE(has(text_format()));
  EXPECT_TRUE(has(intF));
  EXPECT_TRUE(has(doubleF));

  // Get all formats
  {
    lock l;
    int intV = 0;
    double doubleV = 0.0;
    char buf[11];
    EXPECT_EQ(11, l.get_data_length(text_format()));
    EXPECT_TRUE(l.get_data(text_format(), (char*)buf, 11));
    EXPECT_TRUE(l.get_data(intF, (char*)&intV, sizeof(int)));
    EXPECT_TRUE(l.get_data(doubleF, (char*)&doubleV, sizeof(double)));
    EXPECT_EQ("thirty-two", std::string(buf));
    EXPECT_EQ(32, intV);
    EXPECT_EQ(32.48, doubleV);
  }
}
