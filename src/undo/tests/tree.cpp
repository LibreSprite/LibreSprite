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
  // 1 --- 2
  //  |
  //   ------ 3 --- 4
  int model = 0;
  Cmd cmd1(model, 1, 0);
  Cmd cmd2(model, 2, 1);
  Cmd cmd3(model, 3, 1);
  Cmd cmd4(model, 4, 3);

  UndoHistory history;
  cmd1.redo(); history.add(&cmd1);
  cmd2.redo(); history.add(&cmd2);
  history.undo();
  cmd3.redo(); history.add(&cmd3); // Creates a branch in the history
  cmd4.redo(); history.add(&cmd4);

  EXPECT_EQ(4, model);
  history.undo();
  EXPECT_EQ(3, model);
  history.undo();
  EXPECT_EQ(2, model);
  history.undo();
  EXPECT_EQ(1, model);
  history.undo();
  EXPECT_EQ(0,  model);
  EXPECT_FALSE(history.canUndo());
  history.redo();
  EXPECT_EQ(1, model);
  history.redo();
  EXPECT_EQ(2, model);
  history.redo();
  EXPECT_EQ(3, model);
  history.redo();
  EXPECT_EQ(4,  model);
  EXPECT_FALSE(history.canRedo());
}
