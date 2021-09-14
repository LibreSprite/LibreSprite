// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "doc/context.h"
#include "doc/document.h"
#include "doc/layer.h"
#include "doc/site.h"
#include "doc/sprite.h"

namespace doc {

  template<typename Base>
  class TestContextT : public Base {
  public:
    TestContextT() : m_activeDoc(nullptr) {
    }

  protected:

    void onGetActiveSite(Site* site) const override {
      Document* doc = m_activeDoc;
      if (!doc)
        return;

      site->document(doc);
      site->sprite(doc->sprite());
      site->layer(doc->sprite()->folder()->getFirstLayer());
      site->frame(0);
    }

    void onAddDocument(Document* doc) override {
      m_activeDoc = doc;
      this->notifyActiveSiteChanged();
    }

    void onRemoveDocument(Document* doc) override {
      if (m_activeDoc == doc) {
        m_activeDoc = nullptr;
        this->notifyActiveSiteChanged();
      }
    }

  private:
    Document* m_activeDoc;
  };

  typedef TestContextT<Context> TestContext;

} // namespace doc
