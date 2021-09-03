// Aseprite Scripting Library
// Copyright (c) 2015-2016 David Capello
// Copyright (c) 2021 LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "base/injection.h"

namespace script {

    class EngineDelegate : public Injectable<EngineDelegate> {
    public:
        virtual ~EngineDelegate() { }
        virtual void onConsolePrint(const char* text) = 0;
    };

}
