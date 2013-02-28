/*
 * $Id: TypeTraits.cc 5260 2012-02-26 22:13:16Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "TypeTraits.hh"
#include <string>
#include <iostream>

using namespace std;

namespace karabo {
    namespace io {
	namespace h5 {

//
// 	    template<> std::string ScalarTypeTraits::classId(const signed char&) {
//		return "Int8";
//	    }
//
//	    template<> std::string ScalarTypeTraits::classId(const short&) {
//		return "Int16";
//	    }
//
//	    template<> std::string ScalarTypeTraits::classId(const int&) {
//		return "Int32";
//	    }
//
//	    template<> std::string ScalarTypeTraits::classId(const long long&) {
//		return "Int64";
//	    }
//
//	    template<> std::string ScalarTypeTraits::classId(const unsigned char&) {
//		return "UInt8";
//	    }
//
//	    template<> std::string ScalarTypeTraits::classId(const unsigned short&) {
//		return "UInt16";
//	    }
//
//	    template<> std::string ScalarTypeTraits::classId(const unsigned int&) {
//		return "UInt32";
//	    }
//
//	    template<> std::string ScalarTypeTraits::classId(const unsigned long long&) {
//		return "UInt64";
//	    }
//
//	    template<> std::string ScalarTypeTraits::classId(const double&) {
//		return "Double";
//	    }
//
//	    template<> std::string ScalarTypeTraits::classId(const float&) {
//		return "Float";
//	    }
//
//	    template<> std::string ScalarTypeTraits::classId(const std::string&) {
//		return "String";
//	    }
//
//	    template<> std::string ScalarTypeTraits::classId(const bool&) {
//		return "Bool";
//	    }
//
//	    template<> std::string ArrayTypeTraits::classId(const signed char&) {
//		return "Int8Array";
//	    }
//
//	    template<> std::string ArrayTypeTraits::classId(const short&) {
//		return "Int16Array";
//	    }
//
//	    template<> std::string ArrayTypeTraits::classId(const int&) {
//		return "Int32Array";
//	    }
//
//	    template<> std::string ArrayTypeTraits::classId(const long long&) {
//		return "Int64Array";
//	    }
//
//	    template<> std::string ArrayTypeTraits::classId(const unsigned char&) {
//		return "UInt8Array";
//	    }
//
//	    template<> std::string ArrayTypeTraits::classId(const unsigned short&) {
//		return "UInt16Array";
//	    }
//
//	    template<> std::string ArrayTypeTraits::classId(const unsigned int&) {
//		return "UInt32Array";
//	    }
//
//	    template<> std::string ArrayTypeTraits::classId(const unsigned long long&) {
//		return "UInt64Array";
//	    }
//
//	    template<> std::string ArrayTypeTraits::classId(const double&) {
//		return "DoubleArray";
//	    }
//
//	    template<> std::string ArrayTypeTraits::classId(const float&) {
//		return "FloatArray";
//	    }
//
//	    template<> std::string ArrayTypeTraits::classId(const std::string&) {
//		return "StringArray";
//	    }
//
//	    template<> std::string ArrayTypeTraits::classId(const bool&) {
//		return "BoolArray";
//	    }

	    // native hdf5 types used in memory

            template <> const H5::DataType ScalarTypes::getHdf5NativeType(const char&) {
		return H5::PredType::NATIVE_CHAR;
	    }
            
	    template <> const H5::DataType ScalarTypes::getHdf5NativeType(const signed char&) {
		return H5::PredType::NATIVE_CHAR;
	    }

	    template <> const H5::DataType ScalarTypes::getHdf5NativeType(const short&) {
		return H5::PredType::NATIVE_SHORT;
	    }

	    template <> const H5::DataType ScalarTypes::getHdf5NativeType(const int&) {
		return H5::PredType::NATIVE_INT;
	    }

	    template <> const H5::DataType ScalarTypes::getHdf5NativeType(const long long&) {
		return H5::PredType::NATIVE_LLONG;
	    }

	    template <> const H5::DataType ScalarTypes::getHdf5NativeType(const unsigned char&) {
		return H5::PredType::NATIVE_UCHAR;
	    }

	    template <> const H5::DataType ScalarTypes::getHdf5NativeType(const unsigned short&) {
		return H5::PredType::NATIVE_USHORT;
	    }

	    template <> const H5::DataType ScalarTypes::getHdf5NativeType(const unsigned int&) {
		return H5::PredType::NATIVE_INT;
	    }

	    template <> const H5::DataType ScalarTypes::getHdf5NativeType(const unsigned long long&) {
		return H5::PredType::NATIVE_ULLONG;
	    }

	    template <> const H5::DataType ScalarTypes::getHdf5NativeType(const bool&) {
		return H5::PredType::NATIVE_UCHAR;
	    }

	    template <> const H5::DataType ScalarTypes::getHdf5NativeType(const float&) {
		return H5::PredType::NATIVE_FLOAT;
	    }

	    template <> const H5::DataType ScalarTypes::getHdf5NativeType(const double&) {
		return H5::PredType::NATIVE_DOUBLE;
	    }

	    template <> const H5::DataType ScalarTypes::getHdf5NativeType(const std::string&) {
		static H5::StrType strType(H5::PredType::C_S1, H5T_VARIABLE);
		return strType;
	    }


	    // standard hdf5 types used in file

            
	    template <> const H5::DataType ScalarTypes::getHdf5StandardType(const char&) {
		return H5::PredType::STD_I8LE;
	    }
            
	    template <> const H5::DataType ScalarTypes::getHdf5StandardType(const signed char&) {
		return H5::PredType::STD_I8LE;
	    }

	    template <> const H5::DataType ScalarTypes::getHdf5StandardType(const short&) {
		return H5::PredType::STD_I16LE;
	    }

	    template <> const H5::DataType ScalarTypes::getHdf5StandardType(const int&) {
		return H5::PredType::STD_I32LE;
	    }

	    template <> const H5::DataType ScalarTypes::getHdf5StandardType(const long long&) {
		return H5::PredType::STD_I64LE;
	    }

	    template <> const H5::DataType ScalarTypes::getHdf5StandardType(const unsigned char&) {
		return H5::PredType::STD_U8LE;
	    }

	    template <> const H5::DataType ScalarTypes::getHdf5StandardType(const unsigned short&) {
		return H5::PredType::STD_U16LE;
	    }

	    template <> const H5::DataType ScalarTypes::getHdf5StandardType(const unsigned int&) {
		return H5::PredType::STD_U32LE;
	    }

	    template <> const H5::DataType ScalarTypes::getHdf5StandardType(const unsigned long long&) {
		return H5::PredType::STD_U64LE;
	    }

	    template <> const H5::DataType ScalarTypes::getHdf5StandardType(const bool&) {
		return H5::PredType::STD_U8LE;
	    }

	    template <> const H5::DataType ScalarTypes::getHdf5StandardType(const float&) {
		return H5::PredType::IEEE_F32LE;
	    }

	    template <> const H5::DataType ScalarTypes::getHdf5StandardType(const double&) {
		return H5::PredType::IEEE_F64LE;
	    }

	    template <> const H5::DataType ScalarTypes::getHdf5StandardType(const std::string&) {
		static H5::StrType strType(H5::PredType::C_S1, H5T_VARIABLE);
		return strType;
	    }

//
//
//	    // native hdf5 types used in memory
//
//	    template <> const H5::DataType ArrayTypes::getHdf5NativeType(const ArrayDimensions& dims, const signed char&) {
//		return H5::ArrayType(H5::PredType::NATIVE_CHAR, dims.size(), &dims[0]);
//	    }
//
//	    template <> const H5::DataType ArrayTypes::getHdf5NativeType(const ArrayDimensions& dims, const short&) {
//		return H5::ArrayType(H5::PredType::NATIVE_SHORT, dims.size(), &dims[0]);
//	    }
//
//	    template <> const H5::DataType ArrayTypes::getHdf5NativeType(const ArrayDimensions& dims, const int&) {
//		return H5::ArrayType(H5::PredType::NATIVE_INT, dims.size(), &dims[0]);
//	    }
//
//	    template <> const H5::DataType ArrayTypes::getHdf5NativeType(const ArrayDimensions& dims, const long long&) {
//		return H5::ArrayType(H5::PredType::NATIVE_LLONG, dims.size(), &dims[0]);
//	    }
//
//	    template <> const H5::DataType ArrayTypes::getHdf5NativeType(const ArrayDimensions& dims, const unsigned char&) {
//		return H5::ArrayType(H5::PredType::NATIVE_UCHAR, dims.size(), &dims[0]);
//	    }
//
//	    template <> const H5::DataType ArrayTypes::getHdf5NativeType(const ArrayDimensions& dims, const unsigned short&) {
//		return H5::ArrayType(H5::PredType::NATIVE_USHORT, dims.size(), &dims[0]);
//	    }
//
//	    template <> const H5::DataType ArrayTypes::getHdf5NativeType(const ArrayDimensions& dims, const unsigned int&) {
//		return H5::ArrayType(H5::PredType::NATIVE_UINT, dims.size(), &dims[0]);
//	    }
//
//	    template <> const H5::DataType ArrayTypes::getHdf5NativeType(const ArrayDimensions& dims, const unsigned long long&) {
//		return H5::ArrayType(H5::PredType::NATIVE_ULLONG, dims.size(), &dims[0]);
//	    }
//
//	    template <> const H5::DataType ArrayTypes::getHdf5NativeType(const ArrayDimensions& dims, const bool&) {
//		// use UCHAR to store bool variables
//		return H5::ArrayType(H5::PredType::NATIVE_UCHAR, dims.size(), &dims[0]);
//	    }
//
//	    template <> const H5::DataType ArrayTypes::getHdf5NativeType(const ArrayDimensions& dims, const float&) {
//		return H5::ArrayType(H5::PredType::NATIVE_FLOAT, dims.size(), &dims[0]);
//	    }
//
//	    template <> const H5::DataType ArrayTypes::getHdf5NativeType(const ArrayDimensions& dims, const double&) {
//		return H5::ArrayType(H5::PredType::NATIVE_DOUBLE, dims.size(), &dims[0]);
//	    }
//
//	    template <> const H5::DataType ArrayTypes::getHdf5NativeType(const ArrayDimensions& dims, const string&) {
//		H5::StrType strType(H5::PredType::C_S1, H5T_VARIABLE);
//		return H5::ArrayType(strType, dims.size(), &dims[0]);
//	    }
//
//
//	    // standard hdf5 types used in file
//
//	    template <> const H5::DataType ArrayTypes::getHdf5StandardType(const ArrayDimensions& dims, const signed char&) {
//		return H5::ArrayType(H5::PredType::STD_I8LE, dims.size(), &dims[0]);
//	    }
//
//	    template <> const H5::DataType ArrayTypes::getHdf5StandardType(const ArrayDimensions& dims, const short&) {
//		return H5::ArrayType(H5::PredType::STD_I16LE, dims.size(), &dims[0]);
//	    }
//
//	    template <> const H5::DataType ArrayTypes::getHdf5StandardType(const ArrayDimensions& dims, const int&) {
//		return H5::ArrayType(H5::PredType::STD_I32LE, dims.size(), &dims[0]);
//	    }
//
//	    template <> const H5::DataType ArrayTypes::getHdf5StandardType(const ArrayDimensions& dims, const long long&) {
//		return H5::ArrayType(H5::PredType::STD_I64LE, dims.size(), &dims[0]);
//	    }
//
//	    template <> const H5::DataType ArrayTypes::getHdf5StandardType(const ArrayDimensions& dims, const unsigned char&) {
//		return H5::ArrayType(H5::PredType::STD_U8LE, dims.size(), &dims[0]);
//	    }
//
//	    template <> const H5::DataType ArrayTypes::getHdf5StandardType(const ArrayDimensions& dims, const unsigned short&) {
//		return H5::ArrayType(H5::PredType::STD_U16LE, dims.size(), &dims[0]);
//	    }
//
//	    template <> const H5::DataType ArrayTypes::getHdf5StandardType(const ArrayDimensions& dims, const unsigned int&) {
//		return H5::ArrayType(H5::PredType::STD_U32LE, dims.size(), &dims[0]);
//	    }
//
//	    template <> const H5::DataType ArrayTypes::getHdf5StandardType(const ArrayDimensions& dims, const unsigned long long&) {
//		return H5::ArrayType(H5::PredType::STD_U64LE, dims.size(), &dims[0]);
//	    }
//
//	    template <> const H5::DataType ArrayTypes::getHdf5StandardType(const ArrayDimensions& dims, const bool&) {
//		return H5::ArrayType(H5::PredType::STD_U8LE, dims.size(), &dims[0]);
//	    }
//
//	    template <> const H5::DataType ArrayTypes::getHdf5StandardType(const ArrayDimensions& dims, const float&) {
//		return H5::ArrayType(H5::PredType::IEEE_F32LE, dims.size(), &dims[0]);
//	    }
//
//	    template <> const H5::DataType ArrayTypes::getHdf5StandardType(const ArrayDimensions& dims, const double&) {
//		return H5::ArrayType(H5::PredType::IEEE_F64LE, dims.size(), &dims[0]);
//	    }
//
//	    template <> const H5::DataType ArrayTypes::getHdf5StandardType(const ArrayDimensions& dims, const std::string&) {
//		H5::StrType strType(H5::PredType::C_S1, H5T_VARIABLE);
//		return H5::ArrayType(strType, dims.size(), &dims[0]);
//	    }
//


	}
    }
}
