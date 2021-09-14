// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/document_access.h"
#include "doc/site.h"

namespace app {

  class Context;

  template<typename DocumentAccessT>
  class ContextAccess {
  public:
    const Context* context() const { return m_context; }
    const Site* site() const { return &m_site; }
    const DocumentAccessT& document() const { return m_document; }
    const Sprite* sprite() const { return m_site.sprite(); }
    const Layer* layer() const { return m_site.layer(); }
    frame_t frame() const { return m_site.frame(); }
    const Cel* cel() const { return m_site.cel(); }

    // You cannot change the site directly from a writable ContextAccess anyway.
    const Site* site() { return &m_site; }

    Context* context() { return const_cast<Context*>(m_context); }
    DocumentAccessT& document() { return m_document; }
    Sprite* sprite() { return m_site.sprite(); }
    Layer* layer() { return m_site.layer(); }
    Cel* cel() { return m_site.cel(); }

    Image* image(int* x = NULL, int* y = NULL, int* opacity = NULL) const {
      return m_site.image(x, y, opacity);
    }

    Palette* palette() const {
      return m_site.palette();
    }

  protected:
    ContextAccess(const Context* context, int timeout)
      : m_context(context)
      , m_document(context->activeDocument(), timeout)
      , m_site(context->activeSite())
    {
    }

    template<typename DocumentReaderT>
    ContextAccess(const Context* context, const DocumentReaderT& documentReader, int timeout)
      : m_context(context)
      , m_document(documentReader, timeout)
      , m_site(context->activeSite())
    {
    }

  private:
    const Context* m_context;
    DocumentAccessT m_document;
    Site m_site;
  };

  // You can use this class to access to the given context to read the
  // active document.
  class ContextReader : public ContextAccess<DocumentReader> {
  public:
    ContextReader(const Context* context, int timeout = 0)
      : ContextAccess<DocumentReader>(context, timeout) {
    }
  };

  // You can use this class to access to the given context to write the
  // active document.
  class ContextWriter : public ContextAccess<DocumentWriter> {
  public:
    ContextWriter(const Context* context, int timeout = 0)
      : ContextAccess<DocumentWriter>(context, timeout) {
    }

    ContextWriter(const ContextReader& reader, int timeout = 0)
      : ContextAccess<DocumentWriter>(reader.context(), reader.document(), timeout) {
    }
  };

} // namespace app
