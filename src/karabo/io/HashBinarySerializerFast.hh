/*
 * $Id$
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_IO_HASHBINARYSERIALIZERFAST_HH
#define	KARABO_IO_HASHBINARYSERIALIZERFAST_HH

#include <iostream>
#include "BinarySerializer.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    namespace io {

        /**
         * The HashBinarySerializer class.
         */
        class HashBinarySerializerFast : public BinarySerializer<karabo::util::Hash> {

            bool m_nodesAsSharedPtr;

        public:

            KARABO_CLASSINFO(HashBinarySerializerFast, "FastBin", "1.0")

            static void expectedParameters(karabo::util::Schema& expected);


            HashBinarySerializerFast(const karabo::util::Hash& input);

            virtual void save(const karabo::util::Hash& object, std::vector<char>& buffer);

            virtual void load(karabo::util::Hash& object, const char* archive, const size_t nBytes) {
                std::stringstream is;
                is.rdbuf()->pubsetbuf(const_cast<char*> (archive), nBytes);
                this->readHash(object, is);
            }

            void save(const std::vector<karabo::util::Hash>& objects, std::vector<char>& archive);

            void load(std::vector<karabo::util::Hash>& objects, const char* archive, const size_t nBytes);

            /**
             * Destructor.
             */
            virtual ~HashBinarySerializerFast() {
            };


        private: // members


        private: // functions

            void writeHash(const karabo::util::Hash& hash, std::vector<char>& buffer);
            void writeNode(const karabo::util::Hash::Node& element, std::vector<char>& buffer);
            void writeAttributes(const karabo::util::Hash::Attributes& attributes, std::vector<char>& buffer);
            void writeAny(const boost::any& value, const karabo::util::Types::ReferenceType type, std::vector<char>& buffer);

            template<typename T>
            inline void writeSingleValue(std::vector<char>& buffer, const T& value) {
                const char* src = reinterpret_cast<const char*> (&value);
                const size_t n = sizeof (value);
                const size_t pos = buffer.size();
                buffer.resize(pos + n);
                std::memcpy(&buffer[pos], src, n);
            }

            template<typename T>
            inline void writeComplexValue(std::vector<char>& buffer, const std::complex<T>& value) {
                writeSingleValue(buffer, value.real());
                writeSingleValue(buffer, value.imag());
            }

            template<typename T>
            inline void writeSequenceBulk(std::vector<char>& buffer, const std::vector<T>& vect) {
                writeSingleValue(buffer, (unsigned int) vect.size());
                const char* src = reinterpret_cast<const char*> (&vect[0]);
                const size_t n = vect.size() * sizeof (T);
                const size_t pos = buffer.size();
                buffer.resize(pos + n);
                std::memcpy(&buffer[pos], src, n);
            }

            template<typename T>
            inline void writeSequence(std::vector<char>& buffer, const std::vector<T>& vect) {
                writeSingleValue(buffer, (unsigned int) vect.size());
                for (unsigned i = 0; i < vect.size(); ++i) {
                    writeSingleValue(buffer, vect[i]);
                }
            }

            template<typename T>
            inline void writeRawArray(std::vector<char>& buffer, const std::pair<const T*, size_t>& raw) {
                writeSingleValue(buffer, (unsigned int) raw.second);
                const char* src = reinterpret_cast<const char*> (raw.first);
                const size_t n = raw.second * sizeof (T);
                const size_t pos = buffer.size();
                buffer.resize(pos + n);
                std::memcpy(&buffer[pos], src, n);
            }

            inline void writeSingleValue(std::vector<char>& buffer, const boost::any&, const karabo::util::Types::ReferenceType type);

            inline void writeSequence(std::vector<char>& buffer, const boost::any&, const karabo::util::Types::ReferenceType type);

            inline void writeRawArray(std::vector<char>& buffer, const boost::any&, const karabo::util::Types::ReferenceType type);

            inline void writeKey(std::vector<char>& buffer, const std::string& str);

            inline void writeType(std::vector<char>& buffer, const karabo::util::Types::ReferenceType type);

            inline void writeSize(std::vector<char>& buffer, const unsigned size);

            void readHash(karabo::util::Hash& hash, std::istream& is);

            void readNode(karabo::util::Hash::Node& element, std::istream& is);

            void readAttributes(karabo::util::Hash::Attributes& attributes, std::istream& is);

            void readAny(boost::any& value, const karabo::util::Types::ReferenceType type, std::istream& is);

            template<typename T>
            inline T readSingleValue(std::istream& is) {

                union {

                    char buffer[32];
                    T tbuffer[32 / sizeof (T)];
                } all;
                memset(all.buffer, 0, 32);
                is.read(all.buffer, sizeof (T));
                return all.tbuffer[0];
            }

            template<typename T>
            inline std::complex<T> readComplexValue(std::istream& is) {
                T const real = readSingleValue<T>(is);
                T const imag = readSingleValue<T>(is);
                return std::complex<T > (real, imag);
            }

            template<typename T>
            inline void readSequenceBulk(std::istream& is, boost::any& value, unsigned size) {
                value = std::vector<T > ();
                std::vector<T>& result = boost::any_cast<std::vector<T>& >(value);
                result.resize(size);
                is.read(reinterpret_cast<char*> (&result[0]), size * sizeof (T));
            }

            template<typename T>
            inline void readSequence(std::istream& is, boost::any& value, unsigned size) {
                value = std::vector<T > ();
                std::vector<T>& result = boost::any_cast<std::vector<T>& >(value);
                result.resize(size);
                for (unsigned i = 0; i < size; ++i) {
                    result[i] = readSingleValue<T > (is);
                }
            }

            inline void readSingleValue(std::istream& is, boost::any& value, const karabo::util::Types::ReferenceType type);

            inline void readSequence(std::istream& is, boost::any&, const karabo::util::Types::ReferenceType type);



            static inline unsigned readSize(std::istream& is);

            static inline std::string readKey(std::istream& is);

            inline karabo::util::Types::ReferenceType readType(std::istream& is);
        };

        template<>
        void HashBinarySerializerFast::writeSingleValue(std::vector<char>& buffer, const std::string&);

        template<>
        void HashBinarySerializerFast::writeSingleValue(std::vector<char>& buffer, const std::complex<float>&);

        template<>
        void HashBinarySerializerFast::writeSingleValue(std::vector<char>& buffer, const std::complex<double>&);

        template<>
        void HashBinarySerializerFast::writeSingleValue(std::vector<char>& buffer, const karabo::util::Schema&);

        template<>
        void HashBinarySerializerFast::writeSingleValue(std::vector<char>& buffer, const karabo::util::Hash&);

        template<>
        void HashBinarySerializerFast::writeSingleValue(std::vector<char>& buffer, const karabo::util::CppNone& value);

        template<>
        std::string HashBinarySerializerFast::readSingleValue(std::istream& is);

        template<>
        std::complex<double> HashBinarySerializerFast::readSingleValue(std::istream& is);

        template<>
        std::complex<float> HashBinarySerializerFast::readSingleValue(std::istream& is);

        template<>
        karabo::util::Schema HashBinarySerializerFast::readSingleValue(std::istream& is);

        template<>
        karabo::util::Hash HashBinarySerializerFast::readSingleValue(std::istream& is);



    }
}

KARABO_REGISTER_CONFIGURATION_BASE_CLASS(karabo::io::BinarySerializer<karabo::util::Hash>)

#endif
