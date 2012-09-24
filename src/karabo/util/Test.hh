/*
 * $Id: Test.hh 5229 2012-02-23 14:51:14Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef EXFEL_UTIL_TEST_HH
#define	EXFEL_UTIL_TEST_HH

#include <string>
#include <iostream>
#include <boost/current_function.hpp>
#include "Exception.hh"

namespace exfel {
  namespace util {

    class DECLSPEC_UTIL Test {
    public:

      void init(const std::string& name, int argc, char** argv);
      const std::string& dataDir() const;
      const std::string& runDir() const;
      const std::string& name() const;
      std::string info() const;
      std::string file( const std::string& name);
      
      friend std::ostream & operator<<(std::ostream& os, const Test& param) {
        os << param.info();
        return os;
      }

#define TEST_INIT(tobj,targc,targv) tobj.init(BOOST_CURRENT_FUNCTION , targc, targv)

    private:

      std::string m_dataDir;
      std::string m_runDir;
      std::string m_name;
      std::string m_funcSig;
    };

  }
}


#endif	/* EXFEL_UTIL_TEST_HH */

