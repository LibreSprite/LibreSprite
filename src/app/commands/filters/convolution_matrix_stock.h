// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include <vector>

#include "base/shared_ptr.h"

namespace filters {
  class ConvolutionMatrix;
}

namespace app {
  using namespace filters;

  // A container of all convolution matrices in the convmatr.def file.
  class ConvolutionMatrixStock {
  public:
    typedef std::vector<base::SharedPtr<ConvolutionMatrix> >::iterator iterator;
    typedef std::vector<base::SharedPtr<ConvolutionMatrix> >::const_iterator const_iterator;

    ConvolutionMatrixStock();
    virtual ~ConvolutionMatrixStock();

    iterator begin() { return m_matrices.begin(); }
    iterator end() { return m_matrices.end(); }
    const_iterator begin() const { return m_matrices.begin(); }
    const_iterator end() const { return m_matrices.end(); }

    base::SharedPtr<ConvolutionMatrix> getByName(const char* name);

    void reloadStock();
    void cleanStock();

  private:
    std::vector<base::SharedPtr<ConvolutionMatrix> > m_matrices;
  };

} // namespace app
