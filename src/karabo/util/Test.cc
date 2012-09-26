/*
 * $Id: Test.cc 5229 2012-02-23 14:51:14Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "Test.hh"
#include <string>
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include "Exception.hh"
#include <cassert>
#include <boost/filesystem/path.hpp>

using namespace boost::filesystem;
using namespace std;


namespace karabo {
  namespace util {

    void Test::init(const string& funcSig, int argc, char** argv) {

      m_funcSig = funcSig;
      #if defined(_WIN32)
        boost::regex re("int __cdecl\\s(test.+)\\(");
      #else
        boost::regex re("int (test.+)\\(.+");
      #endif

      boost::smatch what;
      bool result = boost::regex_search(funcSig, what, re);
      if (result && what.size() == 2) {
        m_name = what.str(1);
      } else {
        throw INIT_EXCEPTION("Test could not be properly initialized");
      }

      path runPath = system_complete(initial_path<path > ());
      m_runDir = runPath.normalize().string();
      if (argc > 1) {
        runPath = system_complete(path(argv[1]));
      }
      m_dataDir = runPath.normalize().string();
    }

    const string& Test::dataDir() const {
      return m_dataDir;
    }

    const string& Test::runDir() const {
      return m_runDir;
    }

    const string& Test::name() const {
      return m_name;
    }

    string Test::file(const string& name) {
	if(name[0] == '/' ){
	    return name;
	}
      return path( path(dataDir()) / path(name) ).string();
    }

    std::string Test::info() const {
      string info = "Test: " + name() + "\n"
              + "CWD:  " + runDir() + "\n"
              + "data dir: " + dataDir();
      return info;
    }
  }
}
