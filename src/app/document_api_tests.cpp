// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "tests/test.h"

#include "app/context.h"
#include "app/document.h"
#include "app/document_api.h"
#include "app/transaction.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/primitives.h"
#include "doc/test_context.h"

#include <memory>

using namespace app;
using namespace doc;

typedef std::unique_ptr<app::Document> DocumentPtr;

TEST(DocumentApi, MoveCel) {
  TestContextT<app::Context> ctx;
  DocumentPtr doc(static_cast<app::Document*>(ctx.documents().add(32, 16)));
  Sprite* sprite = doc->sprite();
  LayerImage* layer1 = dynamic_cast<LayerImage*>(sprite->folder()->getFirstLayer());
  LayerImage* layer2 = new LayerImage(sprite);

  Cel* cel1 = layer1->cel(frame_t(0));
  cel1->setPosition(2, -2);
  cel1->setOpacity(128);

  Image* image1 = cel1->image();
  EXPECT_EQ(32, image1->width());
  EXPECT_EQ(16, image1->height());
  for (int v=0; v<image1->height(); ++v)
    for (int u=0; u<image1->width(); ++u)
      image1->putPixel(u, v, u+v*image1->width());

  // Create a copy for later comparison.
  std::unique_ptr<Image> expectedImage(Image::createCopy(image1));

  Transaction transaction(&ctx, "");
  doc->getApi(transaction).moveCel(
    layer1, frame_t(0),
    layer2, frame_t(1));
  transaction.commit();

  EXPECT_EQ(NULL, layer1->cel(frame_t(0)));

  Cel* cel2 = layer2->cel(frame_t(1));
  ASSERT_TRUE(cel2 != NULL);

  Image* image2 = cel2->image();
  EXPECT_EQ(32, image2->width());
  EXPECT_EQ(16, image2->height());
  EXPECT_EQ(0, count_diff_between_images(expectedImage.get(), image2));
  EXPECT_EQ(2, cel2->x());
  EXPECT_EQ(-2, cel2->y());
  EXPECT_EQ(128, cel2->opacity());

  doc->close();
}
