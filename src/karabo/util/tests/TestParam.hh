/*
 * $Id: TestParam.hh 5642 2012-03-20 13:11:09Z jszuba $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef EXFEL_UTIL_TESTPARAM_HH
#define	EXFEL_UTIL_TESTPARAM_HH

#include "../Factory.hh"
#include "utiltestdll.hh"

/**
 * The main European XFEL namespace
 */
namespace exfel {

  /**
   * Namespace for package util
   */
  namespace util {

    class DECLSPEC_UTILTEST TestParam : public Schema {
      static exfel::util::Schema m_expected;
    public:
      EXFEL_CLASSINFO(TestParam, "TP", "1.0")

      EXFEL_FACTORY_BASE_CLASS

      static void define(exfel::util::Schema& expected);
      static void expectedParameters(exfel::util::Schema& expected);
      void configure(const exfel::util::Hash& input);
    };
   
  }
}

EXFEL_REGISTER_FACTORY_BASE_HH(exfel::util::TestParam, TEMPLATE_UTILTEST, DECLSPEC_UTILTEST)

#endif	/* EXFEL_UTIL_TESTPARAM_HH */
