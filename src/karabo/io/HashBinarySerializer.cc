/*
 * $Id$
 *
 * Author: <djelloul.boukhelef@xfel.eu>
 *
 * Created on February 27, 2013, 10:03 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/shared_array.hpp>

#include "HashBinarySerializer.hh"

using namespace karabo::util;
using namespace std;

namespace karabo {
    namespace io {

        KARABO_REGISTER_FOR_CONFIGURATION(BinarySerializer<Hash>, HashBinarySerializer);

        void HashBinarySerializer::serialize(const Hash& hash, std::ostream& os) {
            writeSize(os, hash.size());
            for (Hash::const_iterator iter = hash.begin(); iter != hash.end(); ++iter) {
                serialize(*iter, os);
            }
        }

        void HashBinarySerializer::serialize(Hash& hash, std::istream& is) {
            size_t size = readSize(is);
            for (size_t i = 0; i < size; ++i) {
                Hash::Node element;
                std::string name = readKey(is);
                serialize(element, is);
                hash.set(name, element.getValueAsAny());
                hash.setAttributes(name, element.getAttributes());
            }
        }

        void HashBinarySerializer::serialize(const Hash::Node& element, std::ostream& os) {
            writeKey(os, element.getKey());
            writeType(os, element.getType());
            serialize(element.getAttributes(), os);
            if (element.getType() == Types::HASH) {
                serialize(element.getValue<Hash > (), os);
            } else {
                serialize(element.getValueAsAny(), element.getType(), os);
            }
        }

        void HashBinarySerializer::serialize(Hash::Node& element, std::istream& is) {
            Types::ReferenceType type = readType(is);
            serialize(element.getAttributes(), is);

            if (type == Types::HASH) {
                element.getValueAsAny() = Hash();
            }
            serialize(element.getValueAsAny(), type, is);
        }

        void HashBinarySerializer::serialize(const Hash::Attributes& attributes, std::ostream& os) {
            writeSize(os, attributes.size());
            for (Hash::Attributes::const_iterator iter = attributes.begin(); iter != attributes.end(); ++iter) {
                const Element<std::string>& attr = *iter;
                writeKey(os, attr.getKey());
                writeType(os, attr.getType());
                serialize(attr.getValueAsAny(), attr.getType(), os);
            }
        }

        void HashBinarySerializer::serialize(Hash::Attributes& attributes, std::istream& is) {
            size_t size = readSize(is);
            for (size_t i = 0; i < size; ++i) {
                std::string name = readKey(is);
                Types::ReferenceType type = readType(is);
                boost::any value;
                serialize(value, type, is);
                attributes.set(name, value);
            }
        }

        void HashBinarySerializer::serialize(const boost::any& value, const Types::ReferenceType type, std::ostream& os) {
            switch (Types::category(type)) {
                case Types::HASH:
                case Types::SIMPLE: return writeSingleValue(os, value, type);
                case Types::VECTOR_HASH:
                case Types::SEQUENCE: return writeSequence(os, value, type);
                default:
                    throw KARABO_IO_EXCEPTION("Could not properly categorize value type \"" + Types::to<ToLiteral>(type) + "\" for writing to archive");
            }
        }

        void HashBinarySerializer::serialize(boost::any& value, const Types::ReferenceType type, std::istream& is) {
            switch (Types::category(type)) {
                    // case Types::HASH:
                case Types::SIMPLE: value = readSingleValue(is, type);
                    return;
                case Types::SEQUENCE: readSequence(is, value, type);
                    return;
                case Types::HASH:
                    serialize(boost::any_cast<Hash& > (value), is);
                    return;
                case Types::VECTOR_HASH:
                {
                    size_t size = readSize(is);
                    value = std::vector<Hash > (size);
                    std::vector<Hash>& result = boost::any_cast<std::vector<Hash>& >(value);
                    for (size_t i = 0; i < size; ++i) {
                        serialize(result[i], is);
                    }
                    return;
                }
                default:
                    throw KARABO_IO_EXCEPTION("Could not properly categorize value \"" + Types::to<ToLiteral>(type) + "\" for reading from archive");
            }
        }

        template<>
        std::string HashBinarySerializer::readSingleValue(std::istream& is) {
            size_t size = readSize(is);
            boost::shared_array<char> buffer(new char[size + 1]);
            buffer[size] = 0;
            is.read(buffer.get(), size);
            return std::string(buffer.get());
        }

        template<>
        std::complex<double> HashBinarySerializer::readSingleValue(std::istream& is) {
            return readComplexValue<double > (is);
        }

        template<>
        std::complex<float> HashBinarySerializer::readSingleValue(std::istream& is) {
            return readComplexValue<float> (is);
        }

        template<>
        Hash HashBinarySerializer::readSingleValue(std::istream& is) {
            Hash hash;
            serialize(hash, is);
            return hash;
        }

        boost::any HashBinarySerializer::readSingleValue(std::istream& is, const Types::ReferenceType type) {
            switch (type) {
                case Types::CHAR: return boost::any(readSingleValue<char>(is));
                case Types::INT8: return boost::any(readSingleValue<signed char>(is));
                case Types::INT16: return boost::any(readSingleValue<short>(is));
                case Types::INT32: return boost::any(readSingleValue<int>(is));
                case Types::INT64: return boost::any(readSingleValue<long long>(is));
                case Types::UINT8: return boost::any(readSingleValue<unsigned char>(is));
                case Types::UINT16: return boost::any(readSingleValue<unsigned short>(is));
                case Types::UINT32: return boost::any(readSingleValue<unsigned int>(is));
                case Types::UINT64: return boost::any(readSingleValue<unsigned long long>(is));
                case Types::FLOAT: return boost::any(readSingleValue<float>(is));
                case Types::DOUBLE: return boost::any(readSingleValue<double>(is));
                case Types::BOOL: return boost::any(readSingleValue<bool>(is));
                case Types::COMPLEX_FLOAT: return boost::any(readSingleValue<std::complex<float> >(is));
                case Types::COMPLEX_DOUBLE: return boost::any(readSingleValue<std::complex<double> >(is));
                case Types::STRING: return boost::any(readSingleValue<std::string > (is));
                case Types::HASH: return boost::any(readSingleValue<Hash > (is));
                default:
                    throw KARABO_IO_EXCEPTION("Encountered unknown data type whilst reading from binary archive");
            }
        }

        void HashBinarySerializer::readSequence(std::istream& is, boost::any& result, const Types::ReferenceType type) {
            size_t size = readSize(is);
            switch (type) {
                case Types::VECTOR_BOOL: return readSequence<bool > (is, result, size);
                case Types::VECTOR_STRING: return readSequence<std::string > (is, result, size);
                case Types::VECTOR_CHAR: return readSequence<char>(is, result, size);
                case Types::VECTOR_INT8: return readSequence<signed char>(is, result, size);
                case Types::VECTOR_INT16: return readSequence<short>(is, result, size);
                case Types::VECTOR_INT32: return readSequence<int>(is, result, size);
                case Types::VECTOR_INT64: return readSequence<long long>(is, result, size);
                case Types::VECTOR_UINT8: return readSequence<unsigned char>(is, result, size);
                case Types::VECTOR_UINT16: return readSequence<unsigned short>(is, result, size);
                case Types::VECTOR_UINT32: return readSequence<unsigned int>(is, result, size);
                case Types::VECTOR_UINT64: return readSequence<unsigned long long>(is, result, size);
                case Types::VECTOR_FLOAT: return readSequence<float>(is, result, size);
                case Types::VECTOR_DOUBLE: return readSequence<double>(is, result, size);
                case Types::VECTOR_COMPLEX_FLOAT: return readSequence<std::complex<float> >(is, result, size);
                case Types::VECTOR_COMPLEX_DOUBLE: return readSequence<std::complex<double> >(is, result, size);
                case Types::VECTOR_HASH: return readSequence<Hash > (is, result, size);
                default:
                    throw KARABO_IO_EXCEPTION("Encountered unknown array data type whilst reading from binary archive");
            }
        }

        template<>
        void HashBinarySerializer::writeSingleValue(std::ostream& os, const std::string& str) {
            size_t size = str.length();
            writeSize(os, size);
            os.write(str.c_str(), size);
        }

        template<>
        void HashBinarySerializer::writeSingleValue(std::ostream& os, const std::complex<float>& value) {
            return writeComplexValue(os, value);
        }

        template<>
        void HashBinarySerializer::writeSingleValue(std::ostream& os, const std::complex<double>& value) {
            return writeComplexValue(os, value);
        }

        template<>
        void HashBinarySerializer::writeSingleValue(std::ostream& os, const Hash& hash) {
            serialize(hash, os);
        }

        void HashBinarySerializer::writeSingleValue(std::ostream& os, const boost::any& value, const Types::ReferenceType type) {
            switch (type) {
                case Types::CHAR: return writeSingleValue(os, boost::any_cast<char>(value));
                case Types::INT8: return writeSingleValue(os, boost::any_cast<signed char>(value));
                case Types::INT16: return writeSingleValue(os, boost::any_cast<short>(value));
                case Types::INT32: return writeSingleValue(os, boost::any_cast<int>(value));
                case Types::INT64: return writeSingleValue(os, boost::any_cast<long long>(value));
                case Types::UINT8: return writeSingleValue(os, boost::any_cast<unsigned char>(value));
                case Types::UINT16: return writeSingleValue(os, boost::any_cast<unsigned short>(value));
                case Types::UINT32: return writeSingleValue(os, boost::any_cast<unsigned int>(value));
                case Types::UINT64: return writeSingleValue(os, boost::any_cast<unsigned long long>(value));
                case Types::FLOAT: return writeSingleValue(os, boost::any_cast<float>(value));
                case Types::DOUBLE: return writeSingleValue(os, boost::any_cast<double>(value));
                case Types::BOOL: return writeSingleValue(os, boost::any_cast<bool>(value));
                case Types::COMPLEX_FLOAT: return writeSingleValue(os, boost::any_cast<std::complex<float> >(value));
                case Types::COMPLEX_DOUBLE: return writeSingleValue(os, boost::any_cast<std::complex<double> >(value));
                case Types::STRING: return writeSingleValue(os, boost::any_cast<std::string > (value)); //
                case Types::HASH: return writeSingleValue(os, boost::any_cast<Hash > (value)); //
                default: 
                    throw KARABO_IO_EXCEPTION("Encountered unknown data type whilst writing to binary archive");
            }
        }

        void HashBinarySerializer::writeSequence(std::ostream& os, const boost::any& value, const Types::ReferenceType type) {
            switch (type) {
                case Types::VECTOR_BOOL: return writeSequence(os, boost::any_cast < vector <bool> >(value));
                case Types::VECTOR_STRING: return writeSequence(os, boost::any_cast<vector <std::string> >(value));
                case Types::VECTOR_CHAR: return writeSequence(os, boost::any_cast<vector <char> >(value));
                case Types::VECTOR_INT8: return writeSequence(os, boost::any_cast < vector <signed char> >(value));
                case Types::VECTOR_INT16: return writeSequence(os, boost::any_cast<vector <short> >(value));
                case Types::VECTOR_INT32: return writeSequence(os, boost::any_cast<vector <int> >(value));
                case Types::VECTOR_INT64: return writeSequence(os, boost::any_cast<vector <long long> >(value));
                case Types::VECTOR_UINT8: return writeSequence(os, boost::any_cast<vector <unsigned char> >(value));
                case Types::VECTOR_UINT16: return writeSequence(os, boost::any_cast<vector <unsigned short> >(value));
                case Types::VECTOR_UINT32: return writeSequence(os, boost::any_cast<vector <unsigned int> >(value));
                case Types::VECTOR_UINT64: return writeSequence(os, boost::any_cast<vector <unsigned long long> >(value));
                case Types::VECTOR_FLOAT: return writeSequence(os, boost::any_cast<vector <float> >(value));
                case Types::VECTOR_DOUBLE: return writeSequence(os, boost::any_cast<vector <double> >(value));
                case Types::VECTOR_COMPLEX_FLOAT: return writeSequence(os, boost::any_cast<vector <std::complex<float> > >(value));
                case Types::VECTOR_COMPLEX_DOUBLE: return writeSequence(os, boost::any_cast<vector <std::complex<double> > >(value));
                case Types::VECTOR_HASH: return writeSequence(os, boost::any_cast<vector <Hash > >(value));
                default:
                    throw KARABO_IO_EXCEPTION("Encountered unknown array data type whilst writing to binary archive");
            }
        }

        void HashBinarySerializer::writeSize(std::ostream& os, size_t size) {
            os.write((char*) &size, sizeof (size));
        }

        size_t HashBinarySerializer::readSize(std::istream& is) {
            size_t size;
            is.read((char*) &size, sizeof (size));
            return size;
        }

        void HashBinarySerializer::writeKey(std::ostream& os, const std::string& str) {
            writeSize(os, str.size());
            os.write(str.c_str(), str.size());
        }

        std::string HashBinarySerializer::readKey(std::istream& is) {
            char buffer[256];
            memset(buffer, 0, 256);
            is.read(buffer, readSize(is));
            return std::string(buffer);
        }

        void HashBinarySerializer::writeType(std::ostream& os, const Types::ReferenceType type) {
            return writeSize(os, type);
        }

        Types::ReferenceType HashBinarySerializer::readType(std::istream& is) {
            return Types::ReferenceType(readSize(is));
        }
    }
}
