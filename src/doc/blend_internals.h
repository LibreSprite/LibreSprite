// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_BLEND_INTERNALS_H_INCLUDED
#define DOC_BLEND_INTERNALS_H_INCLUDED
#pragma once

#define MASK 0xff
#define ONE_HALF 0x80
#define G_SHIFT 8

#define MUL_UN8(a, b, t)						\
    ((t) = (a) * (uint16_t)(b) + ONE_HALF, ((((t) >> G_SHIFT ) + (t) ) >> G_SHIFT ))

#define DIV_UN8(a, b)							\
    (((uint16_t) (a) * MASK + ((b) / 2)) / (b))

#endif
