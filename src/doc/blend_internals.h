// LibreSprite Document Library
// Copyright (C) 2001-2015  David Capello
// Copyright (C) 2021       LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#define MASK 0xff
#define ONE_HALF 0x80
#define G_SHIFT 8

#define MUL_UN8(a, b, t)						\
    ((t) = (a) * (uint16_t)(b) + ONE_HALF, ((((t) >> G_SHIFT ) + (t) ) >> G_SHIFT ))

#define DIV_UN8(a, b)							\
    (((uint16_t) (a) * MASK + ((b) / 2)) / (b))
