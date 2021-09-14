// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include <exception>
#include <string>

namespace base {

  class Exception : public std::exception {
  public:
    Exception() throw();
    Exception(const char* format, ...) throw();
    Exception(const std::string& msg) throw();
    virtual ~Exception() throw();

    const char* what() const throw();

  protected:
    void setMessage(const char* msg) throw();

  private:
    std::string m_msg;
  };

}
