// Undo Library
// Copyright (C) 2015-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "test.h"

#include "cmd.h"
#include "undo_history.h"

using namespace undo;

int main(int argc, char** argv)
{
  int model = 0;
  Cmd cmd1(model, 1, 0);
  Cmd cmd2(model, 2, 1);

  EXPECT_EQ(0, model);
  cmd1.redo();
  EXPECT_EQ(1, model);
  cmd2.redo();
  EXPECT_EQ(2, model);

  UndoHistory history;
  EXPECT_FALSE(history.canUndo());
  EXPECT_FALSE(history.canRedo());
  history.add(&cmd1);
  EXPECT_TRUE(history.canUndo());
  EXPECT_FALSE(history.canRedo());
  history.add(&cmd2);
  EXPECT_TRUE(history.canUndo());
  EXPECT_FALSE(history.canRedo());

  history.undo();
  EXPECT_EQ(1, model);
  EXPECT_TRUE(history.canUndo());
  EXPECT_TRUE(history.canRedo());
  history.undo();
  EXPECT_EQ(0, model);
  EXPECT_FALSE(history.canUndo());
  EXPECT_TRUE(history.canRedo());

  history.redo();
  EXPECT_EQ(1, model);
  EXPECT_TRUE(history.canUndo());
  EXPECT_TRUE(history.canRedo());
  history.redo();
  EXPECT_EQ(2, model);
  EXPECT_TRUE(history.canUndo());
  EXPECT_FALSE(history.canRedo());
}
