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

#ifndef KARABO_IO_HASHBINARYSERIALIZER_HH
#define KARABO_IO_HASHBINARYSERIALIZER_HH

#include <iostream>

#include "BinarySerializer.hh"
#include "BufferSet.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    namespace io {

        /**
         * @class HashBinarySerializer
         * @brief The HashBinarySerializer provides an implementation of BinarySerializer
         *        for the karabo::util::Hash
         *
         * While a karabo::util::Hash can in principle hold arbitrary data types, Hash
         * serialization is limited to data types known to the karabo::util::Types type
         * system. Hashes containing other data types will lead to exceptions during
         * serialization.
         */
        class HashBinarySerializer : public BinarySerializer<karabo::util::Hash> {
           public:
            KARABO_CLASSINFO(HashBinarySerializer, "Bin", "1.0")

            static void expectedParameters(karabo::util::Schema& expected);

            HashBinarySerializer(const karabo::util::Hash& input);

            /**
             * Save a Hash to a binary archive
             * @param object to save
             * @param archive to save to - buffer.clear() will be called first
             */
            virtual void save(const karabo::util::Hash& object, std::vector<char>& buffer);

            /**
             * Save a Hash by appending it to a binary archive
             * @param object to save
             * @param archive to append to - no clear() called
             */
            virtual void save2(const karabo::util::Hash& object, std::vector<char>& buffer);

            virtual void save(const karabo::util::Hash& object, BufferSet& buffers);

            virtual size_t load(karabo::util::Hash& object, const char* archive, const size_t nBytes);

            virtual void load(karabo::util::Hash& object, const BufferSet& buffers);

            void save(const std::vector<karabo::util::Hash>& objects, std::vector<char>& archive);

            size_t load(std::vector<karabo::util::Hash>& objects, const char* archive, const size_t nBytes);

            /**
             * Destructor.
             */
            virtual ~HashBinarySerializer(){};


           private: // members
           private: // functions
            void writeHash(const karabo::util::Hash& hash, std::vector<char>& buffer) const;
            void writeHash(const karabo::util::Hash& hash, BufferSet& buffers) const;
            void writeNode(const karabo::util::Hash::Node& element, std::vector<char>& buffer) const;
            void writeNodeMultiBuffer(const karabo::util::Hash::Node& element, BufferSet& buffers) const;
            void writeAttributes(const karabo::util::Hash::Attributes& attributes, std::vector<char>& buffer) const;
            void writeAny(const std::any& value, const karabo::util::Types::ReferenceType type,
                          std::vector<char>& buffer) const;
            void writeAny(const std::any& value, const karabo::util::Types::ReferenceType type,
                          BufferSet& buffers) const;

            template <typename T>
            inline void writeSingleValue(std::vector<char>& buffer, const T& value) const {
                const char* src = reinterpret_cast<const char*>(&value);
                const size_t n = sizeof(value);
                const size_t pos = buffer.size();
                buffer.resize(pos + n);
                std::memcpy(&buffer[pos], src, n);
            }

            template <typename T>
            inline void writeSingleValue(BufferSet& buffers, const T& value) const {
                const char* src = reinterpret_cast<const char*>(&value);
                const size_t n = sizeof(value);
                const size_t pos = buffers.back().size();
                buffers.back().resize(pos + n);
                std::memcpy(&(buffers.back())[pos], src, n);
            }


            inline void writeSingleValue(BufferSet& buffers, const karabo::util::ByteArray& value) const {
                buffers.emplaceBack(value);
            }

            template <typename T>
            inline void writeComplexValue(std::vector<char>& buffer, const std::complex<T>& value) const {
                writeSingleValue(buffer, value.real());
                writeSingleValue(buffer, value.imag());
            }

            template <typename T>
            inline void writeSequenceBulk(std::vector<char>& buffer, const std::vector<T>& vect) const {
                writeSize(buffer, static_cast<unsigned int>(vect.size()));
                const char* src = reinterpret_cast<const char*>(&vect[0]);
                const size_t n = vect.size() * sizeof(T);
                const size_t pos = buffer.size();
                buffer.resize(pos + n);
                std::memcpy(&buffer[pos], src, n);
            }

            template <typename T>
            inline void writeSequence(std::vector<char>& buffer, const std::vector<T>& vect) const {
                writeSize(buffer, static_cast<unsigned int>(vect.size()));
                for (unsigned i = 0; i < vect.size(); ++i) {
                    writeSingleValue(buffer, vect[i]);
                }
            }

            template <typename T>
            inline void writeRawArray(std::vector<char>& buffer, const std::pair<const T*, size_t>& raw) const {
                writeSize(buffer, static_cast<unsigned int>(raw.second));
                const char* src = reinterpret_cast<const char*>(raw.first);
                const size_t n = raw.second * sizeof(T);
                const size_t pos = buffer.size();
                buffer.resize(pos + n);
                std::memcpy(&buffer[pos], src, n);
            }

            inline void writeSingleValue(std::vector<char>& buffer, const std::any&,
                                         const karabo::util::Types::ReferenceType type) const;

            inline void writeSingleValue(BufferSet& buffers, const std::any&,
                                         const karabo::util::Types::ReferenceType type) const;

            inline void writeSequence(std::vector<char>& buffer, const std::any&,
                                      const karabo::util::Types::ReferenceType type) const;

            inline void writeRawArray(std::vector<char>& buffer, const std::any&,
                                      const karabo::util::Types::ReferenceType type) const;

            inline void writeKey(std::vector<char>& buffer, const std::string& str) const;

            inline void writeType(std::vector<char>& buffer, const karabo::util::Types::ReferenceType type) const;

            inline void writeSize(std::vector<char>& buffer, const unsigned size) const;

            void readHash(karabo::util::Hash& hash, std::istream& is) const;

            void readHash(karabo::util::Hash& hash, std::istream& is, const BufferSet& buffers) const;

            void readNode(karabo::util::Hash::Node& element, std::istream& is) const;

            void readNode(karabo::util::Hash::Node& element, std::istream& is, const BufferSet& buffers) const;

            void readAttributes(karabo::util::Hash::Attributes& attributes, std::istream& is) const;

            void readAny(std::any& value, const karabo::util::Types::ReferenceType type, std::istream& is) const;

            void readAny(std::any& value, const karabo::util::Types::ReferenceType type, std::istream& is,
                         const BufferSet& buffers) const;

            inline bool nextBufIfEos(std::istream& is, const BufferSet& buffers) const {
                if (is.tellg() == -1 ||
                    (size_t)is.tellg() >=
                          buffers.current().size() - 1) { // buffers of zero size will also pass through in this way
                    if (buffers.next()) {
                        auto& cb = buffers.current();
                        is.rdbuf()->pubsetbuf(cb.data(), cb.size());
                        is.rdbuf()->pubseekpos(0);
                        return true;
                    }
                }
                return false;
            }

            template <typename T>
            inline T readSingleValue(std::istream& is) const {
                union {
                    char buffer[32];
                    T tbuffer[32 / sizeof(T)];
                } all;
                memset(all.buffer, 0, 32);
                is.read(all.buffer, sizeof(T));
                return all.tbuffer[0];
            }

            karabo::util::ByteArray readByteArrayAsCopy(std::istream& is, size_t size) const;


            template <typename T>
            inline std::complex<T> readComplexValue(std::istream& is) const {
                T const real = readSingleValue<T>(is);
                T const imag = readSingleValue<T>(is);
                return std::complex<T>(real, imag);
            }

            template <typename T>
            inline void readSequenceBulk(std::istream& is, std::any& value, unsigned size) const {
                value = std::vector<T>();
                std::vector<T>& result = std::any_cast<std::vector<T>&>(value);
                result.resize(size);
                is.read(reinterpret_cast<char*>(&result[0]), size * sizeof(T));
            }

            template <typename T>
            inline void readSequence(std::istream& is, std::any& value, unsigned size) const {
                value = std::vector<T>();
                std::vector<T>& result = std::any_cast<std::vector<T>&>(value);
                result.resize(size);
                for (unsigned i = 0; i < size; ++i) {
                    result[i] = readSingleValue<T>(is);
                }
            }

            inline void readSingleValue(std::istream& is, std::any& value,
                                        const karabo::util::Types::ReferenceType type) const;
            inline void readSingleValue(std::istream& is, std::any& value,
                                        const karabo::util::Types::ReferenceType type, const BufferSet& buffers) const;

            inline void readSequence(std::istream& is, std::any&, const karabo::util::Types::ReferenceType type) const;


            static inline unsigned readSize(std::istream& is);

            static inline std::string readKey(std::istream& is);

            inline karabo::util::Types::ReferenceType readType(std::istream& is) const;
        };

        template <>
        void HashBinarySerializer::writeSingleValue(std::vector<char>& buffer, const std::string&) const;

        template <>
        void HashBinarySerializer::writeSingleValue(std::vector<char>& buffer, const std::complex<float>&) const;

        template <>
        void HashBinarySerializer::writeSingleValue(std::vector<char>& buffer, const std::complex<double>&) const;

        template <>
        void HashBinarySerializer::writeSingleValue(std::vector<char>& buffer, const karabo::util::Schema&) const;

        template <>
        void HashBinarySerializer::writeSingleValue(std::vector<char>& buffer, const karabo::util::Hash&) const;

        template <>
        void HashBinarySerializer::writeSingleValue(std::vector<char>& buffer,
                                                    const karabo::util::CppNone& value) const;

        template <>
        void HashBinarySerializer::writeSingleValue(std::vector<char>& buffer,
                                                    const karabo::util::ByteArray& value) const;

        template <>
        std::string HashBinarySerializer::readSingleValue(std::istream& is) const;

        template <>
        std::complex<double> HashBinarySerializer::readSingleValue(std::istream& is) const;

        template <>
        std::complex<float> HashBinarySerializer::readSingleValue(std::istream& is) const;

        template <>
        karabo::util::Schema HashBinarySerializer::readSingleValue(std::istream& is) const;

        template <>
        karabo::util::Hash HashBinarySerializer::readSingleValue(std::istream& is) const;

        template <>
        karabo::util::ByteArray HashBinarySerializer::readSingleValue(std::istream& is) const;


    } // namespace io
} // namespace karabo

KARABO_REGISTER_CONFIGURATION_BASE_CLASS(karabo::io::BinarySerializer<karabo::util::Hash>)

#endif
