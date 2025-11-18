// Clip Library
// Copyright (c) 2015-2016 David Capello

#include "clip.h"
#include <iostream>
#include <iomanip>

template<typename T>
static void print_channel(const clip::image& img,
                          const clip::image_spec& spec,
                          const std::string& channel_name,
                          const int channel_mask,
                          const int channel_shift,
                          const int channel_fill_width,
                          const int pixel_size) {
  std::cout << channel_name << ":\n";
  for (unsigned long y=0; y<spec.height; ++y) {
    const char* p = (img.data()+spec.bytes_per_row*y);
    std::cout << "  ";
    for (unsigned long x=0; x<spec.width; ++x, p += pixel_size) {
      std::cout << std::right
                << std::hex
                << std::setw(channel_fill_width)
                << (((*((T*)p)) & channel_mask) >> channel_shift) << " ";
    }
    std::cout << "\n";
  }
}

template<typename T>
static void print_channels(const clip::image& img,
                           const clip::image_spec& spec,
                           const int channel_fill_width,
                           int pixel_size = sizeof(T)) {
  print_channel<T>(img, spec, "Red",   spec.red_mask,   spec.red_shift,   channel_fill_width, pixel_size);
  print_channel<T>(img, spec, "Green", spec.green_mask, spec.green_shift, channel_fill_width, pixel_size);
  print_channel<T>(img, spec, "Blue",  spec.blue_mask,  spec.blue_shift,  channel_fill_width, pixel_size);
  if (spec.alpha_mask)
    print_channel<T>(img, spec, "Alpha", spec.alpha_mask, spec.alpha_shift, channel_fill_width, pixel_size);
}

int main() {
  if (!clip::has(clip::image_format())) {
    std::cout << "Clipboard doesn't contain an image\n";
    return 1;
  }

  clip::image img;
  if (!clip::get_image(img)) {
    std::cout << "Error getting image from clipboard\n";
    return 1;
  }

  clip::image_spec spec = img.spec();

  std::cout << "Image in clipboard "
            << spec.width << "x" << spec.height
            << " (" << spec.bits_per_pixel << "bpp)\n"
            << "Format:" << "\n"
            << std::hex
            << "  Red   mask: " << spec.red_mask << "\n"
            << "  Green mask: " << spec.green_mask << "\n"
            << "  Blue  mask: " << spec.blue_mask << "\n"
            << "  Alpha mask: " << spec.alpha_mask << "\n"
            << std::dec
            << "  Red   shift: " << spec.red_shift << "\n"
            << "  Green shift: " << spec.green_shift << "\n"
            << "  Blue  shift: " << spec.blue_shift << "\n"
            << "  Alpha shift: " << spec.alpha_shift << "\n";

  std::cout << "Bytes:\n";
  for (unsigned long y=0; y<spec.height; ++y) {
    char* p = img.data()+spec.bytes_per_row*y;
    std::cout << "  ";
    for (unsigned long x=0; x<spec.width; ++x) {
      for (unsigned long byte=0; byte<spec.bits_per_pixel/8; ++byte, ++p)
        std::cout << std::right << std::hex << std::setfill('0') << std::setw(2) << int((*p) & 0xff) << " ";
    }
    std::cout << "\n";
  }

  switch (spec.bits_per_pixel) {
    case 16: print_channels<uint16_t>(img, spec, 2);    break;
    case 24: print_channels<uint32_t>(img, spec, 2, 3); break;
    case 32: print_channels<uint32_t>(img, spec, 2);    break;
    case 64: print_channels<uint64_t>(img, spec, 4);    break;
  }
}
