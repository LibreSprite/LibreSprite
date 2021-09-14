// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "base/disable_copying.h"
#include "base/observable.h"
#include "doc/color_mode.h"
#include "doc/documents_observer.h"
#include "doc/object_id.h"

#include <vector>

namespace doc {
  class Context;
  class Document;

  class Documents : public base::Observable<DocumentsObserver> {
  public:
    typedef std::vector<Document*>::iterator iterator;
    typedef std::vector<Document*>::const_iterator const_iterator;

    Documents(Context* ctx);
    ~Documents();

    iterator begin() { return m_docs.begin(); }
    iterator end() { return m_docs.end(); }
    const_iterator begin() const { return m_docs.begin(); }
    const_iterator end() const { return m_docs.end(); }

    Document* front() const { return m_docs.front(); }
    Document* back() const { return m_docs.back(); }
    Document* lastAdded() const { return front(); }

    int size() const { return (int)m_docs.size(); }
    bool empty() const { return m_docs.empty(); }

    // Add a new documents to the list.
    Document* add(int width, int height, ColorMode mode = ColorMode::RGB, int ncolors = 256);
    Document* add(Document* doc);

    // Removes a document from the list without deleting it. You must
    // to delete the document after removing it.
    void remove(Document* doc);

    // Moves the document to the given location in the same
    // list. E.g. It is used to reorder documents when they are
    // selected as active.
    void move(Document* doc, int index);

    Document* operator[](int index) const { return m_docs[index]; }
    Document* getById(ObjectId id) const;
    Document* getByName(const std::string& name) const;
    Document* getByFileName(const std::string& filename) const;

  private:
    // Deletes all documents in the list (calling "delete" operation).
    void deleteAll();

    Context* m_ctx;
    std::vector<Document*> m_docs;

    DISABLE_COPYING(Documents);
  };

} // namespace doc
