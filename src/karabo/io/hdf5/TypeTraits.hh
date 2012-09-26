/*
 * $Id: TypeTraits.hh 6095 2012-05-08 10:05:56Z boukhele $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_IO_TYPETRAITS_HH
#define	KARABO_IO_TYPETRAITS_HH

#include <string>
#include <hdf5/hdf5.h>
#include <hdf5/H5Cpp.h>
#include "../ArrayView.hh"

namespace karabo {
    namespace io {
        namespace hdf5 {

            struct ScalarTypeTraits {

                template<typename T> static std::string classId(const T& var = T()) {
                    // see karabo/util/Type.hh for explanation of similar case
                    return this_type_is_not_supported_by_purpose(var);
                }
            };

            template<> std::string ScalarTypeTraits::classId(const signed char& var);
            template<> std::string ScalarTypeTraits::classId(const short& var);
            template<> std::string ScalarTypeTraits::classId(const int& var);
            template<> std::string ScalarTypeTraits::classId(const long long& var);
            template<> std::string ScalarTypeTraits::classId(const unsigned char& var);
            template<> std::string ScalarTypeTraits::classId(const unsigned short& var);
            template<> std::string ScalarTypeTraits::classId(const unsigned int& var);
            template<> std::string ScalarTypeTraits::classId(const unsigned long long& var);
            template<> std::string ScalarTypeTraits::classId(const double& var);
            template<> std::string ScalarTypeTraits::classId(const float& var);
            template<> std::string ScalarTypeTraits::classId(const std::string& var);
            template<> std::string ScalarTypeTraits::classId(const bool& var);

            struct ArrayTypeTraits {

                template<typename T> static std::string classId(const T& var = T()) {
                    // see karabo/util/Type.hh for explanation of similar case
                    return this_type_is_not_supported_by_purpose(var);
                }
            };

            template<> std::string ArrayTypeTraits::classId(const signed char& var);
            template<> std::string ArrayTypeTraits::classId(const short& var);
            template<> std::string ArrayTypeTraits::classId(const int& var);
            template<> std::string ArrayTypeTraits::classId(const long long& var);
            template<> std::string ArrayTypeTraits::classId(const unsigned char& var);
            template<> std::string ArrayTypeTraits::classId(const unsigned short& var);
            template<> std::string ArrayTypeTraits::classId(const unsigned int& var);
            template<> std::string ArrayTypeTraits::classId(const unsigned long long& var);
            template<> std::string ArrayTypeTraits::classId(const double& var);
            template<> std::string ArrayTypeTraits::classId(const float& var);
            template<> std::string ArrayTypeTraits::classId(const std::string& var);
            template<> std::string ArrayTypeTraits::classId(const bool& var);


            template<> std::string ArrayTypeTraits::classId(const ArrayView< signed char>& var);
            template<> std::string ArrayTypeTraits::classId(const ArrayView< short>& var);
            template<> std::string ArrayTypeTraits::classId(const ArrayView< int>& var);
            template<> std::string ArrayTypeTraits::classId(const ArrayView< long long>& var);
            template<> std::string ArrayTypeTraits::classId(const ArrayView< unsigned char>& var);
            template<> std::string ArrayTypeTraits::classId(const ArrayView< unsigned short>& var);
            template<> std::string ArrayTypeTraits::classId(const ArrayView< unsigned int>& var);
            template<> std::string ArrayTypeTraits::classId(const ArrayView< unsigned long long>& var);
            template<> std::string ArrayTypeTraits::classId(const ArrayView< double>& var);
            template<> std::string ArrayTypeTraits::classId(const ArrayView< float>& var);
            template<> std::string ArrayTypeTraits::classId(const ArrayView< std::string>& var);
            template<> std::string ArrayTypeTraits::classId(const ArrayView< bool>& var);

            template<> std::string ArrayTypeTraits::classId(const std::vector< signed char>& var);
            template<> std::string ArrayTypeTraits::classId(const std::vector< short>& var);
            template<> std::string ArrayTypeTraits::classId(const std::vector< int>& var);
            template<> std::string ArrayTypeTraits::classId(const std::vector< long long>& var);
            template<> std::string ArrayTypeTraits::classId(const std::vector< unsigned char>& var);
            template<> std::string ArrayTypeTraits::classId(const std::vector< unsigned short>& var);
            template<> std::string ArrayTypeTraits::classId(const std::vector< unsigned int>& var);
            template<> std::string ArrayTypeTraits::classId(const std::vector< unsigned long long>& var);
            template<> std::string ArrayTypeTraits::classId(const std::vector< double>& var);
            template<> std::string ArrayTypeTraits::classId(const std::vector< float>& var);
            template<> std::string ArrayTypeTraits::classId(const std::vector< std::string>& var);
            template<> std::string ArrayTypeTraits::classId(const std::deque< bool>& var);

            class ScalarTypes {
            public:

                template <class U>
                static const H5::DataType getHdf5NativeType(const U& var = U()) {
                    return NotImplementedType(var);
                }

                template <class U>
                static const H5::DataType getHdf5StandardType(const U& var = U()) {
                    return NotImplementedType(var);
                }
            };

            template<> const H5::DataType ScalarTypes::getHdf5NativeType(const signed char&);
            template<> const H5::DataType ScalarTypes::getHdf5NativeType(const short&);
            template<> const H5::DataType ScalarTypes::getHdf5NativeType(const int&);
            template<> const H5::DataType ScalarTypes::getHdf5NativeType(const long long&);
            template<> const H5::DataType ScalarTypes::getHdf5NativeType(const unsigned char&);
            template<> const H5::DataType ScalarTypes::getHdf5NativeType(const unsigned short&);
            template<> const H5::DataType ScalarTypes::getHdf5NativeType(const unsigned int&);
            template<> const H5::DataType ScalarTypes::getHdf5NativeType(const unsigned long long&);
            template<> const H5::DataType ScalarTypes::getHdf5NativeType(const float&);
            template<> const H5::DataType ScalarTypes::getHdf5NativeType(const double&);
            template<> const H5::DataType ScalarTypes::getHdf5NativeType(const bool&);
            template<> const H5::DataType ScalarTypes::getHdf5NativeType(const std::string&);

            template<> const H5::DataType ScalarTypes::getHdf5StandardType(const signed char&);
            template<> const H5::DataType ScalarTypes::getHdf5StandardType(const short&);
            template<> const H5::DataType ScalarTypes::getHdf5StandardType(const int&);
            template<> const H5::DataType ScalarTypes::getHdf5StandardType(const long long&);
            template<> const H5::DataType ScalarTypes::getHdf5StandardType(const unsigned char&);
            template<> const H5::DataType ScalarTypes::getHdf5StandardType(const unsigned short&);
            template<> const H5::DataType ScalarTypes::getHdf5StandardType(const unsigned int&);
            template<> const H5::DataType ScalarTypes::getHdf5StandardType(const unsigned long long&);
            template<> const H5::DataType ScalarTypes::getHdf5StandardType(const float&);
            template<> const H5::DataType ScalarTypes::getHdf5StandardType(const double&);
            template<> const H5::DataType ScalarTypes::getHdf5StandardType(const bool&);
            template<> const H5::DataType ScalarTypes::getHdf5StandardType(const std::string&);

            class ArrayTypes {
            public:

                template <class U>
                static const H5::DataType getHdf5NativeType(const ArrayDimensions& dims, const U& var = U()) {
                    return NotImplementedType(var);
                }

                template <class U>
                static const H5::DataType getHdf5StandardType(const ArrayDimensions& dims, const U& var = U()) {
                    return NotImplementedType(var);
                }
            };

            template<> const H5::DataType ArrayTypes::getHdf5NativeType(const ArrayDimensions& dims, const signed char&);
            template<> const H5::DataType ArrayTypes::getHdf5NativeType(const ArrayDimensions& dims, const short&);
            template<> const H5::DataType ArrayTypes::getHdf5NativeType(const ArrayDimensions& dims, const int&);
            template<> const H5::DataType ArrayTypes::getHdf5NativeType(const ArrayDimensions& dims, const long long&);
            template<> const H5::DataType ArrayTypes::getHdf5NativeType(const ArrayDimensions& dims, const unsigned char&);
            template<> const H5::DataType ArrayTypes::getHdf5NativeType(const ArrayDimensions& dims, const unsigned short&);
            template<> const H5::DataType ArrayTypes::getHdf5NativeType(const ArrayDimensions& dims, const unsigned int&);
            template<> const H5::DataType ArrayTypes::getHdf5NativeType(const ArrayDimensions& dims, const unsigned long long&);
            template<> const H5::DataType ArrayTypes::getHdf5NativeType(const ArrayDimensions& dims, const float&);
            template<> const H5::DataType ArrayTypes::getHdf5NativeType(const ArrayDimensions& dims, const double&);
            template<> const H5::DataType ArrayTypes::getHdf5NativeType(const ArrayDimensions& dims, const bool&);
            template<> const H5::DataType ArrayTypes::getHdf5NativeType(const ArrayDimensions& dims, const std::string&);

            template<> const H5::DataType ArrayTypes::getHdf5StandardType(const ArrayDimensions& dims, const signed char&);
            template<> const H5::DataType ArrayTypes::getHdf5StandardType(const ArrayDimensions& dims, const short&);
            template<> const H5::DataType ArrayTypes::getHdf5StandardType(const ArrayDimensions& dims, const int&);
            template<> const H5::DataType ArrayTypes::getHdf5StandardType(const ArrayDimensions& dims, const long long&);
            template<> const H5::DataType ArrayTypes::getHdf5StandardType(const ArrayDimensions& dims, const unsigned char&);
            template<> const H5::DataType ArrayTypes::getHdf5StandardType(const ArrayDimensions& dims, const unsigned short&);
            template<> const H5::DataType ArrayTypes::getHdf5StandardType(const ArrayDimensions& dims, const unsigned int&);
            template<> const H5::DataType ArrayTypes::getHdf5StandardType(const ArrayDimensions& dims, const unsigned long long&);
            template<> const H5::DataType ArrayTypes::getHdf5StandardType(const ArrayDimensions& dims, const float&);
            template<> const H5::DataType ArrayTypes::getHdf5StandardType(const ArrayDimensions& dims, const double&);
            template<> const H5::DataType ArrayTypes::getHdf5StandardType(const ArrayDimensions& dims, const bool&);
            template<> const H5::DataType ArrayTypes::getHdf5StandardType(const ArrayDimensions& dims, const std::string&);



        }
    }
}


#endif	/* KARABO_IO_TYPETRAITS_HH */

