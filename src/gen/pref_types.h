// Aseprite Code Generator
// Copyright (c) 2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include <string>
#include "tinyxml2.h"

void gen_pref_header(tinyxml2::XMLDocument* doc, const std::string& inputFn);
void gen_pref_impl(tinyxml2::XMLDocument* doc, const std::string& inputFn);
