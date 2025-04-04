/*
 * $Id$
 *
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */


#include "HashBinarySerializer.hh"

#include <boost/shared_array.hpp>

#include "SchemaBinarySerializer.hh"

using namespace karabo::data;
using namespace std;

KARABO_EXPLICIT_TEMPLATE(karabo::io::BinarySerializer<karabo::data::Hash>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::BinarySerializer<Hash>, karabo::io::HashBinarySerializer);

namespace karabo {
    namespace io {


        void HashBinarySerializer::expectedParameters(karabo::data::Schema& expected) {}


        HashBinarySerializer::HashBinarySerializer(const karabo::data::Hash& input) {}


        void HashBinarySerializer::save(const karabo::data::Hash& object, std::vector<char>& buffer) {
            buffer.resize(0);
            writeHash(object, buffer);
        }


        void HashBinarySerializer::save2(const karabo::data::Hash& object, std::vector<char>& buffer) {
            writeHash(object, buffer);
        }


        void HashBinarySerializer::save(const karabo::data::Hash& object, BufferSet& buffers) {
            buffers.clear();
            writeHash(object, buffers);
            buffers.updateSize();
            buffers.rewind();
        }


        void HashBinarySerializer::writeHash(const karabo::data::Hash& hash, std::vector<char>& buffer) const {
            writeSize(buffer, hash.size());
            for (Hash::const_iterator iter = hash.begin(); iter != hash.end(); ++iter) {
                writeNode(*iter, buffer);
            }
        }

        void HashBinarySerializer::writeHash(const karabo::data::Hash& hash, BufferSet& buffers) const {
            writeSize(buffers.back(), hash.size());
            for (Hash::const_iterator iter = hash.begin(); iter != hash.end(); ++iter) {
                writeNodeMultiBuffer(*iter, buffers);
            }
        }


        void HashBinarySerializer::writeSize(std::vector<char>& buffer, const unsigned int size) const {
            writeSingleValue(buffer, size);
        }


        void HashBinarySerializer::writeNode(const karabo::data::Hash::Node& element, std::vector<char>& buffer) const {
            const string& key = element.getKey();
            writeKey(buffer, key);
            if (element.is<Hash>()) {
                writeType(buffer, Types::HASH);
                writeAttributes(element.getAttributes(), buffer);
                writeHash(element.getValue<Hash>(), buffer);
            } else if (element.is<Hash::Pointer>()) {
                writeType(buffer, Types::HASH_POINTER);
                writeAttributes(element.getAttributes(), buffer);
                writeHash(*(element.getValue<Hash::Pointer>()), buffer);
            } else if (element.is<vector<Hash>>()) {
                writeType(buffer, Types::VECTOR_HASH);
                writeAttributes(element.getAttributes(), buffer);
                const vector<Hash>& tmp = element.getValue<vector<Hash>>();
                writeSize(buffer, tmp.size());
                for (size_t i = 0; i < tmp.size(); ++i) {
                    writeHash(tmp[i], buffer);
                }
            } else if (element.is<vector<Hash::Pointer>>()) {
                writeType(buffer, Types::VECTOR_HASH_POINTER);
                writeAttributes(element.getAttributes(), buffer);
                const vector<Hash::Pointer>& tmp = element.getValue<vector<Hash::Pointer>>();
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

        void HashBinarySerializer::writeNodeMultiBuffer(const karabo::data::Hash::Node& element,
                                                        BufferSet& buffers) const {
            const string& key = element.getKey();
            writeKey(buffers.back(), key);
            if (element.is<Hash>()) {
                writeType(buffers.back(), Types::HASH);
                writeAttributes(element.getAttributes(), buffers.back());
                writeHash(element.getValue<Hash>(), buffers);
            } else if (element.is<Hash::Pointer>()) {
                writeType(buffers.back(), Types::HASH_POINTER);
                writeAttributes(element.getAttributes(), buffers.back());
                writeHash(*(element.getValue<Hash::Pointer>()), buffers);
            } else if (element.is<vector<Hash>>()) {
                writeType(buffers.back(), Types::VECTOR_HASH);
                writeAttributes(element.getAttributes(), buffers.back());
                const vector<Hash>& tmp = element.getValue<vector<Hash>>();
                writeSize(buffers.back(), tmp.size());
                for (size_t i = 0; i < tmp.size(); ++i) {
                    writeHash(tmp[i], buffers);
                }
            } else if (element.is<vector<Hash::Pointer>>()) {
                writeType(buffers.back(), Types::VECTOR_HASH_POINTER);
                writeAttributes(element.getAttributes(), buffers.back());
                const vector<Hash::Pointer>& tmp = element.getValue<vector<Hash::Pointer>>();
                writeSize(buffers.back(), tmp.size());
                for (size_t i = 0; i < tmp.size(); ++i) {
                    writeHash(*(tmp[i]), buffers);
                }
            } else {
                writeType(buffers.back(), element.getType());
                writeAttributes(element.getAttributes(), buffers.back());
                writeAny(element.getValueAsAny(), element.getType(), buffers);
            }
        }


        void HashBinarySerializer::writeKey(std::vector<char>& buffer, const std::string& str) const {
            // ATTENTION: Some optimization takes place here, the size indicator for a key is limited to 1 byte
            // instead of the generic 4 byte for everything else!!
            if (str.size() > 255) {
                throw KARABO_IO_EXCEPTION("Could not serialize key \"" + str + "\" of length " +
                                          std::to_string(str.size()) + ": over 255 bytes");
            }
            const unsigned char size = str.size();
            writeSingleValue(buffer, size);
            const size_t pos = buffer.size();
            buffer.resize(pos + size);
            std::memcpy(&buffer[pos], str.c_str(), size);
        }


        void HashBinarySerializer::writeType(std::vector<char>& buffer, const Types::ReferenceType type) const {
            return writeSingleValue(buffer, static_cast<unsigned int>(type));
        }


        void HashBinarySerializer::writeAttributes(const karabo::data::Hash::Attributes& attributes,
                                                   std::vector<char>& buffer) const {
            writeSize(buffer, attributes.size());
            for (Hash::Attributes::const_iterator iter = attributes.begin(); iter != attributes.end(); ++iter) {
                const Element<std::string>& attr = *iter;
                writeKey(buffer, attr.getKey());
                const Types::ReferenceType type = attr.getType();
                writeType(buffer, type);
                writeAny(attr.getValueAsAny(), type, buffer);
            }
        }


        void HashBinarySerializer::writeAny(const std::any& value, const karabo::data::Types::ReferenceType type,
                                            std::vector<char>& buffer) const {
            switch (Types::category(type)) {
                case Types::SCHEMA:
                case Types::HASH:
                case Types::SIMPLE:
                    return writeSingleValue(buffer, value, type);
                case Types::VECTOR_HASH:
                case Types::SEQUENCE:
                    return writeSequence(buffer, value, type);
                default:
                    throw KARABO_IO_EXCEPTION("Could not properly categorize value type \"" +
                                              Types::to<ToLiteral>(type) + "\" for writing to archive");
            }
        }

        void HashBinarySerializer::writeAny(const std::any& value, const karabo::data::Types::ReferenceType type,
                                            BufferSet& buffers) const {
            switch (Types::category(type)) {
                case Types::SCHEMA:
                case Types::HASH:
                case Types::SIMPLE:
                    return writeSingleValue(buffers, value, type);
                case Types::VECTOR_HASH:
                case Types::SEQUENCE:
                    return writeSequence(buffers.back(), value, type);
                default:
                    throw KARABO_IO_EXCEPTION("Could not properly categorize value type \"" +
                                              Types::to<ToLiteral>(type) + "\" for writing to archive");
            }
        }


        void HashBinarySerializer::writeSingleValue(std::vector<char>& buffer, const std::any& value,
                                                    const Types::ReferenceType type) const {
            switch (type) {
                case Types::CHAR:
                    return writeSingleValue(buffer, std::any_cast<const char&>(value));
                case Types::INT8:
                    return writeSingleValue(buffer, std::any_cast<const signed char&>(value));
                case Types::INT16:
                    return writeSingleValue(buffer, std::any_cast<const short&>(value));
                case Types::INT32:
                    return writeSingleValue(buffer, std::any_cast<const int&>(value));
                case Types::INT64:
                    return writeSingleValue(buffer, std::any_cast<const long long&>(value));
                case Types::UINT8:
                    return writeSingleValue(buffer, std::any_cast<const unsigned char&>(value));
                case Types::UINT16:
                    return writeSingleValue(buffer, std::any_cast<const unsigned short&>(value));
                case Types::UINT32:
                    return writeSingleValue(buffer, std::any_cast<const unsigned int&>(value));
                case Types::UINT64:
                    return writeSingleValue(buffer, std::any_cast<const unsigned long long&>(value));
                case Types::FLOAT:
                    return writeSingleValue(buffer, std::any_cast<const float&>(value));
                case Types::DOUBLE:
                    return writeSingleValue(buffer, std::any_cast<const double&>(value));
                case Types::BOOL:
                    return writeSingleValue(buffer, std::any_cast<const bool&>(value));
                case Types::COMPLEX_FLOAT:
                    return writeSingleValue(buffer, std::any_cast<const std::complex<float>&>(value));
                case Types::COMPLEX_DOUBLE:
                    return writeSingleValue(buffer, std::any_cast<const std::complex<double>&>(value));
                case Types::STRING:
                    return writeSingleValue(buffer, std::any_cast<const std::string&>(value)); //
                case Types::SCHEMA:
                    return writeSingleValue(buffer, std::any_cast<const Schema&>(value));
                case Types::HASH:
                    return writeSingleValue(buffer, std::any_cast<const Hash&>(value));
                case Types::NONE:
                    return writeSingleValue(buffer, std::any_cast<const CppNone&>(value));
                case Types::BYTE_ARRAY:
                    return writeSingleValue(buffer, std::any_cast<const ByteArray&>(value));
                default:
                    throw KARABO_IO_EXCEPTION("Encountered unknown data type while writing to binary archive");
            }
        }

        void HashBinarySerializer::writeSingleValue(BufferSet& buffers, const std::any& value,
                                                    const Types::ReferenceType type) const {
            switch (type) {
                case Types::CHAR:
                    return writeSingleValue(buffers.back(), std::any_cast<const char&>(value));
                case Types::INT8:
                    return writeSingleValue(buffers.back(), std::any_cast<const signed char&>(value));
                case Types::INT16:
                    return writeSingleValue(buffers.back(), std::any_cast<const short&>(value));
                case Types::INT32:
                    return writeSingleValue(buffers.back(), std::any_cast<const int&>(value));
                case Types::INT64:
                    return writeSingleValue(buffers.back(), std::any_cast<const long long&>(value));
                case Types::UINT8:
                    return writeSingleValue(buffers.back(), std::any_cast<const unsigned char&>(value));
                case Types::UINT16:
                    return writeSingleValue(buffers.back(), std::any_cast<const unsigned short&>(value));
                case Types::UINT32:
                    return writeSingleValue(buffers.back(), std::any_cast<const unsigned int&>(value));
                case Types::UINT64:
                    return writeSingleValue(buffers.back(), std::any_cast<const unsigned long long&>(value));
                case Types::FLOAT:
                    return writeSingleValue(buffers.back(), std::any_cast<const float&>(value));
                case Types::DOUBLE:
                    return writeSingleValue(buffers.back(), std::any_cast<const double&>(value));
                case Types::BOOL:
                    return writeSingleValue(buffers.back(), std::any_cast<const bool&>(value));
                case Types::COMPLEX_FLOAT:
                    return writeSingleValue(buffers.back(), std::any_cast<const std::complex<float>&>(value));
                case Types::COMPLEX_DOUBLE:
                    return writeSingleValue(buffers.back(), std::any_cast<const std::complex<double>&>(value));
                case Types::STRING:
                    return writeSingleValue(buffers.back(), std::any_cast<const std::string&>(value)); //
                case Types::SCHEMA:
                    return writeSingleValue(buffers.back(), std::any_cast<const Schema&>(value));
                case Types::HASH:
                    return writeSingleValue(buffers.back(), std::any_cast<const Hash&>(value));
                case Types::NONE:
                    return writeSingleValue(buffers.back(), std::any_cast<const CppNone&>(value));
                case Types::BYTE_ARRAY:
                    return writeSingleValue(buffers, std::any_cast<const ByteArray&>(value));
                default:
                    throw KARABO_IO_EXCEPTION("Encountered unknown data type while writing to binary archive");
            }
        }


        template <>
        void HashBinarySerializer::writeSingleValue(std::vector<char>& buffer, const std::string& str) const {
            writeRawArray(buffer, std::make_pair(str.c_str(), str.length()));
        }


        template <>
        void HashBinarySerializer::writeSingleValue(std::vector<char>& buffer, const std::complex<float>& value) const {
            return writeComplexValue(buffer, value);
        }


        template <>
        void HashBinarySerializer::writeSingleValue(std::vector<char>& buffer,
                                                    const std::complex<double>& value) const {
            return writeComplexValue(buffer, value);
        }


        template <>
        void HashBinarySerializer::writeSingleValue(std::vector<char>& buffer, const Schema& schema) const {
            Hash hash;
            SchemaBinarySerializer serializer /* = SchemaBinarySerializer*/ (hash);

            // Prepare buffer - old versions of this serialised the schema into a new 'std::vector<char>'
            // and, instead of just appending it, used writeSequenceBulk to add it to 'buffer'.
            // That does first write the size of the schema archive. To stay compatible, foresee a size
            // now and set it later.
            const size_t oldSize = buffer.size();
            buffer.resize(buffer.size() + sizeof(unsigned int)); // sizes are stored in unsigned int

            // Directly append to buffer
            serializer.save2(schema, buffer);

            // Fix-up buffer, i.e. write raw schema size for compatibility with old writeSequenceBulk version
            const unsigned int rawSchemaSize =
                  static_cast<unsigned int>(buffer.size() - (oldSize + sizeof(unsigned int)));
            const char* rawSchemaSizeBytes = reinterpret_cast<const char*>(&rawSchemaSize);
            memcpy(&buffer[oldSize], rawSchemaSizeBytes, sizeof(rawSchemaSize));
        }


        template <>
        void HashBinarySerializer::writeSingleValue(std::vector<char>& buffer, const Hash& hash) const {
            writeHash(hash, buffer);
        }


        template <>
        void HashBinarySerializer::writeSingleValue(std::vector<char>& buffer, const CppNone& value) const {
            writeSize(buffer, 0);
        }


        template <>
        void HashBinarySerializer::writeSingleValue(std::vector<char>& buffer, const ByteArray& value) const {
            writeSize(buffer, static_cast<unsigned int>(value.second));
            const char* src = value.first.get();
            const size_t n = value.second;
            const size_t pos = buffer.size();
            buffer.resize(pos + n);
            std::memcpy(&buffer[pos], src, n);
        }


        void HashBinarySerializer::writeSequence(std::vector<char>& buffer, const std::any& value,
                                                 const Types::ReferenceType type) const {
            switch (type) {
                case Types::VECTOR_CHAR:
                    return writeSequenceBulk(buffer, std::any_cast<const vector<char>&>(value));
                case Types::VECTOR_INT8:
                    return writeSequenceBulk(buffer, std::any_cast<const vector<signed char>&>(value));
                case Types::VECTOR_INT16:
                    return writeSequenceBulk(buffer, std::any_cast<const vector<short>&>(value));
                case Types::VECTOR_INT32:
                    return writeSequenceBulk(buffer, std::any_cast<const vector<int>&>(value));
                case Types::VECTOR_INT64:
                    return writeSequenceBulk(buffer, std::any_cast<const vector<long long>&>(value));
                case Types::VECTOR_UINT8:
                    return writeSequenceBulk(buffer, std::any_cast<const vector<unsigned char>&>(value));
                case Types::VECTOR_UINT16:
                    return writeSequenceBulk(buffer, std::any_cast<const vector<unsigned short>&>(value));
                case Types::VECTOR_UINT32:
                    return writeSequenceBulk(buffer, std::any_cast<const vector<unsigned int>&>(value));
                case Types::VECTOR_UINT64:
                    return writeSequenceBulk(buffer, std::any_cast<const vector<unsigned long long>&>(value));
                case Types::VECTOR_FLOAT:
                    return writeSequenceBulk(buffer, std::any_cast<const vector<float>&>(value));
                case Types::VECTOR_DOUBLE:
                    return writeSequenceBulk(buffer, std::any_cast<const vector<double>&>(value));
                case Types::VECTOR_COMPLEX_FLOAT:
                    return writeSequence(buffer, std::any_cast<const vector<std::complex<float>>&>(value));
                case Types::VECTOR_COMPLEX_DOUBLE:
                    return writeSequence(buffer, std::any_cast<const vector<std::complex<double>>&>(value));
                case Types::VECTOR_STRING:
                    return writeSequence(buffer, std::any_cast<const vector<std::string>&>(value));
                case Types::VECTOR_BOOL:
                    return writeSequence(buffer, std::any_cast<const vector<bool>&>(value));
                case Types::VECTOR_HASH:
                    return writeSequence(buffer, std::any_cast<const vector<Hash>&>(value));
                case Types::VECTOR_NONE:
                    return writeSequence(buffer, std::any_cast<const vector<CppNone>&>(value));
                default:
                    throw KARABO_IO_EXCEPTION("Encountered unknown array data type whilst writing to binary archive");
            }
        }


        size_t HashBinarySerializer::load(karabo::data::Hash& object, const char* archive, const size_t nBytes) {
            std::stringstream is;
            is.rdbuf()->pubsetbuf(const_cast<char*>(archive), nBytes);
            this->readHash(object, is);
            return size_t(is.tellg());
        }

        void HashBinarySerializer::load(karabo::data::Hash& object, const BufferSet& buffers) {
            buffers.rewind();
            std::stringstream is;
            is.rdbuf()->pubsetbuf(buffers.current().data(), buffers.current().size());
            this->readHash(object, is, buffers);
            buffers.rewind();
        }

        void HashBinarySerializer::readHash(Hash& hash, std::istream& is) const {
            unsigned size = readSize(is);
            for (unsigned i = 0; i < size; ++i) {
                std::string name = readKey(is);
                Hash::Node& node = hash.set(name, true); // The boolean is a dummy to allow working on references later
                readNode(node, is);
            }
        }

        void HashBinarySerializer::readHash(Hash& hash, std::istream& is, const BufferSet& buffers) const {
            unsigned size = readSize(is);
            for (unsigned i = 0; i < size; ++i) {
                nextBufIfEos(is, buffers);
                std::string name = readKey(is);
                Hash::Node& node = hash.set(name, true); // The boolean is a dummy to allow working on references later
                readNode(node, is, buffers);
            }
        }


        void HashBinarySerializer::readNode(Hash::Node& node, std::istream& is) const {
            Types::ReferenceType type = readType(is);
            readAttributes(node.getAttributes(), is);

            if (type == Types::HASH) {
                node.setValue(Hash());
                Hash& tmp = node.getValue<Hash>();
                readHash(tmp, is);
            } else if (type == Types::HASH_POINTER) {
                node.setValue(Hash::Pointer(new Hash()));
                Hash& tmp = *(node.getValue<Hash::Pointer>());
                readHash(tmp, is);
            } else if (type == Types::VECTOR_HASH) {
                const size_t size = readSize(is);
                node.setValue(std::vector<Hash>());
                std::vector<Hash>& result = node.getValue<std::vector<Hash>>();
                result.resize(size);
                for (size_t i = 0; i < size; ++i) {
                    readHash(result[i], is);
                }
            } else if (type == Types::VECTOR_HASH_POINTER) {
                const size_t size = readSize(is);
                node.setValue(std::vector<Hash::Pointer>());
                std::vector<Hash::Pointer>& result = node.getValue<std::vector<Hash::Pointer>>();
                result.resize(size);
                for (size_t i = 0; i < size; ++i) {
                    result[i].reset(new Hash());
                    readHash(*(result[i]), is);
                }
            } else {
                readAny(node.getValueAsAny(), type, is);
            }
        }


        void HashBinarySerializer::readNode(Hash::Node& node, std::istream& is, const BufferSet& buffers) const {
            Types::ReferenceType type = readType(is);
            readAttributes(node.getAttributes(), is);
            if (type == Types::HASH) {
                node.setValue(Hash());
                Hash& tmp = node.getValue<Hash>();
                readHash(tmp, is, buffers);
            } else if (type == Types::HASH_POINTER) {
                node.setValue(Hash::Pointer(new Hash()));
                Hash& tmp = *(node.getValue<Hash::Pointer>());
                readHash(tmp, is, buffers);
            } else if (type == Types::VECTOR_HASH) {
                const size_t size = readSize(is);
                node.setValue(std::vector<Hash>());
                std::vector<Hash>& result = node.getValue<std::vector<Hash>>();
                result.resize(size);
                for (size_t i = 0; i < size; ++i) {
                    readHash(result[i], is, buffers);
                }
            } else if (type == Types::VECTOR_HASH_POINTER) {
                const size_t size = readSize(is);
                node.setValue(std::vector<Hash::Pointer>());
                std::vector<Hash::Pointer>& result = node.getValue<std::vector<Hash::Pointer>>();
                result.resize(size);
                for (size_t i = 0; i < size; ++i) {
                    result[i].reset(new Hash());
                    readHash(*(result[i]), is, buffers);
                }
            } else {
                readAny(node.getValueAsAny(), type, is, buffers);
            }
        }


        void HashBinarySerializer::readAttributes(Hash::Attributes& attributes, std::istream& is) const {
            unsigned size = readSize(is);
            for (unsigned i = 0; i < size; ++i) {
                std::string name = readKey(is);
                Types::ReferenceType type = readType(is);
                std::any value;
                readAny(value, type, is);
                attributes.set(name, std::move(value));
            }
        }


        void HashBinarySerializer::readAny(std::any& value, const Types::ReferenceType type, std::istream& is) const {
            switch (Types::category(type)) {
                case Types::SCHEMA:
                case Types::SIMPLE:
                    readSingleValue(is, value, type);
                    return;
                case Types::SEQUENCE:
                    readSequence(is, value, type);
                    return;
                case Types::HASH:
                    readHash(std::any_cast<Hash&>(value), is);
                    return;
                case Types::VECTOR_HASH: {
                    unsigned size = readSize(is);
                    value = std::vector<Hash>();
                    std::vector<Hash>& result = std::any_cast<std::vector<Hash>&>(value);
                    result.resize(size);
                    for (unsigned i = 0; i < size; ++i) {
                        readHash(result[i], is);
                    }
                    return;
                }
                default:
                    throw KARABO_IO_EXCEPTION("Could not properly categorize value \"" + Types::to<ToLiteral>(type) +
                                              "\" for reading from archive");
            }
        }

        void HashBinarySerializer::readAny(std::any& value, const Types::ReferenceType type, std::istream& is,
                                           const BufferSet& buffers) const {
            switch (Types::category(type)) {
                case Types::SCHEMA:
                case Types::SIMPLE:
                    readSingleValue(is, value, type, buffers);
                    return;
                case Types::SEQUENCE:
                    readSequence(is, value, type);
                    return;
                case Types::HASH:
                    readHash(std::any_cast<Hash&>(value), is, buffers);
                    return;
                case Types::VECTOR_HASH: {
                    unsigned size = readSize(is);
                    value = std::vector<Hash>();
                    std::vector<Hash>& result = std::any_cast<std::vector<Hash>&>(value);
                    result.resize(size);
                    for (unsigned i = 0; i < size; ++i) {
                        readHash(result[i], is, buffers);
                    }
                    return;
                }
                default:
                    throw KARABO_IO_EXCEPTION("Could not properly categorize value \"" + Types::to<ToLiteral>(type) +
                                              "\" for reading from archive");
            }
        }


        template <>
        std::string HashBinarySerializer::readSingleValue(std::istream& is) const {
            const unsigned int size = readSize(is);
            std::string result;
            if (size) {
                result.resize(size); // unfortunately, all characters are initialised with null although not needed
                // Further optimisation:
                // With access to raw character pointers inside 'is', one could directly create the string with an
                // interator range and get rid of the null initialisation overhead: return string(rawPtr, rawPtr + size)
                is.read(&(result[0]), size);
            }
            return result;
        }


        template <>
        Schema HashBinarySerializer::readSingleValue(std::istream& is) const {
            Hash hash;
            SchemaBinarySerializer serializer(hash);
            // TODO Optimize this by reading directly from istream
            const unsigned int size = readSize(is);
            Schema schema;
            if (size) {
                std::unique_ptr<char> buffer(new char[size]);
                is.read(buffer.get(), size);
                serializer.load(schema, buffer.get(), size);
            }
            return schema;
        }


        template <>
        std::complex<double> HashBinarySerializer::readSingleValue(std::istream& is) const {
            return readComplexValue<double>(is);
        }


        template <>
        std::complex<float> HashBinarySerializer::readSingleValue(std::istream& is) const {
            return readComplexValue<float>(is);
        }


        template <>
        Hash HashBinarySerializer::readSingleValue(std::istream& is) const {
            Hash hash;
            readHash(hash, is);
            return hash;
        }


        template <>
        karabo::data::CppNone HashBinarySerializer::readSingleValue(std::istream& is) const {
            unsigned size = readSize(is);
            if (size != 0)
                throw KARABO_IO_EXCEPTION(
                      "Encountered not 'None' data type whilst reading from binary archive: size is " + toString(size) +
                      ", but should be 0");
            return karabo::data::CppNone();
        }


        template <>
        karabo::data::ByteArray HashBinarySerializer::readSingleValue(std::istream& is) const {
            const size_t size = readSize(is);
            ByteArray result(std::shared_ptr<char>(new char[size], std::default_delete<char[]>()), size);
            is.read(result.first.get(), size);
            return result;
        }

        karabo::data::ByteArray HashBinarySerializer::readByteArrayAsCopy(std::istream& is, size_t size) const {
            ByteArray result(std::shared_ptr<char>(new char[size], std::default_delete<char[]>()), size);
            is.read(result.first.get(), size);
            return result;
        }


        void HashBinarySerializer::readSingleValue(std::istream& is, std::any& value,
                                                   const Types::ReferenceType type) const {
            switch (type) {
                case Types::CHAR:
                    value = readSingleValue<char>(is);
                    break;
                case Types::INT8:
                    value = readSingleValue<signed char>(is);
                    break;
                case Types::INT16:
                    value = readSingleValue<short>(is);
                    break;
                case Types::INT32:
                    value = readSingleValue<int>(is);
                    break;
                case Types::INT64:
                    value = readSingleValue<long long>(is);
                    break;
                case Types::UINT8:
                    value = readSingleValue<unsigned char>(is);
                    break;
                case Types::UINT16:
                    value = readSingleValue<unsigned short>(is);
                    break;
                case Types::UINT32:
                    value = readSingleValue<unsigned int>(is);
                    break;
                case Types::UINT64:
                    value = readSingleValue<unsigned long long>(is);
                    break;
                case Types::FLOAT:
                    value = readSingleValue<float>(is);
                    break;
                case Types::DOUBLE:
                    value = readSingleValue<double>(is);
                    break;
                case Types::BOOL:
                    value = readSingleValue<bool>(is);
                    break;
                case Types::COMPLEX_FLOAT:
                    value = readSingleValue<std::complex<float>>(is);
                    break;
                case Types::COMPLEX_DOUBLE:
                    value = readSingleValue<std::complex<double>>(is);
                    break;
                case Types::STRING:
                    value = readSingleValue<std::string>(is);
                    break;
                case Types::BYTE_ARRAY:
                    value = readSingleValue<ByteArray>(is);
                    break;
                case Types::SCHEMA:
                    value = readSingleValue<Schema>(is);
                    break;
                case Types::HASH:
                    value = readSingleValue<Hash>(is);
                    break;
                case Types::NONE:
                    value = readSingleValue<CppNone>(is);
                    break;
                default:
                    throw KARABO_IO_EXCEPTION("Encountered unknown data type whilst reading from binary archive");
            }
        }

        void HashBinarySerializer::readSingleValue(std::istream& is, std::any& value, const Types::ReferenceType type,
                                                   const BufferSet& buffers) const {
            switch (type) {
                case Types::CHAR:
                    value = readSingleValue<char>(is);
                    break;
                case Types::INT8:
                    value = readSingleValue<signed char>(is);
                    break;
                case Types::INT16:
                    value = readSingleValue<short>(is);
                    break;
                case Types::INT32:
                    value = readSingleValue<int>(is);
                    break;
                case Types::INT64:
                    value = readSingleValue<long long>(is);
                    break;
                case Types::UINT8:
                    value = readSingleValue<unsigned char>(is);
                    break;
                case Types::UINT16:
                    value = readSingleValue<unsigned short>(is);
                    break;
                case Types::UINT32:
                    value = readSingleValue<unsigned int>(is);
                    break;
                case Types::UINT64:
                    value = readSingleValue<unsigned long long>(is);
                    break;
                case Types::FLOAT:
                    value = readSingleValue<float>(is);
                    break;
                case Types::DOUBLE:
                    value = readSingleValue<double>(is);
                    break;
                case Types::BOOL:
                    value = readSingleValue<bool>(is);
                    break;
                case Types::COMPLEX_FLOAT:
                    value = readSingleValue<std::complex<float>>(is);
                    break;
                case Types::COMPLEX_DOUBLE:
                    value = readSingleValue<std::complex<double>>(is);
                    break;
                case Types::STRING:
                    value = readSingleValue<std::string>(is);
                    break;
                case Types::BYTE_ARRAY: {
                    const size_t size = readSize(is);
                    nextBufIfEos(is, buffers);
                    if (!buffers.currentIsByteArrayCopy()) {
                        value = buffers.currentAsByteArray();
                        // switch to the next buffer and set stream to it.
                        buffers.next();
                        auto& cb = buffers.current();
                        is.rdbuf()->pubsetbuf(cb.data(), cb.size());
                        is.rdbuf()->pubseekpos(0);
                    } else { // bytearray wasn't separated
                        value = readByteArrayAsCopy(is, size);
                    }
                    break;
                }
                case Types::SCHEMA:
                    value = readSingleValue<Schema>(is);
                    break;
                case Types::HASH:
                    value = readSingleValue<Hash>(is);
                    break;
                case Types::NONE:
                    value = readSingleValue<CppNone>(is);
                    break;
                default:
                    throw KARABO_IO_EXCEPTION("Encountered unknown data type whilst reading from binary archive");
            }
        }


        void HashBinarySerializer::readSequence(std::istream& is, std::any& result,
                                                const Types::ReferenceType type) const {
            unsigned size = readSize(is);
            switch (type) {
                case Types::VECTOR_BOOL:
                    return readSequence<bool>(is, result, size);
                case Types::VECTOR_STRING:
                    return readSequence<std::string>(is, result, size);
                case Types::VECTOR_CHAR:
                    return readSequenceBulk<char>(is, result, size);
                case Types::VECTOR_INT8:
                    return readSequenceBulk<signed char>(is, result, size);
                case Types::VECTOR_INT16:
                    return readSequenceBulk<short>(is, result, size);
                case Types::VECTOR_INT32:
                    return readSequenceBulk<int>(is, result, size);
                case Types::VECTOR_INT64:
                    return readSequenceBulk<long long>(is, result, size);
                case Types::VECTOR_UINT8:
                    return readSequenceBulk<unsigned char>(is, result, size);
                case Types::VECTOR_UINT16:
                    return readSequenceBulk<unsigned short>(is, result, size);
                case Types::VECTOR_UINT32:
                    return readSequenceBulk<unsigned int>(is, result, size);
                case Types::VECTOR_UINT64:
                    return readSequenceBulk<unsigned long long>(is, result, size);
                case Types::VECTOR_FLOAT:
                    return readSequenceBulk<float>(is, result, size);
                case Types::VECTOR_DOUBLE:
                    return readSequenceBulk<double>(is, result, size);
                case Types::VECTOR_COMPLEX_FLOAT:
                    return readSequence<std::complex<float>>(is, result, size);
                case Types::VECTOR_COMPLEX_DOUBLE:
                    return readSequence<std::complex<double>>(is, result, size);
                case Types::VECTOR_HASH:
                    return readSequence<Hash>(is, result, size);
                case Types::VECTOR_NONE:
                    return readSequence<CppNone>(is, result, size);
                default:
                    throw KARABO_IO_EXCEPTION("Encountered unknown array data type whilst reading from binary archive");
            }
        }


        unsigned HashBinarySerializer::readSize(std::istream& is) {
            unsigned size;
            is.read((char*)&size, sizeof(size));
            return size;
        }


        std::string HashBinarySerializer::readKey(std::istream& is) {
            char buffer[256]; // memset(buffer, 0, 256);
            unsigned char size = 0;
            is.read((char*)&size, sizeof(size));
            is.read(buffer, size);
            buffer[size] = 0;
            return std::string(buffer);
        }


        Types::ReferenceType HashBinarySerializer::readType(std::istream& is) const {
            return Types::ReferenceType(readSize(is));
        }


        void HashBinarySerializer::save(const std::vector<karabo::data::Hash>& objects, std::vector<char>& archive) {
            Hash tmp("KRB_Sequence", objects);
            save(tmp, archive);
        }


        size_t HashBinarySerializer::load(std::vector<karabo::data::Hash>& objects, const char* archive,
                                          const size_t nBytes) {
            vector<Hash> tmp(1);
            size_t bytes = load(tmp[0], archive, nBytes);
            if (tmp[0].begin()->getKey() == "KRB_Sequence") {
                objects.swap(tmp[0].get<vector<Hash>>("KRB_Sequence"));
            } else {
                objects.swap(tmp);
            }
            return bytes;
        }
    } // namespace io
} // namespace karabo
