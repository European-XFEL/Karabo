/*
 * $Id: TestParam.cc 4643 2011-11-04 16:04:16Z heisenb@DESY.DE $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "TestParam.hh"
#include "../Factory.hh"
#include <cassert>

using namespace std;
using namespace exfel::util;

namespace exfel {
  namespace util {

    EXFEL_REGISTER_ONLY_ME_CC(TestParam)

    Schema TestParam::m_expected = Schema();

    void TestParam::expectedParameters(Schema& expected) {
      expected.append(TestParam::m_expected);
    }

    void TestParam::define(exfel::util::Schema& expected ){
      TestParam::m_expected.clear();
      TestParam::m_expected.append(expected);
    }

    void TestParam::configure(const Hash& input) {
      clear();
      append(input);
    }

  }
}
