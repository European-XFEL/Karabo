/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/python.hpp>
#include <boost/filesystem.hpp>

#include <karabo/util/Types.hh>

using namespace karabo::util;
namespace bp = boost::python;

void exportPyUtilTypes() { //exposing ::karabo::util::Types
  typedef bp::class_< Types, boost::noncopyable > Types_exposer_t;
  Types_exposer_t Types_exposer = Types_exposer_t("Types", bp::no_init);
  bp::scope Types_scope(Types_exposer);
  bp::enum_< Types::Format > ("Format")
          .value("FORMAT_INTERN", Types::FORMAT_INTERN)
          .value("FORMAT_XSD", Types::FORMAT_XSD)
          .value("FORMAT_CPP", Types::FORMAT_CPP)
          .export_values()
          ;
  bp::enum_< Types::ReferenceType > ("Type")
          .value("BOOL", Types::BOOL)
          .value("INT8", Types::INT8)
          .value("INT16", Types::INT16)
          .value("INT32", Types::INT32)
          .value("INT64", Types::INT64)
          .value("UINT8", Types::UINT8)
          .value("UINT16", Types::UINT16)
          .value("UINT32", Types::UINT32)
          .value("UINT64", Types::UINT64)
          .value("CHAR", Types::CHAR)
          .value("FLOAT", Types::FLOAT)
          .value("COMPLEX_FLOAT", Types::COMPLEX_FLOAT)
          .value("DOUBLE", Types::DOUBLE)
          .value("COMPLEX_DOUBLE", Types::COMPLEX_DOUBLE)
          .value("STRING", Types::STRING)
          .value("CONST_CHAR_PTR", Types::CONST_CHAR_PTR)
          .value("VECTOR_STRING", Types::VECTOR_STRING)
          .value("VECTOR_INT8", Types::VECTOR_INT8)
          .value("VECTOR_INT16", Types::VECTOR_INT16)
          .value("VECTOR_INT32", Types::VECTOR_INT32)
          .value("VECTOR_INT64", Types::VECTOR_INT64)
          .value("VECTOR_UINT8", Types::VECTOR_UINT8)
          .value("VECTOR_UINT16", Types::VECTOR_UINT16)
          .value("VECTOR_UINT32", Types::VECTOR_UINT32)
          .value("VECTOR_UINT64", Types::VECTOR_UINT64)
          .value("VECTOR_CHAR", Types::VECTOR_CHAR)
          .value("VECTOR_BOOL", Types::VECTOR_BOOL)
          .value("VECTOR_DOUBLE", Types::VECTOR_DOUBLE)
          .value("VECTOR_FLOAT", Types::VECTOR_FLOAT)
          .value("OCCURANCE_TYPE", Types::OCCURANCE_TYPE)
          .value("ASSIGNMENT_TYPE", Types::ASSIGNMENT_TYPE)
          .value("DATA_TYPE", Types::DATA_TYPE)
          .value("EXPERT_LEVEL_TYPE", Types::EXPERT_LEVEL_TYPE)
          .value("ACCESS_TYPE", Types::ACCESS_TYPE)
          .value("PATH", Types::PATH)
          .value("HASH", Types::HASH)
          .value("SCHEMA", Types::SCHEMA)
          .value("VECTOR_HASH", Types::VECTOR_HASH)
          .value("UNKNOWN", Types::UNKNOWN)
          .export_values()
          ;
  { //::karabo::util::Types::convert

    typedef ::std::string(*convert_function_type)(::karabo::util::Types::ReferenceType const);

    Types_exposer.def(
            "convert"
            , convert_function_type(&::karabo::util::Types::convert)
            , (bp::arg("type")));

  }
  { //::karabo::util::Types::convert

    typedef ::karabo::util::Types::ReferenceType(*convert_function_type)(::std::string const &);

    Types_exposer.def(
            "convert"
            , convert_function_type(&::karabo::util::Types::convert)
            , (bp::arg("type")));

  }
  { //::karabo::util::Types::convertFromXsd

    typedef ::karabo::util::Types::ReferenceType(*convertFromXsd_function_type)(::std::string const &);

    Types_exposer.def(
            "convertFromXsd"
            , convertFromXsd_function_type(&::karabo::util::Types::convertFromXsd)
            , (bp::arg("type")));

  }
  { //::karabo::util::Types::convertToXsd

    typedef ::std::string(*convertToXsd_function_type)(::karabo::util::Types::ReferenceType const);

    Types_exposer.def(
            "convertToXsd"
            , convertToXsd_function_type(&::karabo::util::Types::convertToXsd)
            , (bp::arg("type")));

  }
  { //::karabo::util::Types::getType

    typedef ::std::pair< karabo::util::Types::ReferenceType, std::basic_string< char, std::char_traits< char >, std::allocator< char > > > (::karabo::util::Types::*getType_function_type)(::std::type_info const &) const;

    Types_exposer.def(
            "getType"
            , getType_function_type(&::karabo::util::Types::getType)
            , (bp::arg("typeInfo")));

  }
  { //::karabo::util::Types::getTypeAsId

    typedef ::karabo::util::Types::ReferenceType(::karabo::util::Types::*getTypeAsId_function_type)(::std::type_info const &) const;

    Types_exposer.def(
            "getTypeAsId"
            , getTypeAsId_function_type(&::karabo::util::Types::getTypeAsId)
            , (bp::arg("typeInfo")));

  }
  { //::karabo::util::Types::getTypeAsId

    typedef ::karabo::util::Types::ReferenceType(*getTypeAsId_function_type)(::std::vector< std::string > const &);

    Types_exposer.def(
            "getTypeAsId"
            , getTypeAsId_function_type(&::karabo::util::Types::getTypeAsId)
            , (bp::arg("var")));

  }
  { //::karabo::util::Types::getTypeAsId

    typedef ::karabo::util::Types::ReferenceType(*getTypeAsId_function_type)(::std::vector< double > const &);

    Types_exposer.def(
            "getTypeAsId"
            , getTypeAsId_function_type(&::karabo::util::Types::getTypeAsId)
            , (bp::arg("var")));

  }
  { //::karabo::util::Types::getTypeAsId

    typedef ::karabo::util::Types::ReferenceType(*getTypeAsId_function_type)(::std::vector< float > const &);

    Types_exposer.def(
            "getTypeAsId"
            , getTypeAsId_function_type(&::karabo::util::Types::getTypeAsId)
            , (bp::arg("var")));

  }
  { //::karabo::util::Types::getTypeAsId

    typedef ::karabo::util::Types::ReferenceType(*getTypeAsId_function_type)(::std::vector< unsigned long long > const &);

    Types_exposer.def(
            "getTypeAsId"
            , getTypeAsId_function_type(&::karabo::util::Types::getTypeAsId)
            , (bp::arg("var")));

  }
  { //::karabo::util::Types::getTypeAsId

    typedef ::karabo::util::Types::ReferenceType(*getTypeAsId_function_type)(::std::vector< unsigned int > const &);

    Types_exposer.def(
            "getTypeAsId"
            , getTypeAsId_function_type(&::karabo::util::Types::getTypeAsId)
            , (bp::arg("var")));

  }
  { //::karabo::util::Types::getTypeAsId

    typedef ::karabo::util::Types::ReferenceType(*getTypeAsId_function_type)(::std::vector< unsigned short > const &);

    Types_exposer.def(
            "getTypeAsId"
            , getTypeAsId_function_type(&::karabo::util::Types::getTypeAsId)
            , (bp::arg("var")));

  }
  { //::karabo::util::Types::getTypeAsId

    typedef ::karabo::util::Types::ReferenceType(*getTypeAsId_function_type)(::std::vector< unsigned char > const &);

    Types_exposer.def(
            "getTypeAsId"
            , getTypeAsId_function_type(&::karabo::util::Types::getTypeAsId)
            , (bp::arg("var")));

  }
  { //::karabo::util::Types::getTypeAsId

    typedef ::karabo::util::Types::ReferenceType(*getTypeAsId_function_type)(::std::vector< signed long long > const &);

    Types_exposer.def(
            "getTypeAsId"
            , getTypeAsId_function_type(&::karabo::util::Types::getTypeAsId)
            , (bp::arg("var")));

  }
  { //::karabo::util::Types::getTypeAsId

    typedef ::karabo::util::Types::ReferenceType(*getTypeAsId_function_type)(::std::vector< int > const &);

    Types_exposer.def(
            "getTypeAsId"
            , getTypeAsId_function_type(&::karabo::util::Types::getTypeAsId)
            , (bp::arg("var")));

  }
  { //::karabo::util::Types::getTypeAsId

    typedef ::karabo::util::Types::ReferenceType(*getTypeAsId_function_type)(::std::vector< signed short > const &);

    Types_exposer.def(
            "getTypeAsId"
            , getTypeAsId_function_type(&::karabo::util::Types::getTypeAsId)
            , (bp::arg("var")));

  }
  { //::karabo::util::Types::getTypeAsId

    typedef ::karabo::util::Types::ReferenceType(*getTypeAsId_function_type)(::std::vector< signed char > const &);

    Types_exposer.def(
            "getTypeAsId"
            , getTypeAsId_function_type(&::karabo::util::Types::getTypeAsId)
            , (bp::arg("var")));

  }
  { //::karabo::util::Types::getTypeAsId

    typedef ::karabo::util::Types::ReferenceType(*getTypeAsId_function_type)(::std::vector< char > const &);

    Types_exposer.def(
            "getTypeAsId"
            , getTypeAsId_function_type(&::karabo::util::Types::getTypeAsId)
            , (bp::arg("var")));

  }
  { //::karabo::util::Types::getTypeAsId

    typedef ::karabo::util::Types::ReferenceType(*getTypeAsId_function_type)(::std::deque< bool > const &);

    Types_exposer.def(
            "getTypeAsId"
            , getTypeAsId_function_type(&::karabo::util::Types::getTypeAsId)
            , (bp::arg("var")));

  }
  { //::karabo::util::Types::getTypeAsId

    typedef ::karabo::util::Types::ReferenceType(*getTypeAsId_function_type)(::boost::filesystem::path const &);

    Types_exposer.def(
            "getTypeAsId"
            , getTypeAsId_function_type(&::karabo::util::Types::getTypeAsId)
            , (bp::arg("var")));

  }
  { //::karabo::util::Types::getTypeAsId

    typedef ::karabo::util::Types::ReferenceType(*getTypeAsId_function_type)(::std::string const &);

    Types_exposer.def(
            "getTypeAsId"
            , getTypeAsId_function_type(&::karabo::util::Types::getTypeAsId)
            , (bp::arg("var")));

  }
  { //::karabo::util::Types::getTypeAsId

    typedef ::karabo::util::Types::ReferenceType(*getTypeAsId_function_type)(double const &);

    Types_exposer.def(
            "getTypeAsId"
            , getTypeAsId_function_type(&::karabo::util::Types::getTypeAsId)
            , (bp::arg("var")));

  }
  { //::karabo::util::Types::getTypeAsId

    typedef ::karabo::util::Types::ReferenceType(*getTypeAsId_function_type)(float const &);

    Types_exposer.def(
            "getTypeAsId"
            , getTypeAsId_function_type(&::karabo::util::Types::getTypeAsId)
            , (bp::arg("var")));

  }
  { //::karabo::util::Types::getTypeAsId

    typedef ::karabo::util::Types::ReferenceType(*getTypeAsId_function_type)(unsigned long long const &);

    Types_exposer.def(
            "getTypeAsId"
            , getTypeAsId_function_type(&::karabo::util::Types::getTypeAsId)
            , (bp::arg("var")));

  }
  { //::karabo::util::Types::getTypeAsId

    typedef ::karabo::util::Types::ReferenceType(*getTypeAsId_function_type)(unsigned int const &);

    Types_exposer.def(
            "getTypeAsId"
            , getTypeAsId_function_type(&::karabo::util::Types::getTypeAsId)
            , (bp::arg("var")));

  }
  { //::karabo::util::Types::getTypeAsId

    typedef ::karabo::util::Types::ReferenceType(*getTypeAsId_function_type)(unsigned short const &);

    Types_exposer.def(
            "getTypeAsId"
            , getTypeAsId_function_type(&::karabo::util::Types::getTypeAsId)
            , (bp::arg("var")));

  }
  { //::karabo::util::Types::getTypeAsId

    typedef ::karabo::util::Types::ReferenceType(*getTypeAsId_function_type)(unsigned char const &);

    Types_exposer.def(
            "getTypeAsId"
            , getTypeAsId_function_type(&::karabo::util::Types::getTypeAsId)
            , (bp::arg("var")));

  }
  { //::karabo::util::Types::getTypeAsId

    typedef ::karabo::util::Types::ReferenceType(*getTypeAsId_function_type)(signed long long const &);

    Types_exposer.def(
            "getTypeAsId"
            , getTypeAsId_function_type(&::karabo::util::Types::getTypeAsId)
            , (bp::arg("var")));

  }
  { //::karabo::util::Types::getTypeAsId

    typedef ::karabo::util::Types::ReferenceType(*getTypeAsId_function_type)(int const &);

    Types_exposer.def(
            "getTypeAsId"
            , getTypeAsId_function_type(&::karabo::util::Types::getTypeAsId)
            , (bp::arg("var")));

  }
  { //::karabo::util::Types::getTypeAsId

    typedef ::karabo::util::Types::ReferenceType(*getTypeAsId_function_type)(signed short const &);

    Types_exposer.def(
            "getTypeAsId"
            , getTypeAsId_function_type(&::karabo::util::Types::getTypeAsId)
            , (bp::arg("var")));

  }
  { //::karabo::util::Types::getTypeAsId

    typedef ::karabo::util::Types::ReferenceType(*getTypeAsId_function_type)(signed char const &);

    Types_exposer.def(
            "getTypeAsId"
            , getTypeAsId_function_type(&::karabo::util::Types::getTypeAsId)
            , (bp::arg("var")));

  }
  { //::karabo::util::Types::getTypeAsId

    typedef ::karabo::util::Types::ReferenceType(*getTypeAsId_function_type)(char const &);

    Types_exposer.def(
            "getTypeAsId"
            , getTypeAsId_function_type(&::karabo::util::Types::getTypeAsId)
            , (bp::arg("var")));

  }
  { //::karabo::util::Types::getTypeAsId

    typedef ::karabo::util::Types::ReferenceType(*getTypeAsId_function_type)(bool const &);

    Types_exposer.def(
            "getTypeAsId"
            , getTypeAsId_function_type(&::karabo::util::Types::getTypeAsId)
            , (bp::arg("var")));

  }
  { //::karabo::util::Types::getTypeAsString

    typedef ::std::string(::karabo::util::Types::*getTypeAsString_function_type)(::std::type_info const &) const;

    Types_exposer.def(
            "getTypeAsString"
            , getTypeAsString_function_type(&::karabo::util::Types::getTypeAsString)
            , (bp::arg("typeInfo")));

  }
  { //::karabo::util::Types::getTypeAsStringXsd

    typedef ::std::string(::karabo::util::Types::*getTypeAsStringXsd_function_type)(::std::type_info const &) const;

    Types_exposer.def(
            "getTypeAsStringXsd"
            , getTypeAsStringXsd_function_type(&::karabo::util::Types::getTypeAsStringXsd)
            , (bp::arg("typeInfo")));

  }

  Types_exposer.staticmethod("convert");
  Types_exposer.staticmethod("convertFromXsd");
  Types_exposer.staticmethod("convertToXsd");
  Types_exposer.staticmethod("getTypeAsId");
  Types_exposer.staticmethod("getTypeAsString");

}

