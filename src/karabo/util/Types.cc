/*
 * $Id: Types.cc 4711 2011-11-14 15:01:26Z irinak@DESY.DE $
 *
 * File:   Types.cc
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on July 14, 2010, 4:11 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/filesystem.hpp>

#include "Types.hh"
#include "Schema.hh"

using namespace std;

namespace exfel {
  namespace util {

    // Instantiation of static member
    Types::TypeMap Types::m_typeMap = Types::TypeMap();

    template<> Types::Type  Types::getTypeAsId(const bool&) {
      return BOOL;
    }

    template<> Types::Type  Types::getTypeAsId(const signed char&) {
      return INT8;
    }
    
    template<> Types::Type  Types::getTypeAsId(const char&) {
      return CHAR;
    }

    template<> Types::Type  Types::getTypeAsId(const signed short&) {
      return INT16;
    }

    template<> Types::Type  Types::getTypeAsId(const signed int&) {
      return INT32;
    }

    template<> Types::Type  Types::getTypeAsId(const long long&) {
      return INT64;
    }

    template<> Types::Type  Types::getTypeAsId(const unsigned char&) {
      return UINT8;
    }

    template<> Types::Type  Types::getTypeAsId(const unsigned short&) {
      return UINT16;
    }

    template<> Types::Type  Types::getTypeAsId(const unsigned int&) {
      return UINT32;
    }

    template<> Types::Type  Types::getTypeAsId(const unsigned long long&) {
      return UINT64;
    }

    template<> Types::Type  Types::getTypeAsId(const std::string&) {
      return STRING;
    }

    template<> Types::Type Types::getTypeAsId<float>(const float&) {
      return FLOAT;
    }

    template<> Types::Type  Types::getTypeAsId(const double&) {
      return DOUBLE;
    }

    template<> Types::Type  Types::getTypeAsId(const boost::filesystem::path&) {
      return PATH;
    }

    template<> Types::Type  Types::getTypeAsId(const deque<bool>&) {
      return VECTOR_BOOL;
    }

    template<> Types::Type  Types::getTypeAsId(const vector<signed char>&) {
      return VECTOR_INT8;
    }
    
    template<> Types::Type  Types::getTypeAsId(const vector<char>&) {
      return VECTOR_CHAR;
    }

    template<> Types::Type  Types::getTypeAsId(const vector<signed short>&) {
      return VECTOR_INT16;
    }

    template<> Types::Type  Types::getTypeAsId(const vector<signed int>&) {
      return VECTOR_INT32;
    }

    template<> Types::Type  Types::getTypeAsId(const vector<signed long long>&) {
      return VECTOR_INT64;
    }

    template<> Types::Type  Types::getTypeAsId(const vector<unsigned char>&) {
      return VECTOR_UINT8;
    }

    template<> Types::Type  Types::getTypeAsId(const vector<unsigned short>&) {
      return VECTOR_UINT16;
    }

    template<> Types::Type  Types::getTypeAsId(const vector<unsigned int>&) {
      return VECTOR_UINT32;
    }

    template<> Types::Type  Types::getTypeAsId(const vector<unsigned long long>&) {
      return VECTOR_UINT64;
    }

    template<> Types::Type  Types::getTypeAsId(const vector<std::string>&) {
      return VECTOR_STRING;
    }

    template<> Types::Type  Types::getTypeAsId(const vector<float>&) {
      return VECTOR_FLOAT;
    }

    template<> Types::Type  Types::getTypeAsId(const vector<double>&) {
      return VECTOR_DOUBLE;
    }

   template<> Types::Type  Types::getTypeAsId(const vector<boost::filesystem::path>&) {
      return VECTOR_PATH;
   }
   
    Types::Types() {
      registerTypes();
    }
    
    void Types::reRegisterTypes() {
      m_typeMap.clear();
      registerTypes();
    }
    
    void Types::registerTypes() {
      REGISTER_TYPE(bool, BOOL, "xs:boolean");
      REGISTER_TYPE(char, CHAR, "xs:byte");
      REGISTER_TYPE(signed char, INT8, "xs:byte");
      REGISTER_TYPE(signed short, INT16, "xs:byte");
      REGISTER_TYPE(signed int, INT32, "xs:int");
      REGISTER_TYPE(signed long long, INT64, "xs:long");
      REGISTER_TYPE(unsigned char, UINT8, "xs:unsignedByte");
      REGISTER_TYPE(unsigned short, UINT16, "xs:unsignedShort");
      REGISTER_TYPE(unsigned int, UINT32, "xs:unsignedInt");
      REGISTER_TYPE(unsigned long long, UINT64, "xs:unsignedLong");
      REGISTER_TYPE(complex<float>, COMPLEX_FLOAT, "undefined");
      REGISTER_TYPE(complex<double>, COMPLEX_DOUBLE, "undefined");
      REGISTER_TYPE(float, FLOAT, "xs:float");
      REGISTER_TYPE(double, DOUBLE, "xs:double");
      REGISTER_TYPE(string, STRING, "xs:string");
      REGISTER_TYPE(const char*, CONST_CHAR_PTR, "xs:string");
      REGISTER_TYPE(Schema::OccuranceType, OCCURANCE_TYPE, "undefined");
      REGISTER_TYPE(Schema::AssignmentType, ASSIGNMENT_TYPE, "undefined");
      REGISTER_TYPE(Schema::ExpertLevelType, EXPERT_LEVEL_TYPE, "undefined");
      REGISTER_TYPE(AccessType, ACCESS_TYPE, "undefined");
      REGISTER_TYPE(Types::Type, DATA_TYPE, "undefined");
      REGISTER_TYPE(boost::filesystem::path, PATH, "xs:anyURI");
      REGISTER_TYPE(Hash, HASH, "undefined");
      REGISTER_TYPE(Schema, SCHEMA, "undefined");
      REGISTER_TYPE(vector<string>, VECTOR_STRING, "xs:string");
      REGISTER_TYPE(vector<boost::filesystem::path>, VECTOR_PATH, "xs:anyURI");
      REGISTER_TYPE(vector<char>, VECTOR_CHAR, "xs:byte");
      REGISTER_TYPE(vector<signed char>, VECTOR_INT8, "xs:byte");
      REGISTER_TYPE(vector<signed short>, VECTOR_INT16, "xs:short");
      REGISTER_TYPE(vector<signed int>, VECTOR_INT32, "xs:int");
      REGISTER_TYPE(vector<signed long long>, VECTOR_INT64, "xs:long");
      REGISTER_TYPE(vector<unsigned char>, VECTOR_UINT8, "xs:unsignedByte");
      REGISTER_TYPE(vector<unsigned short>, VECTOR_UINT16, "xs:unsignedShort");
      REGISTER_TYPE(vector<unsigned int>, VECTOR_UINT32, "xs:unsignedInt");
      REGISTER_TYPE(vector<unsigned long long>, VECTOR_UINT64, "xs:unsignedLong");
      REGISTER_TYPE(deque<bool>, VECTOR_BOOL, "xs:boolean");
      REGISTER_TYPE(vector<double>, VECTOR_DOUBLE, "xs:double");
      REGISTER_TYPE(vector<float>, VECTOR_FLOAT, "xs:float");
      REGISTER_TYPE(vector<Hash>, VECTOR_HASH, "undefined");
    }

    Types& Types::getInstance() {
      static Types types;
      return types;
    }

    Types::Type Types::getTypeAsId(const type_info& typeInfo) const {
      ConstTypeIt it = m_typeMap.find(typeInfo.name());
      if (it != m_typeMap.end()) {
        return it->second.get < 0 > ();
      } else {
        return UNKNOWN;
      }
    }

    string Types::getTypeAsString(const type_info& typeInfo) const {
      ConstTypeIt it = m_typeMap.find(typeInfo.name());
      if (it != m_typeMap.end()) {
        return it->second.get < 1 > ();
      } else {
        return "UNKNOWN";
      }
    }

    string Types::getTypeAsStringXsd(const type_info& typeInfo) const {
      ConstTypeIt it = m_typeMap.find(typeInfo.name());
      if (it != m_typeMap.end()) {
        string typeXsd = it->second.get < 2 > ();
        if (typeXsd == "undefined") {
          string typeIntern = it->second.get < 1 > ();
          throw NOT_SUPPORTED_EXCEPTION("No corresponding xsd type exists for datatype "
                  "\"" + typeIntern + "\"");
        }
        return typeXsd;
      } else {
        return "UNKNOWN";
      }
    }

    string Types::convert(const Type type) {
      for (ConstTypeIt it = m_typeMap.begin(); it != m_typeMap.end(); it++) {
        if (it->second.get < 0 > () == type) return it->second.get < 1 > ();
      }
      return "UNKNOWN";
    }

    Types::Type Types::convert(const string& type) {
      for (ConstTypeIt it = m_typeMap.begin(); it != m_typeMap.end(); it++) {
        if (it->second.get < 1 > () == type) return it->second.get < 0 > ();
      }
      return Types::UNKNOWN;
    }

    string Types::convertToXsd(const Type type) {
      for (ConstTypeIt it = m_typeMap.begin(); it != m_typeMap.end(); it++) {
        if (it->second.get < 0 > () == type) {
          string typeXsd = it->second.get < 2 > ();
          if (typeXsd == "undefined") {
            string typeIntern = it->second.get < 1 > ();
            throw NOT_SUPPORTED_EXCEPTION("No corresponding xsd type exists for datatype "
                    "\"" + typeIntern + "\"");
          }
          return typeXsd;
        }
      }
      return "UNKNOWN";
    }

    Types::Type Types::convertFromXsd(const string& type) {
      for (ConstTypeIt it = m_typeMap.begin(); it != m_typeMap.end(); it++) {
        if (it->second.get < 2 > () == type) return it->second.get < 0 > ();
      }
      return Types::UNKNOWN;
    }

    pair<Types::Type, string> Types::getType(const type_info& typeInfo) const {
      ConstTypeIt it = m_typeMap.find(typeInfo.name());
      if (it != m_typeMap.end()) {
        return pair<Types::Type, string > (it->second.get < 0 > (), it->second.get < 1 > ());
      } else {
        return pair<Types::Type, string > (UNKNOWN, "UNKNOWN");
      }
    }

    vector<string> Types::getTypeListAsStrings() const {
      vector<string> ret(m_typeMap.size());
      for (ConstTypeIt it = m_typeMap.begin(); it != m_typeMap.end(); it++) {
        ret.push_back(it->second.get < 1 > ());
      }
      return ret;
    }
  }
}
