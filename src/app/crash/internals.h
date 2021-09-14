// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "doc/object.h"

#include <algorithm>
#include <functional>
#include <map>

namespace app {
namespace crash {

  const uint32_t MAGIC_NUMBER = 0x454E4946; // 'FINE' in ASCII

  class ObjVersions {
  public:
    ObjVersions() {
      m_vers[0] = 0;
      m_vers[1] = 0;
      m_vers[2] = 0;
    }

    size_t size() const { return 3; }
    doc::ObjectVersion operator[](int i) const { return m_vers[i]; }

    doc::ObjectVersion newer() { return m_vers[0]; }
    doc::ObjectVersion older() { return m_vers[2]; }

    // Adds a newer version
    void rotateRevisions(doc::ObjectVersion newer) {
      // Rotate versions
      m_vers[2] = m_vers[1];
      m_vers[1] = m_vers[0];
      m_vers[0] = newer;
    }

    // Adds a version (we don't know if the version if the latest one)
    void add(doc::ObjectVersion ver) {
      auto minver = std::min_element(m_vers, m_vers+2);
      if (*minver < ver) {
        *minver = ver;
        std::sort(m_vers, m_vers+2, std::greater<doc::ObjectVersion>());
      }
    }

  private:
    doc::ObjectVersion m_vers[3];
  };

  typedef std::map<doc::ObjectId, ObjVersions> ObjVersionsMap;

} // namespace crash
} // namespace app
