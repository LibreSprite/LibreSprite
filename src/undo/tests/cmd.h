// Undo Library
// Copyright (C) 2015-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef CMD_H_INCLUDED
#define CMD_H_INCLUDED
#pragma once

#include "undo_command.h"

// Command used in some tests to handle a simple "int" model
class Cmd : public undo::UndoCommand {
public:
  Cmd(int& model, int redo_value, int undo_value)
    : m_model(model)
    , m_redo_value(redo_value)
    , m_undo_value(undo_value) {
  }
  void redo() override { m_model = m_redo_value; }
  void undo() override { m_model = m_undo_value; }
  void dispose() override { }
private:
  int& m_model;
  int m_redo_value;
  int m_undo_value;
};

#endif  // CMD_H_INCLUDED
