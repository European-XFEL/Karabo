/*
 * $Id$
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <karabo/util.hpp>

#include <boost/shared_array.hpp>

#include "HashBinarySerializerFast.hh"
#include "SchemaBinarySerializer.hh"

using namespace karabo::util;
using namespace std;

KARABO_EXPLICIT_TEMPLATE(karabo::io::BinarySerializer<karabo::util::Hash>)

namespace karabo {
    namespace io {

        KARABO_REGISTER_FOR_CONFIGURATION(BinarySerializer<Hash>, HashBinarySerializerFast);


        void HashBinarySerializerFast::expectedParameters(karabo::util::Schema& expected) {
            BOOL_ELEMENT(expected).key("nodesAsSharedPtr")
                    .description("If true, nested hashes will be stored as shared pointers during de-serialization")
                    .displayedName("NodesAsSharedPtr")
                    .assignmentOptional().defaultValue(false)
                    .expertAccess()
                    .commit();
        }


        HashBinarySerializerFast::HashBinarySerializerFast(const karabo::util::Hash& input) {
            input.get("nodesAsSharedPtr", m_nodesAsSharedPtr);
        }


        void HashBinarySerializerFast::save(const karabo::util::Hash& object, std::vector<char>& buffer) {
            buffer.resize(0);
            writeHash(object, buffer);
        }


        void HashBinarySerializerFast::writeHash(const karabo::util::Hash& hash, std::vector<char>& buffer) {
            writeSize(buffer, hash.size());
            for (Hash::const_iterator iter = hash.begin(); iter != hash.end(); ++iter) {
                writeNode(*iter, buffer);
            }
        }


        void HashBinarySerializerFast::writeSize(std::vector<char>& buffer, const unsigned int size) {
            writeSingleValue(buffer, size);
        }


        void HashBinarySerializerFast::writeNode(const karabo::util::Hash::Node& element, std::vector<char>& buffer) {
            writeKey(buffer, element.getKey());
            if (element.is<Hash>()) {
                writeType(buffer, Types::HASH);
                writeAttributes(element.getAttributes(), buffer);
                writeHash(element.getValue<Hash > (), buffer);
            } else if (element.is<Hash::Pointer>()) {
                writeType(buffer, Types::HASH);
                writeAttributes(element.getAttributes(), buffer);
                writeHash(*(element.getValue<Hash::Pointer > ()), buffer);
            } else if (element.is<vector<Hash> >()) {
                writeType(buffer, Types::VECTOR_HASH);
                writeAttributes(element.getAttributes(), buffer);
                const vector<Hash>& tmp = element.getValue<vector<Hash> >();
                writeSize(buffer, tmp.size());
                for (size_t i = 0; i < tmp.size(); ++i) {
                    writeHash(tmp[i], buffer);
                }
            } else if (element.is<vector<Hash::Pointer> >()) {
                writeType(buffer, Types::VECTOR_HASH);
                writeAttributes(element.getAttributes(), buffer);
                const vector<Hash::Pointer>& tmp = element.getValue<vector<Hash::Pointer> >();
                writeSize(buffer, tmp.size());
                for (size_t i = 0; i < tmp.size(); ++i) {
                    writeHash(*(tmp[i]), buffer);
                }
            } else {
                writeType(buffer, element.getType());
                writeAttributes(element.getAttributes(), buffer);
                writeAny(element.getValueAsAny(), element.getType(), buffer);
            }
        }


        void HashBinarySerializerFast::writeKey(std::vector<char>& buffer, const std::string& str) {
            // ATTENTION: Some optimization takes place here, the size indicator for a key is limited to 1 byte
            // instead of the generic 4 byte for everything else!!
            signed char size = str.size();
            writeSingleValue(buffer, size);
            const size_t pos = buffer.size();
            buffer.resize(pos + size);
            std::memcpy(&buffer[pos], str.c_str(), size);
        }


        void HashBinarySerializerFast::writeType(std::vector<char>& buffer, const Types::ReferenceType type) {
            return writeSize(buffer, type);
        }


        void HashBinarySerializerFast::writeAttributes(const karabo::util::Hash::Attributes& attributes, std::vector<char>& buffer) {
            writeSize(buffer, attributes.size());
            for (Hash::Attributes::const_iterator iter = attributes.begin(); iter != attributes.end(); ++iter) {
                const Element<std::string>& attr = *iter;
                writeKey(buffer, attr.getKey());
                const Types::ReferenceType type = attr.getType();
                writeType(buffer, type);
                writeAny(attr.getValueAsAny(), type, buffer);
            }
        }


        void HashBinarySerializerFast::writeAny(const boost::any& value, const karabo::util::Types::ReferenceType type, std::vector<char>& buffer) {

            switch (Types::category(type)) {
                case Types::SCHEMA:
                case Types::HASH:
                case Types::SIMPLE: return writeSingleValue(buffer, value, type);
                case Types::VECTOR_HASH:
                case Types::SEQUENCE: return writeSequence(buffer, value, type);
                case Types::RAW_ARRAY: return writeRawArray(buffer, value, type);
                default:
                    throw KARABO_IO_EXCEPTION("Could not properly categorize value type \"" + Types::to<ToLiteral>(type) + "\" for writing to archive");
            }
        }


        void HashBinarySerializerFast::writeSingleValue(std::vector<char>& buffer, const boost::any& value, const Types::ReferenceType type) {
            switch (type) {
                case Types::CHAR: return writeSingleValue(buffer, boost::any_cast<const char&>(value));
                case Types::INT8: return writeSingleValue(buffer, boost::any_cast<const signed char&>(value));
                case Types::INT16: return writeSingleValue(buffer, boost::any_cast<const short&>(value));
                case Types::INT32: return writeSingleValue(buffer, boost::any_cast<const int&>(value));
                case Types::INT64: return writeSingleValue(buffer, boost::any_cast<const long long&>(value));
                case Types::UINT8: return writeSingleValue(buffer, boost::any_cast<const unsigned char&>(value));
                case Types::UINT16: return writeSingleValue(buffer, boost::any_cast<const unsigned short&>(value));
                case Types::UINT32: return writeSingleValue(buffer, boost::any_cast<const unsigned int&>(value));
                case Types::UINT64: return writeSingleValue(buffer, boost::any_cast<const unsigned long long&>(value));
                case Types::FLOAT: return writeSingleValue(buffer, boost::any_cast<const float&>(value));
                case Types::DOUBLE: return writeSingleValue(buffer, boost::any_cast<const double&>(value));
                case Types::BOOL: return writeSingleValue(buffer, boost::any_cast<const bool&>(value));
                case Types::COMPLEX_FLOAT: return writeSingleValue(buffer, boost::any_cast<const std::complex<float>& >(value));
                case Types::COMPLEX_DOUBLE: return writeSingleValue(buffer, boost::any_cast<const std::complex<double>& >(value));
                case Types::STRING: return writeSingleValue(buffer, boost::any_cast<const std::string& > (value)); //               
                case Types::SCHEMA: return writeSingleValue(buffer, boost::any_cast<const Schema& >(value));
                case Types::HASH: return writeSingleValue(buffer, boost::any_cast<const Hash& > (value));
                case Types::NONE: return writeSingleValue(buffer, boost::any_cast<const CppNone&>(value));
                default:
                    throw KARABO_IO_EXCEPTION("Encountered unknown data type whilst writing to binary archive");
            }
        }


        template<>
        void HashBinarySerializerFast::writeSingleValue(std::vector<char>& buffer, const std::string& str) {
            size_t size = str.length();
            writeRawArray(buffer, std::make_pair(str.c_str(), size));
        }


        template<>
        void HashBinarySerializerFast::writeSingleValue(std::vector<char>& buffer, const std::complex<float>& value) {
            return writeComplexValue(buffer, value);
        }


        template<>
        void HashBinarySerializerFast::writeSingleValue(std::vector<char>& buffer, const std::complex<double>& value) {
            return writeComplexValue(buffer, value);
        }


        template<>
        void HashBinarySerializerFast::writeSingleValue(std::vector<char>& buffer, const Schema& schema) {

            // TODO !!!! IMPROVE THIS CRAZY COPIES !!!!
            Hash hash;
            SchemaBinarySerializer serializer/* = SchemaBinarySerializer*/(hash);
            std::vector<char> archive;
            serializer.save(schema, archive);
            writeSequenceBulk(buffer, archive);
        }


        template<>
        void HashBinarySerializerFast::writeSingleValue(std::vector<char>& buffer, const Hash& hash) {
            writeHash(hash, buffer);
        }


        template<>
        void HashBinarySerializerFast::writeSingleValue(std::vector<char>& buffer, const CppNone& value) {
            unsigned int size = 0;
            writeSize(buffer, size);
        }


        void HashBinarySerializerFast::writeSequence(std::vector<char>& buffer, const boost::any& value, const Types::ReferenceType type) {
            switch (type) {
                case Types::VECTOR_CHAR: return writeSequenceBulk(buffer, boost::any_cast<const vector <char>& >(value));
                case Types::VECTOR_INT8: return writeSequenceBulk(buffer, boost::any_cast < const vector <signed char>& >(value));
                case Types::VECTOR_INT16: return writeSequenceBulk(buffer, boost::any_cast<const vector <short>& >(value));
                case Types::VECTOR_INT32: return writeSequenceBulk(buffer, boost::any_cast<const vector <int>& >(value));
                case Types::VECTOR_INT64: return writeSequenceBulk(buffer, boost::any_cast<const vector <long long>& >(value));
                case Types::VECTOR_UINT8: return writeSequenceBulk(buffer, boost::any_cast<const vector <unsigned char>& >(value));
                case Types::VECTOR_UINT16: return writeSequenceBulk(buffer, boost::any_cast<const vector <unsigned short>& >(value));
                case Types::VECTOR_UINT32: return writeSequenceBulk(buffer, boost::any_cast<const vector <unsigned int>& >(value));
                case Types::VECTOR_UINT64: return writeSequenceBulk(buffer, boost::any_cast<const vector <unsigned long long>& >(value));
                case Types::VECTOR_FLOAT: return writeSequenceBulk(buffer, boost::any_cast<const vector <float>& >(value));
                case Types::VECTOR_DOUBLE: return writeSequenceBulk(buffer, boost::any_cast<const vector <double>& >(value));
                case Types::VECTOR_COMPLEX_FLOAT: return writeSequence(buffer, boost::any_cast<const vector <std::complex<float> >& >(value));
                case Types::VECTOR_COMPLEX_DOUBLE: return writeSequence(buffer, boost::any_cast<const vector <std::complex<double> >& >(value));
                case Types::VECTOR_STRING: return writeSequence(buffer, boost::any_cast<const vector <std::string>& >(value));
                case Types::VECTOR_BOOL: return writeSequence(buffer, boost::any_cast < const vector <bool>& >(value));
                case Types::VECTOR_HASH: return writeSequence(buffer, boost::any_cast<const vector <Hash >& >(value));
                case Types::VECTOR_NONE: return writeSequence(buffer, boost::any_cast < const vector <CppNone>& >(value));
                default:
                    throw KARABO_IO_EXCEPTION("Encountered unknown array data type whilst writing to binary archive");
            }
        }


        void HashBinarySerializerFast::writeRawArray(std::vector<char>& buffer, const boost::any& value, const karabo::util::Types::ReferenceType type) {
            switch (type) {
#define _KARABO_HELPER_MACRO(RefType, CppType) case Types::RefType: return writeRawArray(buffer, boost::any_cast<std::pair<const CppType*, size_t> >(value));

                    _KARABO_HELPER_MACRO(ARRAY_BOOL, bool)
                    _KARABO_HELPER_MACRO(ARRAY_CHAR, char)
                    _KARABO_HELPER_MACRO(ARRAY_INT8, signed char)
                    _KARABO_HELPER_MACRO(ARRAY_UINT8, unsigned char)
                    _KARABO_HELPER_MACRO(ARRAY_INT16, short)
                    _KARABO_HELPER_MACRO(ARRAY_UINT16, unsigned short)
                    _KARABO_HELPER_MACRO(ARRAY_INT32, int)
                    _KARABO_HELPER_MACRO(ARRAY_UINT32, unsigned int)
                    _KARABO_HELPER_MACRO(ARRAY_INT64, long long)
                    _KARABO_HELPER_MACRO(ARRAY_UINT64, unsigned long long)
                    _KARABO_HELPER_MACRO(ARRAY_FLOAT, float)
                    _KARABO_HELPER_MACRO(ARRAY_DOUBLE, double)

#undef _KARABO_HELPER_MACRO
                        default:
                    throw KARABO_IO_EXCEPTION("Encountered unknown array data type whilst writing to binary archive");
            }
        }


        void HashBinarySerializerFast::readHash(Hash& hash, std::istream& is) {
            unsigned size = readSize(is);
            for (unsigned i = 0; i < size; ++i) {               
                std::string name = readKey(is);
                Hash::Node& node = hash.set(name, true); // The boolean is a dummy to allow working on references later
                readNode(node, is);
            }
        }


        void HashBinarySerializerFast::readNode(Hash::Node& node, std::istream& is) {
            Types::ReferenceType type = readType(is);
            readAttributes(node.getAttributes(), is);

            if (type == Types::HASH) {
                if (m_nodesAsSharedPtr) {
                    node.setValue(Hash::Pointer(new Hash()));
                    Hash& tmp = *(node.getValue<Hash::Pointer>());
                    readHash(tmp, is);
                } else {
                    node.setValue(Hash());
                    Hash& tmp = node.getValue<Hash>();
                    readHash(tmp, is);
                }
            } else if (type == Types::VECTOR_HASH) {
                unsigned size = readSize(is);
                if (m_nodesAsSharedPtr) {
                    node.setValue(std::vector<Hash::Pointer > ());
                    std::vector<Hash::Pointer>& result = node.getValue<std::vector<Hash::Pointer > >();
                    result.resize(size, Hash::Pointer(new Hash()));
                    for (unsigned i = 0; i < size; ++i) {
                        readHash(*(result[i]), is);
                    }
                } else {
                    node.setValue(std::vector<Hash > ());
                    std::vector<Hash>& result = node.getValue<std::vector<Hash > >();
                    result.resize(size);
                    for (unsigned i = 0; i < size; ++i) {
                        readHash(result[i], is);
                    }
                }
            } else {
                readAny(node.getValueAsAny(), type, is);
            }
        }


        void HashBinarySerializerFast::readAttributes(Hash::Attributes& attributes, std::istream& is) {
            unsigned size = readSize(is);
            for (unsigned i = 0; i < size; ++i) {
                std::string name = readKey(is);
                Types::ReferenceType type = readType(is);
                boost::any value;
                readAny(value, type, is);
                attributes.set(name, value);
            }
        }


        void HashBinarySerializerFast::readAny(boost::any& value, const Types::ReferenceType type, std::istream& is) {
            switch (Types::category(type)) {
                case Types::SCHEMA:
                case Types::SIMPLE: readSingleValue(is, value, type);
                    return;
                case Types::RAW_ARRAY:
                case Types::SEQUENCE: readSequence(is, value, type);
                    return;
                case Types::HASH:
                    readHash(boost::any_cast<Hash& > (value), is);
                    return;
                case Types::VECTOR_HASH:
                {
                    unsigned size = readSize(is);
                    value = std::vector<Hash > ();
                    std::vector<Hash>& result = boost::any_cast<std::vector<Hash>& >(value);
                    result.resize(size);
                    for (unsigned i = 0; i < size; ++i) {
                        readHash(result[i], is);
                    }
                    return;
                }
                default:
                    throw KARABO_IO_EXCEPTION("Could not properly categorize value \"" + Types::to<ToLiteral>(type) + "\" for reading from archive");
            }
        }


        template<>
        std::string HashBinarySerializerFast::readSingleValue(std::istream& is) {
            unsigned size = readSize(is);
            boost::shared_array<char> buffer(new char[size + 1]);
            buffer[size] = 0;
            is.read(buffer.get(), size);
            return std::string(buffer.get());
        }


        template<>
        Schema HashBinarySerializerFast::readSingleValue(std::istream& is) {
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
        std::complex<double> HashBinarySerializerFast::readSingleValue(std::istream& is) {
            return readComplexValue<double > (is);
        }


        template<>
        std::complex<float> HashBinarySerializerFast::readSingleValue(std::istream& is) {
            return readComplexValue<float> (is);
        }


        template<>
        Hash HashBinarySerializerFast::readSingleValue(std::istream& is) {
            Hash hash;
            readHash(hash, is);
            return hash;
        }


        template<>
        karabo::util::CppNone HashBinarySerializerFast::readSingleValue(std::istream& is) {
            unsigned size = readSize(is);
            if (size != 0)
                throw KARABO_IO_EXCEPTION("Encountered not 'None' data type whilst reading from binary archive: size is " + toString(size) + ", but should be 0");
            return karabo::util::CppNone();
        }


        void HashBinarySerializerFast::readSingleValue(std::istream& is, boost::any& value, const Types::ReferenceType type) {
            switch (type) {
                case Types::CHAR: value = readSingleValue<char>(is); break;
                case Types::INT8: value = readSingleValue<signed char>(is); break;
                case Types::INT16: value = readSingleValue<short>(is); break;
                case Types::INT32: value = readSingleValue<int>(is); break;
                case Types::INT64: value = readSingleValue<long long>(is); break;
                case Types::UINT8: value = readSingleValue<unsigned char>(is); break;
                case Types::UINT16: value = readSingleValue<unsigned short>(is); break;
                case Types::UINT32: value = readSingleValue<unsigned int>(is); break;
                case Types::UINT64: value = readSingleValue<unsigned long long>(is); break;
                case Types::FLOAT: value = readSingleValue<float>(is); break;
                case Types::DOUBLE: value = readSingleValue<double>(is); break;
                case Types::BOOL: value = readSingleValue<bool>(is); break;
                case Types::COMPLEX_FLOAT: value = readSingleValue<std::complex<float> >(is); break;
                case Types::COMPLEX_DOUBLE: value = readSingleValue<std::complex<double> >(is); break;
                case Types::STRING: value = readSingleValue<std::string > (is); break;
                case Types::SCHEMA: value = readSingleValue<Schema >(is); break;
                case Types::HASH: value = readSingleValue<Hash > (is); break;
                case Types::NONE: value = readSingleValue<CppNone >(is); break;
                default:
                    throw KARABO_IO_EXCEPTION("Encountered unknown data type whilst reading from binary archive");
            }
        }


        void HashBinarySerializerFast::readSequence(std::istream& is, boost::any& result, const Types::ReferenceType type) {
            unsigned size = readSize(is);
            switch (type) {

                case Types::ARRAY_BOOL:
                case Types::VECTOR_BOOL: return readSequence<bool > (is, result, size);
                case Types::VECTOR_STRING: return readSequence<std::string > (is, result, size);
                case Types::ARRAY_CHAR:
                case Types::VECTOR_CHAR: return readSequenceBulk<char>(is, result, size);
                case Types::ARRAY_INT8:
                case Types::VECTOR_INT8: return readSequenceBulk<signed char>(is, result, size);
                case Types::ARRAY_INT16:
                case Types::VECTOR_INT16: return readSequenceBulk<short>(is, result, size);
                case Types::ARRAY_INT32:
                case Types::VECTOR_INT32: return readSequenceBulk<int>(is, result, size);
                case Types::ARRAY_INT64:
                case Types::VECTOR_INT64: return readSequenceBulk<long long>(is, result, size);
                case Types::ARRAY_UINT8:
                case Types::VECTOR_UINT8: return readSequenceBulk<unsigned char>(is, result, size);
                case Types::ARRAY_UINT16:
                case Types::VECTOR_UINT16: return readSequenceBulk<unsigned short>(is, result, size);
                case Types::ARRAY_UINT32:
                case Types::VECTOR_UINT32: return readSequenceBulk<unsigned int>(is, result, size);
                case Types::ARRAY_UINT64:
                case Types::VECTOR_UINT64: return readSequenceBulk<unsigned long long>(is, result, size);
                case Types::ARRAY_FLOAT:
                case Types::VECTOR_FLOAT: return readSequenceBulk<float>(is, result, size);
                case Types::ARRAY_DOUBLE:
                case Types::VECTOR_DOUBLE: return readSequenceBulk<double>(is, result, size);
                case Types::VECTOR_COMPLEX_FLOAT: return readSequence<std::complex<float> >(is, result, size);
                case Types::VECTOR_COMPLEX_DOUBLE: return readSequence<std::complex<double> >(is, result, size);
                case Types::VECTOR_HASH: return readSequence<Hash > (is, result, size);
                case Types::VECTOR_NONE: return readSequence<CppNone > (is, result, size);
                default:
                    throw KARABO_IO_EXCEPTION("Encountered unknown array data type whilst reading from binary archive");
            }
        }


        unsigned HashBinarySerializerFast::readSize(std::istream& is) {
            unsigned size;
            is.read((char*) &size, sizeof (size));
            return size;
        }


        std::string HashBinarySerializerFast::readKey(std::istream& is) {
            char buffer[256]; //memset(buffer, 0, 256);
            unsigned char size = 0;
            is.read((char*) &size, sizeof (size));
            is.read(buffer, size);
            buffer[size] = 0;
            return std::string(buffer);
        }


        Types::ReferenceType HashBinarySerializerFast::readType(std::istream& is) {
            return Types::ReferenceType(readSize(is));
        }


        void HashBinarySerializerFast::save(const std::vector<karabo::util::Hash>& objects, std::vector<char>& archive) {
            Hash tmp("KRB_Sequence", objects);
            save(tmp, archive);
        }


        void HashBinarySerializerFast::load(std::vector<karabo::util::Hash>& objects, const char* archive, const size_t nBytes) {
            vector<Hash> tmp(1);
            load(tmp[0], archive, nBytes);
            if (tmp[0].begin()->getKey() == "KRB_Sequence") {
                objects.swap(tmp[0].get<vector<Hash> >("KRB_Sequence"));
            } else {
                objects.swap(tmp);
            }
        }
    }
}
