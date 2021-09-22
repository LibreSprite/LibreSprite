// Aseprite    | Copyright (C) 2001-2015  David Capello
// LibreSprite | Copyright (C) 2021       LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include <string>
#include <memory>

namespace app {

  class Resource {
    std::shared_ptr<void> m_resource;
    std::string m_name;

  public:
    Resource() = default;

    template<typename Type>
    Resource(Type* resource, const std::string& name) : m_resource(resource), m_name(name) {}

    template<typename Type>
    Resource(std::shared_ptr<Type> resource, const std::string& name) : m_resource(resource), m_name(name) {}

    const std::string& name() const { return m_name; }

    operator bool () {
      return m_resource.get();
    }

    template<typename Type=void>
    std::shared_ptr<Type> get() {
      return std::static_pointer_cast<Type>(m_resource);
    }
  };

} // namespace app
