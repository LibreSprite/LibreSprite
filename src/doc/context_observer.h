// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

namespace doc {

  class Document;
  class Site;

  class ContextObserver {
  public:
    virtual ~ContextObserver() { }
    virtual void onActiveSiteChange(const Site& site) { }
  };

} // namespace doc
