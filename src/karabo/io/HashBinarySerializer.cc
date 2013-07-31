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
#include "SchemaBinarySerializer.hh"

using namespace karabo::util;
using namespace std;

namespace karabo {
    namespace io {

        KARABO_REGISTER_FOR_CONFIGURATION(BinarySerializer<Hash>, HashBinarySerializer);


        void HashBinarySerializer::write(const Hash& hash, std::ostream& os) {
            writeSize(os, hash.size());
            for (Hash::const_iterator iter = hash.begin(); iter != hash.end(); ++iter) {
                write(*iter, os);
            }
        }


        void HashBinarySerializer::read(Hash& hash, std::istream& is) {
            unsigned size = readSize(is);
            for (unsigned i = 0; i < size; ++i) {
                Hash::Node element;
                std::string name = readKey(is);
                read(element, is);
                hash.set(name, element.getValueAsAny());
                hash.setAttributes(name, element.getAttributes());
            }
        }


        void HashBinarySerializer::write(const Hash::Node& element, std::ostream& os) {
            writeKey(os, element.getKey());
            writeType(os, element.getType());
            write(element.getAttributes(), os);
            if (element.getType() == Types::HASH) {
                write(element.getValue<Hash > (), os);
            } else {
                write(element.getValueAsAny(), element.getType(), os);
            }
        }


        void HashBinarySerializer::read(Hash::Node& element, std::istream& is) {
            Types::ReferenceType type = readType(is);
            read(element.getAttributes(), is);

            if (type == Types::HASH) {
                element.getValueAsAny() = Hash();
            }
            read(element.getValueAsAny(), type, is);
        }


        void HashBinarySerializer::write(const Hash::Attributes& attributes, std::ostream& os) {
            writeSize(os, attributes.size());
            for (Hash::Attributes::const_iterator iter = attributes.begin(); iter != attributes.end(); ++iter) {
                const Element<std::string>& attr = *iter;
                writeKey(os, attr.getKey());
                writeType(os, attr.getType());
                write(attr.getValueAsAny(), attr.getType(), os);
            }
        }


        void HashBinarySerializer::read(Hash::Attributes& attributes, std::istream& is) {
            unsigned size = readSize(is);
            for (unsigned i = 0; i < size; ++i) {
                std::string name = readKey(is);
                Types::ReferenceType type = readType(is);
                boost::any value;
                read(value, type, is);
                attributes.set(name, value);
            }
        }


        void HashBinarySerializer::write(const boost::any& value, const Types::ReferenceType type, std::ostream& os) {
            switch (Types::category(type)) {
                case Types::HASH: // BH: Can this ever happen, I think not!
                case Types::SCHEMA:
                case Types::SIMPLE: return writeSingleValue(os, value, type);
                case Types::VECTOR_HASH:
                case Types::SEQUENCE: return writeSequence(os, value, type);
                default:
                    throw KARABO_IO_EXCEPTION("Could not properly categorize value type \"" + Types::to<ToLiteral>(type) + "\" for writing to archive");
            }
        }


        void HashBinarySerializer::read(boost::any& value, const Types::ReferenceType type, std::istream& is) {
            switch (Types::category(type)) {
                case Types::SCHEMA:
                case Types::SIMPLE: value = readSingleValue(is, type);
                    return;
                case Types::SEQUENCE: readSequence(is, value, type);
                    return;
                case Types::HASH:
                    read(boost::any_cast<Hash& > (value), is);
                    return;
                case Types::VECTOR_HASH:
                {
                    unsigned size = readSize(is);
                    value = std::vector<Hash > (size);
                    std::vector<Hash>& result = boost::any_cast<std::vector<Hash>& >(value);
                    for (unsigned i = 0; i < size; ++i) {
                        read(result[i], is);
                    }
                    return;
                }
                default:
                    throw KARABO_IO_EXCEPTION("Could not properly categorize value \"" + Types::to<ToLiteral>(type) + "\" for reading from archive");
            }
        }


        template<>
        std::string HashBinarySerializer::readSingleValue(std::istream& is) {
            unsigned size = readSize(is);
            boost::shared_array<char> buffer(new char[size + 1]);
            buffer[size] = 0;
            is.read(buffer.get(), size);
            return std::string(buffer.get());
        }

        template<>
        Schema HashBinarySerializer::readSingleValue(std::istream& is) {
            Hash hash;
            SchemaBinarySerializer serializer /*= SchemaBinarySerializer*/(hash);
            // TODO Optimize this by reading directly from istream
            unsigned size = readSize(is);
            Schema schema;
             boost::shared_array<char> buffer(new char[size]);
             is.read(buffer.get(), size);
             serializer.load(schema, buffer.get(), size);
             return schema;
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
            read(hash, is);
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
                case Types::SCHEMA: return boost::any(readSingleValue<Schema >(is));
                default:
                    throw KARABO_IO_EXCEPTION("Encountered unknown data type whilst reading from binary archive");
            }
        }


        void HashBinarySerializer::readSequence(std::istream& is, boost::any& result, const Types::ReferenceType type) {
            unsigned size = readSize(is);
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
            unsigned size = str.length();
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
            write(hash, os);
        }

        template<>
        void HashBinarySerializer::writeSingleValue(std::ostream& os, const Schema& schema) {
            Hash hash;
            SchemaBinarySerializer serializer/* = SchemaBinarySerializer*/(hash);
            std::vector<char> archive;
            serializer.save(schema, archive);
            unsigned size = archive.size();
            writeSize(os, size);
            os.write(&archive[0], size);
        }


        void HashBinarySerializer::writeSingleValue(std::ostream& os, const boost::any& value, const Types::ReferenceType type) {
            switch (type) {
                case Types::CHAR: return writeSingleValue(os, boost::any_cast<const char&>(value));
                case Types::INT8: return writeSingleValue(os, boost::any_cast<const signed char&>(value));
                case Types::INT16: return writeSingleValue(os, boost::any_cast<const short&>(value));
                case Types::INT32: return writeSingleValue(os, boost::any_cast<const int&>(value));
                case Types::INT64: return writeSingleValue(os, boost::any_cast<const long long&>(value));
                case Types::UINT8: return writeSingleValue(os, boost::any_cast<const unsigned char&>(value));
                case Types::UINT16: return writeSingleValue(os, boost::any_cast<const unsigned short&>(value));
                case Types::UINT32: return writeSingleValue(os, boost::any_cast<const unsigned int&>(value));
                case Types::UINT64: return writeSingleValue(os, boost::any_cast<const unsigned long long&>(value));
                case Types::FLOAT: return writeSingleValue(os, boost::any_cast<const float&>(value));
                case Types::DOUBLE: return writeSingleValue(os, boost::any_cast<const double&>(value));
                case Types::BOOL: return writeSingleValue(os, boost::any_cast<const bool&>(value));
                case Types::COMPLEX_FLOAT: return writeSingleValue(os, boost::any_cast<const std::complex<float>& >(value));
                case Types::COMPLEX_DOUBLE: return writeSingleValue(os, boost::any_cast<const std::complex<double>& >(value));
                case Types::STRING: return writeSingleValue(os, boost::any_cast<const std::string& > (value)); //
                case Types::HASH: return writeSingleValue(os, boost::any_cast<const Hash& > (value)); // BH: Can this ever happen, I think not!
                case Types::SCHEMA: return writeSingleValue(os, boost::any_cast<const Schema& >(value));
                default:
                    throw KARABO_IO_EXCEPTION("Encountered unknown data type whilst writing to binary archive");
            }
        }


        void HashBinarySerializer::writeSequence(std::ostream& os, const boost::any& value, const Types::ReferenceType type) {
            switch (type) {
                case Types::VECTOR_CHAR: return writeSequenceBulk(os, boost::any_cast<const vector <char>& >(value));
                case Types::VECTOR_INT8: return writeSequenceBulk(os, boost::any_cast < const vector <signed char>& >(value));
                case Types::VECTOR_INT16: return writeSequenceBulk(os, boost::any_cast<const vector <short>& >(value));
                case Types::VECTOR_INT32: return writeSequenceBulk(os, boost::any_cast<const vector <int>& >(value));
                case Types::VECTOR_INT64: return writeSequenceBulk(os, boost::any_cast<const vector <long long>& >(value));
                case Types::VECTOR_UINT8: return writeSequenceBulk(os, boost::any_cast<const vector <unsigned char>& >(value));
                case Types::VECTOR_UINT16: return writeSequenceBulk(os, boost::any_cast<const vector <unsigned short>& >(value));
                case Types::VECTOR_UINT32: return writeSequenceBulk(os, boost::any_cast<const vector <unsigned int>& >(value));
                case Types::VECTOR_UINT64: return writeSequenceBulk(os, boost::any_cast<const vector <unsigned long long>& >(value));
                case Types::VECTOR_FLOAT: return writeSequenceBulk(os, boost::any_cast<const vector <float>& >(value));
                case Types::VECTOR_DOUBLE: return writeSequenceBulk(os, boost::any_cast<const vector <double>& >(value));
                case Types::VECTOR_COMPLEX_FLOAT: return writeSequence(os, boost::any_cast<const vector <std::complex<float> >& >(value));
                case Types::VECTOR_COMPLEX_DOUBLE: return writeSequence(os, boost::any_cast<const vector <std::complex<double> >& >(value));
                case Types::VECTOR_HASH: return writeSequence(os, boost::any_cast<const vector <Hash >& >(value));
                case Types::VECTOR_STRING: return writeSequence(os, boost::any_cast<const vector <std::string>& >(value));
                case Types::VECTOR_BOOL: return writeSequence(os, boost::any_cast < const vector <bool>& >(value));
                default:
                    throw KARABO_IO_EXCEPTION("Encountered unknown array data type whilst writing to binary archive");
            }
        }


        void HashBinarySerializer::writeSize(std::ostream& os, unsigned size) {
            os.write((char*) &size, sizeof (size));
        }


        unsigned HashBinarySerializer::readSize(std::istream& is) {
            unsigned size;
            is.read((char*) &size, sizeof (size));
            return size;
        }


        void HashBinarySerializer::writeKey(std::ostream& os, const std::string& str) {
            unsigned char size = str.size();
            os.write((char*) &size, sizeof (size));
            os.write(str.c_str(), size);
        }


        std::string HashBinarySerializer::readKey(std::istream& is) {
            char buffer[256]; //memset(buffer, 0, 256);
            unsigned char size = 0;
            is.read((char*) &size, sizeof (size));
            is.read(buffer, size);
            buffer[size] = 0;
            return std::string(buffer);
        }


        void HashBinarySerializer::writeType(std::ostream& os, const Types::ReferenceType type) {
            return writeSize(os, type);
        }


        Types::ReferenceType HashBinarySerializer::readType(std::istream& is) {
            return Types::ReferenceType(readSize(is));
        }


        void HashBinarySerializer::save(const std::vector<karabo::util::Hash>& objects, std::vector<char>& archive) {
            Hash tmp("KRB_Sequence", objects);
            this->save(tmp, archive);
        }


        void HashBinarySerializer::load(std::vector<karabo::util::Hash>& objects, const char* archive, const size_t nBytes) {
            vector<Hash> tmp(1);
            this->load(tmp[0], archive, nBytes);
            if (tmp[0].begin()->getKey() == "KRB_Sequence") {
                objects.swap(tmp[0].get<vector<Hash> >("KRB_Sequence"));
            } else {
                objects.swap(tmp);
            }
        }
    }
}
