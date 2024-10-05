// LibreSprite | Copyright (C)      2024  LibreSprite contributors
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "app/pref/preferences.h"
#include "app/resource_finder.h"
#include "base/file_handle.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace app {
    namespace {
        std::string language;
        bool languageLoaded = false;
        std::unordered_map<std::string, std::string> translations;
        std::unordered_set<std::string> languageMissing;
    }

void loadLanguage(const std::string& language) {
  languageLoaded = true;
  translations.clear();

  base::FileHandle file;

  app::ResourceFinder rf;
  rf.includeDataDir(("languages/" + language + ".json").c_str());
  if (rf.findFirst())
      file = base::open_file(rf.filename().c_str(), "rb");

  if (!file) {
      std::cout << "Could not load translations for language: " << language << std::endl;
      return;
  }

  enum class State {
      start,
      KeyOrClose,
      InString,
      AfterKey,
      BeforeValue,
      AfterValue,
  } state = State::start;
  State nextState;

  char ch;
  std::string acc, key;
  bool escape = false;

  while (fread(&ch, 1, 1, file.get())) {
    switch (state) {
    case State::start:
      if (ch <= ' ') continue;
      if (ch == '{') {
        state = State::KeyOrClose;
        continue;
      }
      std::cout << "Unexpected character: " << ch << std::endl;
      return;

    case State::KeyOrClose:
      if (ch <= ' ' || ch == ',') continue;
      if (ch == '"') {
        state = State::InString;
        nextState = State::AfterKey;
        continue;
      }
      if (ch == '}') return;
      std::cout << "Unexpected character: " << ch << std::endl;
      return;

    case State::InString:
      if (escape) {
        escape = false;
        switch (ch) {
        case 'n': acc += '\n'; break;
        case 't': acc += '\t'; break;
        case '"': acc += '"'; break;
        case '\\': acc += '\\'; break;
        default: acc += ch; break;
        }
        continue;
      }
      escape = ch == '\\';
      if (escape)
          continue;
      if (ch == '"') {
          state = nextState;
          continue;
      }
      acc += ch;
      continue;

    case State::AfterKey:
      if (ch <= ' ') continue;
      if (ch == ':') {
        key = acc;
        acc.clear();
        state = State::BeforeValue;
        continue;
      }
      std::cout << "Unexpected character: " << ch << std::endl;
      return;

    case State::BeforeValue:
      if (ch <= ' ') continue;
      if (ch == '"') {
          state = State::InString;
          nextState = State::AfterValue;
          continue;
      }
      std::cout << "Unexpected character: " << ch << std::endl;
      return;

    case State::AfterValue:
      translations[key] = acc;
      acc.clear();
      state = State::KeyOrClose;
      break;
    }
  }
}

std::string i18n(const std::string& key, const std::string& src)
{
  if (!languageLoaded) {
    language = app::Preferences::instance().general.language();
    loadLanguage(language);
  }
  auto it = translations.find(key);
  if (it != translations.end())
      return it->second;
  if (!languageMissing.contains(key)) {
      std::cout << "\"" << key << "\" : \"" << src << "\"," << std::endl;
      languageMissing.insert(key);
  }
  return src;
}

void setLanguage(const std::string& language)
{
    app::language = language;
    languageLoaded = false;
    languageMissing.clear();
}

}
