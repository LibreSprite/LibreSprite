// Aseprite Config Library
// Copyright (c) 2014-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "cfg/cfg.h"

#include "base/file_handle.h"
#include "base/log.h"
#include "base/string.h"

#include <stdlib.h>
#include "SimpleIni.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include "she/system.h"

std::string s_cfgdata;

void cfgwrite(){
    she::instance()->gfx([&]{
      EM_ASM((
	self.storage.data = JSON.parse(UTF8ToString($0));
	self.storage.dirty = true;
	self.storage.commit();
      ), s_cfgdata.c_str());
    });
}

bool cfginit(){
  auto data = (char*) EM_ASM_PTR((
      if (self.storage)
	return self.storage.data ? stringToNewUTF8(JSON.stringify(self.storage.data)) : 0;
      self.storage = {
        data:null,
	db:null,
	dirty:false,
	init(){
	  if (storage.wasInit)
	      return;
	  storage.wasInit = true;

	  Object.assign(indexedDB.open("UserSettings", 1), {
	    onupgradeneeded(){
	      storage.db = this.result;
	      console.log(storage.db);
	      storage.db.createObjectStore('userSettings', {keyPath: 'key'});
	    },
	    onerror(){},
	    onsuccess(){
	      storage.db = this.result;
	      let transaction = storage.db.transaction('userSettings', 'readonly');
	      let userSettings = transaction.objectStore('userSettings');
	      Object.assign(userSettings.get('str'), {
		onsuccess(){
		  storage.data = (this.result ?? {data:{}}).data;
		}
	      });
	    }
	  });
	},
	commit(){
	  if (!storage.dirty) return;
	  let transaction = storage.db.transaction('userSettings', 'readwrite');
	  let userSettings = transaction.objectStore('userSettings');
	  Object.assign(userSettings.put({key:'str', data:storage.data}), {
	    onsuccess(){storage.dirty = false;},
	    onerror(){}
	  });
	},
	getItem(key){
	  console.log(key);
	  return storage.data[key];
	},
	setItem(key, value){
	  console.log(key, value);
	  storage.dirty = storage.data[key] != value;
	  storage.data[key] = value;
	}
      };
      storage.init();
      return 0;
    ));

  if (!data)
      return false;

  s_cfgdata = data;
  free(data);
  return true;
}

void thread_init() {
  EM_ASM((
    self.storage = {
    data: JSON.parse(UTF8ToString($0)),
    getItem(key){ return storage.data[key]; },
    setItem(key, value){ storage.data[key] = value; }
    };
  ), s_cfgdata.c_str());
}
#endif

namespace cfg {

class CfgFile::CfgFileImpl {
public:
  const std::string& filename() const {
    return m_filename;
  }

  const char* getValue(const char* section, const char* name, const char* defaultValue) const {
    return m_ini.GetValue(section, name, defaultValue);
  }

  bool getBoolValue(const char* section, const char* name, bool defaultValue) const {
    return m_ini.GetBoolValue(section, name, defaultValue);
  }

  int getIntValue(const char* section, const char* name, int defaultValue) const {
    return m_ini.GetLongValue(section, name, defaultValue);
  }

  double getDoubleValue(const char* section, const char* name, double defaultValue) const {
    return m_ini.GetDoubleValue(section, name, defaultValue);
  }

  void setValue(const char* section, const char* name, const char* value) {
    m_ini.SetValue(section, name, value);
  }

  void setBoolValue(const char* section, const char* name, bool value) {
    m_ini.SetBoolValue(section, name, value);
  }

  void setIntValue(const char* section, const char* name, int value) {
    m_ini.SetLongValue(section, name, value);
  }

  void setDoubleValue(const char* section, const char* name, double value) {
    m_ini.SetDoubleValue(section, name, value);
  }

  void deleteValue(const char* section, const char* name) {
    m_ini.Delete(section, name, true);
  }

  void load(const std::string& filename) {
    m_filename = filename;

    base::FileHandle file(base::open_file(m_filename, "rb"));
    if (file) {
      SI_Error err = m_ini.LoadFile(file.get());
      if (err != SI_OK)
        LOG("Error '%d' loading configuration from '%s'.", err, m_filename.c_str());
    } else {
      std::string data;
#ifdef __EMSCRIPTEN__
      thread_init();
      auto raw = (char*) EM_ASM_PTR({
	const value = self.storage.getItem(UTF8ToString($0));
	if (value === undefined)
	  return 0;
	return stringToNewUTF8(value);
      }, m_filename.c_str());
      if (raw) {
	data = raw;
	free(raw);
      }
#endif
      if (!data.empty())
	m_ini.LoadData(data);
    }
  }

  void save() {
    std::string data;
    SI_Error err = m_ini.Save(data);
    if (err != SI_OK) {
      LOG("Error '%d' saving configuration into '%s'.", err, m_filename.c_str());
      return;
    }

#ifdef __EMSCRIPTEN__
    thread_init();
    auto cfgdata = (char*) EM_ASM_PTR({
      self.storage.setItem(UTF8ToString($0), UTF8ToString($1));
      return stringToNewUTF8(JSON.stringify(self.storage.data));
    }, m_filename.c_str(), data.c_str());
    if (cfgdata) {
	s_cfgdata = cfgdata;
	free(cfgdata);
	cfgwrite();
    }
#endif

    if (base::FileHandle file = base::open_file(m_filename, "wb")) {
	std::fwrite(data.c_str(), 1, data.size(), file.get());
    }
  }

private:
  std::string m_filename;
  CSimpleIniA m_ini;
};

CfgFile::CfgFile()
  : m_impl(new CfgFileImpl)
{
}

CfgFile::~CfgFile()
{
  delete m_impl;
}

const std::string& CfgFile::filename() const
{
  return m_impl->filename();
}

const char* CfgFile::getValue(const char* section, const char* name, const char* defaultValue) const
{
  return m_impl->getValue(section, name, defaultValue);
}

bool CfgFile::getBoolValue(const char* section, const char* name, bool defaultValue)
{
  return m_impl->getBoolValue(section, name, defaultValue);
}

int CfgFile::getIntValue(const char* section, const char* name, int defaultValue)
{
  return m_impl->getIntValue(section, name, defaultValue);
}

double CfgFile::getDoubleValue(const char* section, const char* name, double defaultValue)
{
  return m_impl->getDoubleValue(section, name, defaultValue);
}

void CfgFile::setValue(const char* section, const char* name, const char* value)
{
  m_impl->setValue(section, name, value);
}

void CfgFile::setBoolValue(const char* section, const char* name, bool value)
{
  m_impl->setBoolValue(section, name, value);
}

void CfgFile::setIntValue(const char* section, const char* name, int value)
{
  m_impl->setIntValue(section, name, value);
}

void CfgFile::setDoubleValue(const char* section, const char* name, double value)
{
  m_impl->setDoubleValue(section, name, value);
}

void CfgFile::deleteValue(const char* section, const char* name)
{
  m_impl->deleteValue(section, name);
}

void CfgFile::load(const std::string& filename)
{
  m_impl->load(filename);
}

void CfgFile::save()
{
  m_impl->save();
}

} // namespace cfg
