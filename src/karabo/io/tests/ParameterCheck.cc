/*
 * $Id: ParameterCheck.cc 4973 2012-01-13 14:51:26Z wegerk@DESY.DE $
 *
 * File:   ParameterCheck.cc
 * Author: <irina.kozlova@xfel.eu>
 *
 * Created on December 1, 2010, 4:56 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "ParameterCheck.hh"

EXFEL_REGISTER_ONLY_ME_CC(exfel::io::ParameterCheck)

namespace exfel {

  namespace io {

    using namespace std;
    using namespace exfel::util;

    void ParameterCheck::expectedParameters(Schema& expected) {

      VECTOR_STRING_ELEMENT(expected).key("vectorString")
              .description("vector of strings")
              .displayedName("String Vector")
              .minSize(2)
              .maxSize(4)
              .assignmentOptional().noDefaultValue()
              .commit();

      INT32_ELEMENT(expected).key("Int32Value")
              .description("integer number")
              .displayedName("Int32 Value")
              .minInc(5)
              .maxInc(11)
              .assignmentOptional().noDefaultValue()
              .commit();

      VECTOR_INT32_ELEMENT(expected).key("vectorInt32")
              .description("vector of integer numbers")
              .displayedName("Int32 Vector")
              .minSize(2)
              .maxSize(10)
              .minInc(5)
              .maxInc(200)
              .assignmentOptional().noDefaultValue()
              .commit();

      VECTOR_BOOL_ELEMENT(expected).key("vectorBool")
              .description("vector of boolean")
              .displayedName("vectorOfBoolValues")
              .minSize(2)
              .maxSize(10)
              .assignmentOptional().noDefaultValue()
              .commit();
      
      VECTOR_DOUBLE_ELEMENT(expected).key("vectorDouble")
              .description("vector of double")
              .displayedName("vectorOfDoubleValues")
              .minSize(2)
              .maxSize(10)
              .minInc(1.5)
              .maxInc(5.5)
              .assignmentOptional().defaultValue(vector<double>(5, 7.7))
              .commit();

      UINT8_ELEMENT(expected).key("valueUInt8")
              .description("value unsigned char")
              .displayedName("valueUINT8")
              .assignmentOptional().noDefaultValue()
              .commit();
      
      INT8_ELEMENT(expected).key("valueInt8")
              .description("value signed char")
              .displayedName("valueINT8")
              .assignmentOptional().noDefaultValue()
              .commit();
      
      CHAR_ELEMENT(expected).key("valueChar")
              .description("value char")
              .displayedName("valueCHAR")
              .assignmentOptional().noDefaultValue()
              .commit();

      // TODO What is this? -Asks BH      
      INT8_ELEMENT(expected).key("valueInt8t")
              .description("value signed char, using int8_t")
              .displayedName("valueINT8t")
              .assignmentOptional().noDefaultValue()
              .commit();
      
      VECTOR_UINT8_ELEMENT(expected).key("vectorUInt8")
              .description("vector of unsigned char")
              .displayedName("vectorOfUINT8Values")
              .minSize(2)
              .maxSize(10)
              .assignmentOptional().noDefaultValue()
              .commit();

      VECTOR_INT8_ELEMENT(expected).key("vectorInt8")
              .description("vector of INT8 (signed char)")
              .displayedName("vectorOfINT8Values")
              .assignmentOptional().noDefaultValue()
              .commit();
      
      VECTOR_CHAR_ELEMENT(expected).key("vectorCHAR")
              .description("vector of CHAR (char)")
              .displayedName("vectorOfCHARValues")
              .assignmentOptional().noDefaultValue()
              .commit();
      
      PATH_ELEMENT(expected).key("filepath")
              .description("path to file")
              .displayedName("filepath")
              .assignmentOptional().noDefaultValue()
              .commit();
      
    }

    void ParameterCheck::configure(const Hash& input) {

    }

  } // namespace io
} // namespace exfel

