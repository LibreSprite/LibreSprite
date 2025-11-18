// Clip Library
// Copyright (C) 2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "test.h"

#include "clip.h"

#include <cstring>

using namespace clip;

int main(int argc, char** argv)
{
  clear();
  EXPECT_FALSE(has(image_format()));

  uint32_t data[] = {
    0xffff0000, 0xff00ff00, 0xff0000ff,
    0x7fff0000, 0x7f00ff00, 0x7f0000ff,
  };
  {
    image_spec spec;
    spec.width = 3;
    spec.height = 2;
    spec.bits_per_pixel = 32;
    spec.bytes_per_row = spec.width*4;
    spec.red_mask = 0xff;
    spec.green_mask = 0xff00;
    spec.blue_mask = 0xff0000;
    spec.alpha_mask = 0xff000000;
    spec.red_shift = 0;
    spec.green_shift = 8;
    spec.blue_shift = 16;
    spec.alpha_shift = 24;
    image img(data, spec);
    EXPECT_TRUE(set_image(img));
  }
  EXPECT_TRUE(has(image_format()));

  {
    image_spec spec;
    EXPECT_TRUE(get_image_spec(spec));
    EXPECT_EQ(3, spec.width);
    EXPECT_EQ(2, spec.height);
    EXPECT_EQ(32, spec.bits_per_pixel);
    EXPECT_EQ(spec.width*4, spec.bytes_per_row);
  }

  {
    image img;
    EXPECT_TRUE(get_image(img));
    EXPECT_EQ(3, img.spec().width);
    EXPECT_EQ(2, img.spec().height);

    image_spec spec = img.spec();
    auto data2 = (const uint32_t*)img.data();
    for (int i=0; i<6; ++i) {
      uint32_t c = data2[i];
      c =
        (( c & spec.red_mask  ) >> spec.red_shift  )        |
        (((c & spec.green_mask) >> spec.green_shift) << 8)  |
        (((c & spec.blue_mask ) >> spec.blue_shift ) << 16) |
        (((c & spec.alpha_mask) >> spec.alpha_shift) << 24);
      EXPECT_EQ(data[i], c);
    }
  }

  clear();
  EXPECT_FALSE(has(image_format()));
}
