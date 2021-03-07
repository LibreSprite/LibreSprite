// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/document.h"
#include "app/file/file.h"
#include "app/file/file_format.h"
#include "app/file/format_options.h"
#include "app/ini_file.h"
#include "base/file_handle.h"
#include "doc/doc.h"

#include <stdio.h>
#include <stdlib.h>

#include "png.h"

namespace app {

using namespace base;

class PngFormat : public FileFormat {
  const char* onGetName() const override { return "png"; }
  const char* onGetExtensions() const override { return "png"; }
  int onGetFlags() const override {
    return
      FILE_SUPPORT_LOAD |
      FILE_SUPPORT_SAVE |
      FILE_SUPPORT_RGB |
      FILE_SUPPORT_RGBA |
      FILE_SUPPORT_GRAY |
      FILE_SUPPORT_GRAYA |
      FILE_SUPPORT_INDEXED |
      FILE_SUPPORT_SEQUENCES |
      FILE_SUPPORT_PALETTE_WITH_ALPHA;
  }

  bool onLoad(FileOp* fop) override;
  bool onSave(FileOp* fop) override;
};

FileFormat* CreatePngFormat()
{
  return new PngFormat;
}

static void report_png_error(png_structp png_ptr, png_const_charp error)
{
  ((FileOp*)png_get_error_ptr(png_ptr))->setError("libpng: %s\n", error);
}

bool PngFormat::onLoad(FileOp* fop)
{
  png_uint_32 width, height, y;
  unsigned int sig_read = 0;
  png_structp png_ptr;
  png_infop info_ptr;
  int bit_depth, color_type, interlace_type;
  int pass, number_passes;
  int num_palette;
  png_colorp palette;
  png_bytepp rows_pointer;
  PixelFormat pixelFormat;

  FileHandle handle(open_file_with_exception(fop->filename(), "rb"));
  FILE* fp = handle.get();

  /* Create and initialize the png_struct with the desired error handler
   * functions.  If you want to use the default stderr and longjump method,
   * you can supply NULL for the last three parameters.  We also supply the
   * the compiler header file version, so that we know if the application
   * was compiled with a compatible version of the library
   */
  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, (png_voidp)fop,
                                   report_png_error, report_png_error);
  if (png_ptr == NULL) {
    fop->setError("png_create_read_struct\n");
    return false;
  }

  /* Allocate/initialize the memory for image information. */
  info_ptr = png_create_info_struct(png_ptr);
  if (info_ptr == NULL) {
    fop->setError("png_create_info_struct\n");
    png_destroy_read_struct(&png_ptr, NULL, NULL);
    return false;
  }

  /* Set error handling if you are using the setjmp/longjmp method (this is
   * the normal method of doing things with libpng).
   */
  if (setjmp(png_jmpbuf(png_ptr))) {
    fop->setError("Error reading PNG file\n");
    /* Free all of the memory associated with the png_ptr and info_ptr */
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    /* If we get here, we had a problem reading the file */
    return false;
  }

  /* Set up the input control if you are using standard C streams */
  png_init_io(png_ptr, fp);

  /* If we have already read some of the signature */
  png_set_sig_bytes(png_ptr, sig_read);

  /* The call to png_read_info() gives us all of the information from the
   * PNG file before the first IDAT (image data chunk).
   */
  png_read_info(png_ptr, info_ptr);

  png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
               &interlace_type, NULL, NULL);


  /* Set up the data transformations you want.  Note that these are all
   * optional.  Only call them if you want/need them.  Many of the
   * transformations only work on specific types of images, and many
   * are mutually exclusive.
   */

  /* tell libpng to strip 16 bit/color files down to 8 bits/color */
  png_set_strip_16(png_ptr);

  /* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
   * byte into separate bytes (useful for paletted and grayscale images).
   */
  png_set_packing(png_ptr);

  /* Expand grayscale images to the full 8 bits from 1, 2, or 4 bits/pixel */
  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_expand_gray_1_2_4_to_8(png_ptr);

  /* Turn on interlace handling.  REQUIRED if you are not using
   * png_read_image().  To see how to handle interlacing passes,
   * see the png_read_row() method below:
   */
  number_passes = png_set_interlace_handling(png_ptr);

  /* Optional call to gamma correct and add the background to the palette
   * and update info structure.
   */
  png_read_update_info(png_ptr, info_ptr);

  /* create the output image */
  switch (png_get_color_type(png_ptr, info_ptr)) {

    case PNG_COLOR_TYPE_RGB_ALPHA:
      fop->sequenceSetHasAlpha(true);
    case PNG_COLOR_TYPE_RGB:
      pixelFormat = IMAGE_RGB;
      break;

    case PNG_COLOR_TYPE_GRAY_ALPHA:
      fop->sequenceSetHasAlpha(true);
    case PNG_COLOR_TYPE_GRAY:
      pixelFormat = IMAGE_GRAYSCALE;
      break;

    case PNG_COLOR_TYPE_PALETTE:
      pixelFormat = IMAGE_INDEXED;
      break;

    default:
      fop->setError("Color type not supported\n)");
      png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
      return false;
  }

  int imageWidth = png_get_image_width(png_ptr, info_ptr);
  int imageHeight = png_get_image_height(png_ptr, info_ptr);
  Image* image = fop->sequenceImage(pixelFormat, imageWidth, imageHeight);
  if (!image) {
    fop->setError("file_sequence_image %dx%d\n", imageWidth, imageHeight);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    return false;
  }

  // Transparent color
  png_color_16p png_trans_color = NULL;

  // Read the palette
  if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_PALETTE &&
      png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette)) {
    fop->sequenceSetNColors(num_palette);

    for (int c=0; c<num_palette; ++c) {
      fop->sequenceSetColor(c,
                            palette[c].red,
                            palette[c].green,
                            palette[c].blue);
    }

    // Read alpha values for palette entries
    png_bytep trans = NULL;     // Transparent palette entries
    int num_trans = 0;
    int mask_entry = -1;

    png_get_tRNS(png_ptr, info_ptr, &trans, &num_trans, nullptr);

    for (int i = 0; i < num_trans; ++i) {
      fop->sequenceSetAlpha(i, trans[i]);

      if (trans[i] < 255) {
        fop->sequenceSetHasAlpha(true); // Is a transparent sprite
        if (trans[i] == 0) {
          if (mask_entry < 0)
            mask_entry = i;
        }
      }
    }

    if (mask_entry >= 0)
      fop->document()->sprite()->setTransparentColor(mask_entry);
  }
  else {
    png_get_tRNS(png_ptr, info_ptr, nullptr, nullptr, &png_trans_color);
  }

  // Allocate the memory to hold the image using the fields of info_ptr.
  rows_pointer = (png_bytepp)png_malloc(png_ptr, sizeof(png_bytep) * height);
  for (y = 0; y < height; y++)
    rows_pointer[y] = (png_bytep)png_malloc(png_ptr, png_get_rowbytes(png_ptr, info_ptr));

  for (pass = 0; pass < number_passes; pass++) {
    for (y = 0; y < height; y++) {
      png_read_rows(png_ptr, rows_pointer+y, nullptr, 1);

      fop->setProgress(
        (double)((double)pass + (double)(y+1) / (double)(height))
        / (double)number_passes);

      if (fop->isStop())
        break;
    }
  }

  // Convert rows_pointer into the doc::Image
  for (y = 0; y < height; y++) {
    // RGB_ALPHA
    if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_RGB_ALPHA) {
      uint8_t* src_address = rows_pointer[y];
      uint32_t* dst_address = (uint32_t*)image->getPixelAddress(0, y);
      unsigned int x, r, g, b, a;

      for (x=0; x<width; x++) {
        r = *(src_address++);
        g = *(src_address++);
        b = *(src_address++);
        a = *(src_address++);
        *(dst_address++) = rgba(r, g, b, a);
      }
    }
    // RGB
    else if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_RGB) {
      uint8_t* src_address = rows_pointer[y];
      uint32_t* dst_address = (uint32_t*)image->getPixelAddress(0, y);
      unsigned int x, r, g, b, a;

      for (x=0; x<width; x++) {
        r = *(src_address++);
        g = *(src_address++);
        b = *(src_address++);

        // Transparent color
        if (png_trans_color &&
            r == png_trans_color->red &&
            g == png_trans_color->green &&
            b == png_trans_color->blue) {
          a = 0;
          if (!fop->sequenceGetHasAlpha())
            fop->sequenceSetHasAlpha(true);
        }
        else
          a = 255;

        *(dst_address++) = rgba(r, g, b, a);
      }
    }
    // GRAY_ALPHA
    else if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_GRAY_ALPHA) {
      uint8_t* src_address = rows_pointer[y];
      uint16_t* dst_address = (uint16_t*)image->getPixelAddress(0, y);
      unsigned int x, k, a;

      for (x=0; x<width; x++) {
        k = *(src_address++);
        a = *(src_address++);
        *(dst_address++) = graya(k, a);
      }
    }
    // GRAY
    else if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_GRAY) {
      uint8_t* src_address = rows_pointer[y];
      uint16_t* dst_address = (uint16_t*)image->getPixelAddress(0, y);
      unsigned int x, k, a;

      for (x=0; x<width; x++) {
        k = *(src_address++);

        // Transparent color
        if (png_trans_color &&
            k == png_trans_color->gray) {
          a = 0;
          if (!fop->sequenceGetHasAlpha())
            fop->sequenceSetHasAlpha(true);
        }
        else
          a = 255;

        *(dst_address++) = graya(k, a);
      }
    }
    // PALETTE
    else if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_PALETTE) {
      uint8_t* src_address = rows_pointer[y];
      uint8_t* dst_address = (uint8_t*)image->getPixelAddress(0, y);
      unsigned int x;

      for (x=0; x<width; x++)
        *(dst_address++) = *(src_address++);
    }
    png_free(png_ptr, rows_pointer[y]);
  }
  png_free(png_ptr, rows_pointer);

  // Clean up after the read, and free any memory allocated
  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
  return true;
}

bool PngFormat::onSave(FileOp* fop)
{
  const Image* image = fop->sequenceImage();
  png_uint_32 width, height, y;
  png_structp png_ptr;
  png_infop info_ptr;
  png_colorp palette = NULL;
  png_bytep row_pointer;
  int color_type = 0;
  int pass, number_passes;

  /* open the file */
  FileHandle handle(open_file_with_exception(fop->filename(), "wb"));
  FILE* fp = handle.get();

  /* Create and initialize the png_struct with the desired error handler
   * functions.  If you want to use the default stderr and longjump method,
   * you can supply NULL for the last three parameters.  We also check that
   * the library version is compatible with the one used at compile time,
   * in case we are using dynamically linked libraries.  REQUIRED.
   */
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, (png_voidp)fop,
                                    report_png_error, report_png_error);
  if (png_ptr == NULL) {
    return false;
  }

  /* Allocate/initialize the image information data.  REQUIRED */
  info_ptr = png_create_info_struct(png_ptr);
  if (info_ptr == NULL) {
    png_destroy_write_struct(&png_ptr, NULL);
    return false;
  }

  /* Set error handling.  REQUIRED if you aren't supplying your own
   * error handling functions in the png_create_write_struct() call.
   */
  if (setjmp(png_jmpbuf(png_ptr))) {
    /* If we get here, we had a problem reading the file */
    png_destroy_write_struct(&png_ptr, &info_ptr);
    return false;
  }

  /* set up the output control if you are using standard C streams */
  png_init_io(png_ptr, fp);

  /* Set the image information here.  Width and height are up to 2^31,
   * bit_depth is one of 1, 2, 4, 8, or 16, but valid values also depend on
   * the color_type selected. color_type is one of PNG_COLOR_TYPE_GRAY,
   * PNG_COLOR_TYPE_GRAY_ALPHA, PNG_COLOR_TYPE_PALETTE, PNG_COLOR_TYPE_RGB,
   * or PNG_COLOR_TYPE_RGB_ALPHA.  interlace is either PNG_INTERLACE_NONE or
   * PNG_INTERLACE_ADAM7, and the compression_type and filter_type MUST
   * currently be PNG_COMPRESSION_TYPE_BASE and PNG_FILTER_TYPE_BASE. REQUIRED
   */
  width = image->width();
  height = image->height();

  switch (image->pixelFormat()) {
    case IMAGE_RGB:
      color_type = fop->document()->sprite()->needAlpha() ?
        PNG_COLOR_TYPE_RGB_ALPHA:
        PNG_COLOR_TYPE_RGB;
      break;
    case IMAGE_GRAYSCALE:
      color_type = fop->document()->sprite()->needAlpha() ?
        PNG_COLOR_TYPE_GRAY_ALPHA:
        PNG_COLOR_TYPE_GRAY;
      break;
    case IMAGE_INDEXED:
      color_type = PNG_COLOR_TYPE_PALETTE;
      break;
  }

  png_set_IHDR(png_ptr, info_ptr, width, height, 8, color_type,
               PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

  if (image->pixelFormat() == IMAGE_INDEXED) {
    int c, r, g, b;
    int pal_size = fop->sequenceGetNColors();
    pal_size = MID(1, pal_size, PNG_MAX_PALETTE_LENGTH);

#if PNG_MAX_PALETTE_LENGTH != 256
#error PNG_MAX_PALETTE_LENGTH should be 256
#endif

    // Save the color palette.
    palette = (png_colorp)png_malloc(png_ptr, pal_size * sizeof(png_color));
    for (c = 0; c < pal_size; c++) {
      fop->sequenceGetColor(c, &r, &g, &b);
      palette[c].red   = r;
      palette[c].green = g;
      palette[c].blue  = b;
    }

    png_set_PLTE(png_ptr, info_ptr, palette, pal_size);

    // If the sprite does not have a (visible) background layer, we
    // put alpha=0 to the transparent color.
    int mask_entry = -1;
    if (fop->document()->sprite()->backgroundLayer() == NULL ||
        !fop->document()->sprite()->backgroundLayer()->isVisible()) {
      mask_entry = fop->document()->sprite()->transparentColor();
    }

    int num_trans = pal_size;
    png_bytep trans = (png_bytep)png_malloc(png_ptr, num_trans);

    for (c=0; c<num_trans; ++c) {
      int alpha = 255;
      fop->sequenceGetAlpha(c, &alpha);
      trans[c] = (c == mask_entry ? 0: alpha);
    }

    png_set_tRNS(png_ptr, info_ptr, trans, num_trans, NULL);
    png_free(png_ptr, trans);
  }

  /* Write the file header information. */
  png_write_info(png_ptr, info_ptr);

  /* pack pixels into bytes */
  png_set_packing(png_ptr);

  /* non-interlaced */
  number_passes = 1;

  row_pointer = (png_bytep)png_malloc(png_ptr, png_get_rowbytes(png_ptr, info_ptr));

  /* The number of passes is either 1 for non-interlaced images,
   * or 7 for interlaced images.
   */
  for (pass = 0; pass < number_passes; pass++) {
    /* If you are only writing one row at a time, this works */
    for (y = 0; y < height; y++) {
      /* RGB_ALPHA */
      if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_RGB_ALPHA) {
        uint32_t* src_address = (uint32_t*)image->getPixelAddress(0, y);
        uint8_t* dst_address = row_pointer;
        unsigned int x, c;

        for (x=0; x<width; x++) {
          c = *(src_address++);
          *(dst_address++) = rgba_getr(c);
          *(dst_address++) = rgba_getg(c);
          *(dst_address++) = rgba_getb(c);
          *(dst_address++) = rgba_geta(c);
        }
      }
      /* RGB */
      else if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_RGB) {
        uint32_t* src_address = (uint32_t*)image->getPixelAddress(0, y);
        uint8_t* dst_address = row_pointer;
        unsigned int x, c;

        for (x=0; x<width; x++) {
          c = *(src_address++);
          *(dst_address++) = rgba_getr(c);
          *(dst_address++) = rgba_getg(c);
          *(dst_address++) = rgba_getb(c);
        }
      }
      /* GRAY_ALPHA */
      else if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_GRAY_ALPHA) {
        uint16_t* src_address = (uint16_t*)image->getPixelAddress(0, y);
        uint8_t* dst_address = row_pointer;
        unsigned int x, c;

        for (x=0; x<width; x++) {
          c = *(src_address++);
          *(dst_address++) = graya_getv(c);
          *(dst_address++) = graya_geta(c);
        }
      }
      /* GRAY */
      else if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_GRAY) {
        uint16_t* src_address = (uint16_t*)image->getPixelAddress(0, y);
        uint8_t* dst_address = row_pointer;
        unsigned int x, c;

        for (x=0; x<width; x++) {
          c = *(src_address++);
          *(dst_address++) = graya_getv(c);
        }
      }
      /* PALETTE */
      else if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_PALETTE) {
        uint8_t* src_address = (uint8_t*)image->getPixelAddress(0, y);
        uint8_t* dst_address = row_pointer;
        unsigned int x;

        for (x=0; x<width; x++)
          *(dst_address++) = *(src_address++);
      }

      /* write the line */
      png_write_rows(png_ptr, &row_pointer, 1);

      fop->setProgress(
        (double)((double)pass + (double)(y+1) / (double)(height))
        / (double)number_passes);
    }
  }

  png_free(png_ptr, row_pointer);

  /* It is REQUIRED to call this to finish writing the rest of the file */
  png_write_end(png_ptr, info_ptr);

  /* If you png_malloced a palette, free it here (don't free info_ptr->palette,
     as recommended in versions 1.0.5m and earlier of this example; if
     libpng mallocs info_ptr->palette, libpng will free it).  If you
     allocated it with malloc() instead of png_malloc(), use free() instead
     of png_free(). */
  if (image->pixelFormat() == IMAGE_INDEXED) {
    png_free(png_ptr, palette);
    palette = NULL;
  }

  /* clean up after the write, and free any memory allocated */
  png_destroy_write_struct(&png_ptr, &info_ptr);

  /* all right */
  return true;
}

} // namespace app
