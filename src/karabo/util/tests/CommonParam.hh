/*
 * $Id: CommonParam.hh 5849 2012-04-04 06:57:26Z wegerk $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef EXFEL_UTIL_COMMONPARAM_HH
#define	EXFEL_UTIL_COMMONPARAM_HH

#include <string>
#include "../Schema.hh"
#include "TestParam.hh"

namespace exfel {
  namespace util {

    class CommonParam {
    public:

      template<typename T, Types::Type TType> int testSimpleType(Types::Type type, T def, T min, T max, T step);

      template<typename T, Types::Type TType> int testDefault(Types::Type type, T def);

      template<typename T, Types::Type TType> int testMinInc(Types::Type type, T min, T val);
      template<typename T, Types::Type TType> int testMinIncError(Types::Type type, T min, T val);

      template<typename T, Types::Type TType> int testMinExc(Types::Type type, T min, T val);
      template<typename T, Types::Type TType> int testMinExcError(Types::Type type, T min, T val);

      template<typename T, Types::Type TType> int testMaxInc(Types::Type type, T val, T max);
      template<typename T, Types::Type TType> int testMaxIncError(Types::Type type, T val, T max);

      template<typename T, Types::Type TType> int testMaxExc(Types::Type type, T val, T max);
      template<typename T, Types::Type TType> int testMaxExcError(Types::Type type, T val, T max);

      template<typename T, Types::Type TType> int testMinMaxSize(Types::Type type, T val, unsigned int minSize, unsigned int maxSize);
      template<typename T, Types::Type TType> int testMinMaxSizeError(Types::Type type, T val, unsigned int minSize, unsigned int maxSize);

      template<typename T, typename Y> int testMinMaxIncVect(Types::Type type, T min, T max, Y valvect);
      template<typename T, typename Y> int testMinMaxIncVectError(Types::Type type, T min, T max, Y vect);

      template<typename T, typename Y> int testMinMaxExcVect(Types::Type type, T min, T max, Y valvect);
      template<typename T, typename Y> int testMinMaxExcVectError(Types::Type type, T min, T max, Y vect);

      template<typename T> int testOptions(Types::Type type, std::string& opt, T val);
      template<typename T> int testOptionsError(Types::Type type, std::string& opt, T val);

      int testAccess(AccessType access);
    private:


    };

    template<typename T, Types::Type TType> int CommonParam::testSimpleType(Types::Type type, T def, T min, T max, T step) {

      cout << "Testing " << Types::getInstance().convert(type) << endl;
      cout << "sizeof[bits]: " << 8 * sizeof (def) << endl;
      assert(sizeof (def) == sizeof (min));
      assert(sizeof (def) == sizeof (max));
      assert(sizeof (def) == sizeof (step));

      Schema expected, c;

      SimpleElement<TType > (expected)
              .key("a").defaultValue(def)
              .displayedName("a")
              .description("a")
              .minInc(min).maxInc(max)
              .assignment(Schema::OPTIONAL_PARAM)
              .access(INIT)
              .commit();

      TestParam::define(expected);

      { // check default
        TestParam::Pointer pp = TestParam::create(Schema("TP"));
        assert(pp->get<T > ("a") == def);
      }

      {
        for (T i = min; i < max; i += step) {
          c.clear();
          c.setFromPath("TP.a", i);
          cout << "TP.a=" << i << " ";
          TestParam::Pointer pp = TestParam::create(c);
          assert(pp->get<T > ("a") == i);
        }
        cout << endl;
      }
      return 0;

    }

    template<typename T, Types::Type TType> int CommonParam::testDefault(Types::Type type, T def) {
      string typeStr = Types::getInstance().convert(type);
      cout << "Testing defaultValue: " << typeStr << " ";
      cout << "sizeof[bits]: " << 8 * sizeof (def) << endl;
      Schema expected;

      SimpleElement<TType > (expected)
              .key("a").defaultValue(def)
              .displayedName("a")
              .description("a")
              .assignment(Schema::OPTIONAL_PARAM)
              .access(INIT)
              .commit();

      TestParam::define(expected);

      TestParam::Pointer pp = TestParam::create(Schema("TP"));
      assert(pp->get<T > ("a") == def);

      return 0;

    }

    template<typename T, Types::Type TType> int CommonParam::testMinInc(Types::Type type, T min, T val) {
      string typeStr = Types::getInstance().convert(type);
      cout << "Testing " << typeStr << " ";
      cout << "sizeof[bits]: " << 8 * sizeof (min) << " ";
      if (typeStr != "INT8") {
        cout << "min=" << min << " val=" << val;
      }
      cout << endl;
      assert(sizeof (min) == sizeof (val));
      assert(min <= val);

      Schema expected;

      SimpleElement<TType > (expected)
              .key("a")
              .displayedName("a")
              .description("a")
              .assignment(Schema::OPTIONAL_PARAM)
              .access(INIT)
              .minInc(min)
              .commit();

      TestParam::define(expected);

      Schema c;
      c.setFromPath("TP.a", val);
      TestParam::create(c);

      return 0;
    }

    template<typename T, Types::Type TType> int CommonParam::testMinIncError(Types::Type type, T min, T val) {
      string typeStr = Types::getInstance().convert(type);
      cout << "Testing " << typeStr << " ";
      cout << "sizeof[bits]: " << 8 * sizeof (min) << " ";
      if (typeStr != "INT8") {
        cout << "min=" << min << " val=" << val;
      }
      cout << endl;
      assert(sizeof (min) == sizeof (val));
      assert(min > val);

      Schema expected;
      SimpleElement<TType > (expected)
              .key("a")
              .displayedName("a")
              .description("a")
              .assignment(Schema::OPTIONAL_PARAM)
              .access(INIT)
              .minInc(min)
              .commit();

      TestParam::define(expected);

      try {

        Schema c;
        c.setFromPath("TP.a", val);
        TestParam::create(c);
        cout << "Parameter: val=" << val << " out of range has not caused ParameterException" << endl;
        assert(false);
      } catch (ParameterException e) {
      }
      return 0;
    }

    template<typename T, Types::Type TType> int CommonParam::testMinExc(Types::Type type, T min, T val) {

      string typeStr = Types::getInstance().convert(type);
      cout << "Testing " << typeStr << " ";
      cout << "sizeof[bits]: " << 8 * sizeof (min) << " ";
      if (typeStr != "INT8") {
        cout << "min=" << min << " val=" << val;
      }
      cout << endl;
      assert(sizeof (min) == sizeof (val));
      assert(min < val); // arument check

      Schema expected;
      SimpleElement<TType > (expected)
              .key("a")
              .displayedName("a")
              .description("a")
              .assignment(Schema::OPTIONAL_PARAM)
              .access(INIT)
              .minExc(min)
              .commit();

      TestParam::define(expected);

      Schema c;
      c.setFromPath("TP.a", val);
      TestParam::create(c);

      return 0;
    }

    template<typename T, Types::Type TType> int CommonParam::testMinExcError(Types::Type type, T min, T val) {

      string typeStr = Types::getInstance().convert(type);
      cout << "Testing " << typeStr << " ";
      cout << "sizeof[bits]: " << 8 * sizeof (min) << " ";
      if (typeStr != "INT8") {
        cout << "min=" << min << " val=" << val;
      }
      cout << endl;
      assert(sizeof (min) == sizeof (val));
      assert(min >= val);

      Schema element, expected;
      element.clear();
      element.description("a");
      element.key("a");
      element.displayedName("a");
      element.simpleType(type);
      element.minExc(min);
      element.assignment(Schema::OPTIONAL_PARAM);
      expected.addElement(element);
      TestParam::define(expected);

      try {

        Schema c;
        c.setFromPath("TP.a", val);
        TestParam::create(c);
        cout << "Parameter out of range has not caused ParameterException" << endl;
        assert(false);
      } catch (ParameterException e) {
      }
      return 0;
    }

    template<typename T, Types::Type TType> int CommonParam::testMaxInc(Types::Type type, T val, T max) {
      string typeStr = Types::getInstance().convert(type);
      cout << "Testing " << typeStr << " ";
      cout << "sizeof[bits]: " << 8 * sizeof (max) << " ";
      if (typeStr != "INT8") {
        cout << "val=" << val << " max=" << max;
      }
      cout << endl;
      assert(sizeof (max) == sizeof (val));
      assert(val <= max);

      Schema element, expected;
      element.clear();
      element.description("a");
      element.key("a");
      element.displayedName("a");
      element.simpleType(type);
      element.maxInc(max);
      element.assignment(Schema::OPTIONAL_PARAM);
      expected.addElement(element);
      TestParam::define(expected);

      Schema c;
      c.setFromPath("TP.a", val);
      TestParam::create(c);

      return 0;
    }

    template<typename T, Types::Type TType> int CommonParam::testMaxIncError(Types::Type type, T val, T max) {
      string typeStr = Types::getInstance().convert(type);
      cout << "Testing " << typeStr << " ";
      cout << "sizeof[bits]: " << 8 * sizeof (max) << " ";
      if (typeStr != "INT8") {
        cout << "val=" << val << " max=" << max;
      }
      cout << endl;
      assert(sizeof (max) == sizeof (val));
      assert(val > max);

      Schema element, expected;
      element.clear();
      element.description("a");
      element.key("a");
      element.displayedName("a");
      element.simpleType(type);
      element.maxInc(max);
      element.assignment(Schema::OPTIONAL_PARAM);
      expected.addElement(element);
      TestParam::define(expected);

      try {

        Schema c;
        c.setFromPath("TP.a", val);
        TestParam::create(c);
        cout << "Parameter: val=" << val << " out of range has not caused ParameterException" << endl;
        assert(false);
      } catch (ParameterException e) {
      }
      return 0;
    }

    template<typename T, Types::Type TType> int CommonParam::testMaxExc(Types::Type type, T val, T max) {
      string typeStr = Types::getInstance().convert(type);
      cout << "Testing " << typeStr << " ";
      cout << "sizeof[bits]: " << 8 * sizeof (max) << " ";
      if (typeStr != "INT8") {
        cout << "val=" << val << " max=" << max;
      }
      cout << endl;
      assert(sizeof (max) == sizeof (val));
      assert(val < max);

      Schema element, expected;
      element.clear();
      element.description("a");
      element.key("a");
      element.displayedName("a");
      element.simpleType(type);
      element.maxExc(max);
      element.assignment(Schema::OPTIONAL_PARAM);
      expected.addElement(element);
      TestParam::define(expected);

      Schema c;
      c.setFromPath("TP.a", val);
      TestParam::create(c);

      return 0;
    }

    template<typename T, Types::Type TType> int CommonParam::testMaxExcError(Types::Type type, T val, T max) {
      string typeStr = Types::getInstance().convert(type);
      cout << "Testing " << typeStr << " ";
      cout << "sizeof[bits]: " << 8 * sizeof (max) << " ";
      if (typeStr != "INT8") {
        cout << "val=" << val << " max=" << max;
      }
      cout << endl;
      assert(sizeof (max) == sizeof (val));
      assert(val >= max);

      Schema element, expected;
      element.clear();
      element.description("a");
      element.key("a");
      element.displayedName("a");
      element.simpleType(type);
      element.maxExc(max);
      element.assignment(Schema::OPTIONAL_PARAM);
      expected.addElement(element);
      TestParam::define(expected);

      try {

        Schema c;
        c.setFromPath("TP.a", val);
        TestParam::create(c);
        cout << "Parameter: val=" << val << " out of range has not caused ParameterException" << endl;
        assert(false);
      } catch (ParameterException e) {
      }
      return 0;
    }

    template<typename T, Types::Type TType> int CommonParam::testMinMaxSize(Types::Type type, T val, unsigned int minSize, unsigned int maxSize) {
      string typeStr = Types::getInstance().convert(type);
      cout << "for vector " << typeStr << " of size " << val.size() << " ";
      cout << "minSize= " << minSize << ", maxSize= " << maxSize << endl;
      assert(minSize <= maxSize);

      Schema element, expected;
      element.clear();
      element.description("a");
      element.key("a");
      element.displayedName("a");
      element.simpleType(type);
      element.minSize(minSize);
      element.maxSize(maxSize);
      element.assignment(Schema::OPTIONAL_PARAM);
      expected.addElement(element);
      TestParam::define(expected);

      Schema c;
      c.setFromPath("TP.a", val);
      TestParam::create(c);

      return 0;
    }

    template<typename T, Types::Type TType> int CommonParam::testMinMaxSizeError(Types::Type type, T val, unsigned int minSize, unsigned int maxSize) {
      string typeStr = Types::getInstance().convert(type);
      cout << "for vector " << typeStr << " of size " << val.size() << " ";
      cout << "minSize= " << minSize << ", maxSize= " << maxSize << endl;
      assert(minSize <= maxSize);

      Schema element, expected;
      element.clear();
      element.description("a");
      element.key("a");
      element.displayedName("a");
      element.simpleType(type);
      element.minSize(minSize);
      element.maxSize(maxSize);
      element.assignment(Schema::OPTIONAL_PARAM);
      expected.addElement(element);
      TestParam::define(expected);

      try {

        Schema c;
        c.setFromPath("TP.a", val);
        TestParam::create(c);
        cout << "Size of vector is out of range has not caused ParameterException" << endl;
        assert(false);
      } catch (ParameterException e) {
      }

      return 0;
    }

    template<typename T, typename Y> int CommonParam::testMinMaxIncVect(Types::Type type, T min, T max, Y vect) {
      string typeStr = Types::getInstance().convert(type);
      cout << "Testing " << typeStr << " ";
      cout << "sizeof[bits]: " << 8 * sizeof (min) << " ";
      if (typeStr != "VECTOR_INT8") {
        cout << "minInc=" << min << " maxInc=" << max;
      }
      cout << endl;
      assert(sizeof (min) == sizeof (vect[0]));
      assert(min <= max);

      Schema element, expected;
      element.clear();
      element.description("a");
      element.key("a");
      element.displayedName("a");
      element.simpleType(type);
      element.minInc(min);
      element.maxInc(max);
      element.assignment(Schema::OPTIONAL_PARAM);
      expected.addElement(element);
      TestParam::define(expected);

      Schema c;
      c.setFromPath("TP.a", vect);
      TestParam::create(c);

      return 0;
    }

    template<typename T, typename Y> int CommonParam::testMinMaxIncVectError(Types::Type type, T min, T max, Y vect) {
      string typeStr = Types::getInstance().convert(type);
      cout << "Testing " << typeStr << " ";
      cout << "sizeof[bits]: " << 8 * sizeof (min) << " ";
      if (typeStr != "VECTOR_INT8") {
        cout << "minInc=" << min << " maxInc=" << max;
      }
      cout << endl;
      assert(sizeof (min) == sizeof (vect[0]));
      assert(min <= max);

      Schema element, expected;
      element.clear();
      element.description("a");
      element.key("a");
      element.displayedName("a");
      element.simpleType(type);
      element.minInc(min);
      element.maxInc(max);
      element.assignment(Schema::OPTIONAL_PARAM);
      expected.addElement(element);
      TestParam::define(expected);

      try {
        Schema c;
        c.setFromPath("TP.a", vect);
        TestParam::create(c);
        cout << "Not all vector elements are in the range minInc, maxInc, but this has not caused ParameterException" << endl;
        assert(false);
      } catch (ParameterException e) {
      }

      return 0;
    }

    template<typename T, typename Y> int CommonParam::testMinMaxExcVect(Types::Type type, T min, T max, Y vect) {
      string typeStr = Types::getInstance().convert(type);
      cout << "Testing " << typeStr << " ";
      cout << "sizeof[bits]: " << 8 * sizeof (min) << " ";
      if (typeStr != "VECTOR_INT8") {
        cout << "minExc=" << min << " maxExc=" << max;
      }
      cout << endl;
      assert(sizeof (min) == sizeof (vect[0]));
      assert(min < max);

      Schema element, expected;
      element.clear();
      element.description("a");
      element.key("a");
      element.displayedName("a");
      element.simpleType(type);
      element.minExc(min);
      element.maxExc(max);
      element.assignment(Schema::OPTIONAL_PARAM);
      expected.addElement(element);
      TestParam::define(expected);

      Schema c;
      c.setFromPath("TP.a", vect);
      TestParam::create(c);

      return 0;
    }

    template<typename T, typename Y> int CommonParam::testMinMaxExcVectError(Types::Type type, T min, T max, Y vect) {
      string typeStr = Types::getInstance().convert(type);
      cout << "Testing " << typeStr << " ";
      cout << "sizeof[bits]: " << 8 * sizeof (min) << " ";
      if (typeStr != "VECTOR_INT8") {
        cout << "minExc=" << min << " maxExc=" << max;
      }
      cout << endl;
      assert(sizeof (min) == sizeof (vect[0]));
      assert(min < max);

      Schema element, expected;
      element.clear();
      element.description("a");
      element.key("a");
      element.displayedName("a");
      element.simpleType(type);
      element.minExc(min);
      element.maxExc(max);
      element.assignment(Schema::OPTIONAL_PARAM);
      expected.addElement(element);
      TestParam::define(expected);

      try {
        Schema c;
        c.setFromPath("TP.a", vect);
        TestParam::create(c);
        cout << "Not all vector elements are in the range minExc, maxExc, but this has not caused ParameterException" << endl;
        assert(false);
      } catch (ParameterException e) {
      }

      return 0;
    }

    //checking parameter 'options'

    template<typename T> int CommonParam::testOptions(Types::Type type, std::string& opt, T val) {
      string typeStr = Types::getInstance().convert(type);
      cout << typeStr << " sizeof[bits]: " << 8 * sizeof (val) << " val=" << val << ", options=\"" << opt << "\"" << endl;
      Schema element, expected;

      element.clear();
      element.description("a");
      element.key("a");
      element.displayedName("a");
      element.simpleType(type);
      element.options(opt);
      element.assignment(Schema::OPTIONAL_PARAM);
      expected.addElement(element);
      TestParam::define(expected);

      Schema c;
      c.setFromPath("TP.a", val);
      TestParam::create(c);

      return 0;

    }

    template<typename T> int CommonParam::testOptionsError(Types::Type type, std::string& opt, T val) {
      string typeStr = Types::getInstance().convert(type);
      cout << typeStr << " sizeof[bits]: " << 8 * sizeof (val) << " val=" << val << ", options=\"" << opt << "\"" << endl;
      Schema element, expected;

      element.clear();
      element.description("a");
      element.key("a");
      element.displayedName("a");
      element.simpleType(type);
      element.options(opt);
      element.assignment(Schema::OPTIONAL_PARAM);
      expected.addElement(element);
      TestParam::define(expected);

      try {
        Schema c;
        c.setFromPath("TP.a", val);
        TestParam::create(c);
        cout << "Value is not listed in options, but this has not caused ParameterException" << endl;
        assert(false);
      } catch (ParameterException e) {
      }

      return 0;

    }

    int CommonParam::testAccess(AccessType access) {

      Schema element, expected;

      element.clear();
      element.description("a");
      element.key("a");
      element.displayedName("a");
      element.simpleType(Types::INT32);
      //int a = INIT|exfel::util::READ|WRITE;
      //cout << "access:" << a << endl;
      element.access(access);
      //element.access();
      element.assignment(Schema::OPTIONAL_PARAM);
      expected.addElement(element);
      TestParam::define(expected);
      return 0;

    }

  } //namespace util
} //namespace exfel

#endif	/* EXFEL_UTIL_COMMONPARAM_HH */

