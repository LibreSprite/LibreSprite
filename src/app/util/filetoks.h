// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include <stdio.h>

void tok_reset_line_num();
int tok_line_num();

char* tok_read(FILE* f, char* buf, char* leavings, int sizeof_leavings);
