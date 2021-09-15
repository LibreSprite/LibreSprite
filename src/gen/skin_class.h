// Aseprite Code Generator
// Copyright (c) 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include <string>
#include "tinyxml.h"

void gen_skin_class(TiXmlDocument* doc, const std::string& inputFn);
