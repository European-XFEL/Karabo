/*
 * $Id: Types.hh 4978 2012-01-18 10:43:06Z wegerk@DESY.DE $
 *
 * File:   Types.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on July 13, 2010, 6:55 PM
 *
 * Copyright (C) 2010 European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_UTIL_TYPES_HH
#define	KARABO_UTIL_TYPES_HH

#include <string>
#include <map>
#include <vector>
#include <typeinfo>
#include <complex>
#include <deque>
#include <boost/tuple/tuple.hpp>

#include "utildll.hh"


// Forward the boost::filesystem::path
namespace boost {
    namespace filesystem {
        class path;
    }
}

namespace karabo {
    namespace util {

        class Schema;

        /**
         * Singleton Class for Type Description
         * 
         * TODO (for next library cleaning day)
         * 
         * 1 - Make all Types functions static (use getInstance privately)
         * 2 - Make the API more orthogonal i.e. get rid of convert
         * 3 - Use the FORMAT enum consistently either as template or function argument
         * 
         * BH will do this
         *      
         */
        class DECLSPEC_UTIL Types {
        public:

            struct Any {
            };

            enum Type {
                BOOL, // bool
                CHAR, // char
                INT8, // signed char
                INT16, // signed short
                INT32, // signed int
                INT64, // signed long long
                UINT8, // unsigned char
                UINT16, // unsigned short
                UINT32, // unsigned int
                UINT64, // unsigned long long
                FLOAT, // float
                COMPLEX_FLOAT, // complex<float>
                DOUBLE, // double
                COMPLEX_DOUBLE, // complex<double>
                STRING, // std::string
                CONST_CHAR_PTR, // const char*
                VECTOR_STRING, // std::vector<std::string>
                VECTOR_PATH, // std::vector<boost::filesystem::path>
                VECTOR_CHAR, // std::vector<char>
                VECTOR_INT8, // std::vector<std::signed char>
                VECTOR_INT16, // std::vector<std::signed short>
                VECTOR_INT32, // std::vector<std::int>
                VECTOR_INT64, // std::vector<std::signed long long>
                VECTOR_UINT8, // std::vector<std::unsigned char>
                VECTOR_UINT16, // std::vector<std::unsigned short>
                VECTOR_UINT32, // std::vector<std::unsigned int>
                VECTOR_UINT64, // std::vector<std::unsigned long long>
                VECTOR_BOOL, // std::deque<std::bool>
                VECTOR_DOUBLE, // std::vector<std::double>
                VECTOR_FLOAT, // std::vector<std::float>
                OCCURANCE_TYPE, // Schema::OccuanceType
                ASSIGNMENT_TYPE, // Schema::AssignmentType
                DATA_TYPE, // Types::Type
                EXPERT_LEVEL_TYPE, // Schema::ExpertLevelType
                ACCESS_TYPE, // AccessType
                PATH, // boost::filesystem::path
                HASH, // Hash
                SCHEMA, // Schema
                VECTOR_HASH, // std::vector<Hash>
                ANY, // unspecified type
                UNKNOWN
            };

            enum Format {
                FORMAT_INTERN = 1,
                FORMAT_XSD = 2,
                FORMAT_CPP = 3
            };

            /**
             * Singleton creation
             */
            static Types& getInstance();

            /**
             * Type retrieval given RTTI type_info object
             * @param typeInfo
             * @return Enumerated Types::TYPE
             */
            Type getTypeAsId(const std::type_info& typeInfo) const;

            /**
             * Type retrieval given RTTI type_info object
             * @param typeInfo
             * @return std::string identifying the data type
             */
            std::string getTypeAsString(const std::type_info& typeInfo) const;

            /**
             * Type retrieval given RTTI type_info object
             * @param typeInfo
             * @return std::string identifying the XSD data type
             */
            std::string getTypeAsStringXsd(const std::type_info& typeInfo) const;

            /**
             * TODO This can be heavily improved in performance, as all code could be generated at compile-time already
             * @return
             */
            template <class T, int format>
            static std::string getTypeAsString(const T& var = T()) {
                getInstance();
                std::string typeidName = std::string(typeid (var).name());
                ConstTypeIt it = m_typeMap.find(typeidName);
                if (it != m_typeMap.end()) {
                    return it->second.get < format > ();
                } else {
                    return "UNKNOWN";
                }
            }

            template<class T>
            static Type getTypeAsId(const T& var = T()) {
                // This function does not compile by purpose. There are template specializations for each allowed data type
                // (see template specialization declaration at the bottom of this file and corresponding definitions in Types.cc)
                // Any attempt to use not supported type will result in the compilation error.
                // This concept is used i.e. in SimpleElement
                return this_type_is_not_supported_by_purpose(var);
            }


            /**
             * Given the enumerated type description this returns the string version
             */
            static std::string convert(const Type type);

            /**
             * Given the string type description this returns the enumerated version
             */
            static Types::Type convert(const std::string& type);

            /**
             * Given the enumerated type description this returns the XSD-string version
             */
            static std::string convertToXsd(const Type type);

            /**
             * Given the XSD-string type description this returns the enumerated version
             */
            static Types::Type convertFromXsd(const std::string& type);

            /**
             * This function retrieves all registered types as array
             */
            std::vector<std::string> getTypeListAsStrings() const;

            /**
             * This function retrieves all registered types as array
             */
            std::vector<Type> getTypeListAsIds() const;

            /**
             * Type retrieval given RTTI type_info object
             * @param typeInfo
             * @return A std::pair containing both, enumerated and stringified type information
             */
            std::pair<Type, std::string> getType(const std::type_info& typeInfo) const;

            static void reRegisterTypes();

        private:

#define REGISTER_TYPE(t,e,x) m_typeMap[std::string(typeid(t).name())] = boost::tuple<Type, std::string, std::string, std::string>(e,std::string(#e),std::string(x),std::string(#t))
            Types();

            Types(const Types&);
            Types & operator=(Types&);

            static void registerTypes();

            typedef std::map<const std::string, boost::tuple<Type, std::string, std::string, std::string > > TypeMap;
            typedef TypeMap::const_iterator ConstTypeIt;

            static TypeMap m_typeMap;

        };

        // bool
        template<> DECLSPEC_UTIL Types::Type Types::getTypeAsId(const bool&);
        template<> DECLSPEC_UTIL Types::Type Types::getTypeAsId(const std::deque<bool>&);

        // char
        template<> DECLSPEC_UTIL Types::Type Types::getTypeAsId(const unsigned char&);
        template<> DECLSPEC_UTIL Types::Type Types::getTypeAsId(const char&);
        template<> DECLSPEC_UTIL Types::Type Types::getTypeAsId(const signed char&);
        template<> DECLSPEC_UTIL Types::Type Types::getTypeAsId(const std::vector<unsigned char>&);
        template<> DECLSPEC_UTIL Types::Type Types::getTypeAsId(const std::vector<char>&);
        template<> DECLSPEC_UTIL Types::Type Types::getTypeAsId(const std::vector<signed char>&);

        // short
        template<> DECLSPEC_UTIL Types::Type Types::getTypeAsId(const unsigned short&);
        template<> DECLSPEC_UTIL Types::Type Types::getTypeAsId(const signed short&);
        template<> DECLSPEC_UTIL Types::Type Types::getTypeAsId(const std::vector<unsigned short>&);
        template<> DECLSPEC_UTIL Types::Type Types::getTypeAsId(const std::vector<signed short>&);

        // int
        template<> DECLSPEC_UTIL Types::Type Types::getTypeAsId(const unsigned int&);
        template<> DECLSPEC_UTIL Types::Type Types::getTypeAsId(const signed int&);
        template<> DECLSPEC_UTIL Types::Type Types::getTypeAsId(const std::vector<unsigned int>&);
        template<> DECLSPEC_UTIL Types::Type Types::getTypeAsId(const std::vector<signed int>&);

        // long long
        template<> DECLSPEC_UTIL Types::Type Types::getTypeAsId(const unsigned long long&);
        template<> DECLSPEC_UTIL Types::Type Types::getTypeAsId(const signed long long&);
        template<> DECLSPEC_UTIL Types::Type Types::getTypeAsId(const std::vector<unsigned long long>&);
        template<> DECLSPEC_UTIL Types::Type Types::getTypeAsId(const std::vector<signed long long>&);

        // float
        template<> DECLSPEC_UTIL Types::Type Types::getTypeAsId(const float&);
        template<> DECLSPEC_UTIL Types::Type Types::getTypeAsId(const std::vector<float>&);

        //double
        template<> DECLSPEC_UTIL Types::Type Types::getTypeAsId(const double&);
        template<> DECLSPEC_UTIL Types::Type Types::getTypeAsId(const std::vector<double>&);

        // string
        template<> DECLSPEC_UTIL Types::Type Types::getTypeAsId(const std::string&);
        template<> DECLSPEC_UTIL Types::Type Types::getTypeAsId(const std::vector<std::string>&);

        // path
        template<> DECLSPEC_UTIL Types::Type Types::getTypeAsId(const boost::filesystem::path&);
        template<> DECLSPEC_UTIL Types::Type Types::getTypeAsId(const std::vector<boost::filesystem::path>&);

    } // namespace util

    // Convenience
    typedef karabo::util::Types::Format TypeFormat;

} // namespace karabo

#endif	/* KARABO_UTIL_TYPES_HH */

