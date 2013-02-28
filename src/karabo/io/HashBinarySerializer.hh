/*
 * $Id$
 *
 * Author: <djelloul.boukhelef@xfel.eu>
 *
 * Created on February 27, 2013, 10:03 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_IO_HASHBINARYSERIALIZER_HH
#define	KARABO_IO_HASHBINARYSERIALIZER_HH

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
        class HashBinarySerializer : public BinarySerializer<karabo::util::Hash> {
        public:

            KARABO_CLASSINFO(HashBinarySerializer, "Binary", "1.0")

            HashBinarySerializer(const karabo::util::Hash& input) {}

            static void expectedParameters(karabo::util::Schema& expected) {
            };

            virtual void save(const karabo::util::Hash& object, std::vector<char>& archive) {
                // TODO We will introduce a copy here, which could be avoided with some effort
                std::stringstream os;
                this->serialize(object, os);
                os.seekg(0);
                archive.resize(os.rdbuf()->in_avail());
                os.read(&archive[0], archive.size());
            }

            virtual void load(karabo::util::Hash& object, const char* archive, const size_t nBytes) {
                std::stringstream is;
                is.rdbuf()->pubsetbuf(const_cast<char*>(archive), nBytes);
                this->serialize(object, is);
            }

            /**
             * Destructor.
             */
            virtual ~HashBinarySerializer() {
            };


        private: // members


        private: // functions

            void serialize(const karabo::util::Hash& hash, std::ostream& os);
           
            void serialize(karabo::util::Hash& hash, std::istream& is);

            void serialize(const karabo::util::Hash::Node& element, std::ostream& os);

            void serialize(karabo::util::Hash::Node& element, std::istream& is);

            void serialize(const karabo::util::Hash::Attributes& attributes, std::ostream& os);

            void serialize(karabo::util::Hash::Attributes& attributes, std::istream& is);

            void serialize(const boost::any& value, const karabo::util::Types::ReferenceType type, std::ostream& os);

            void serialize(boost::any& value, const karabo::util::Types::ReferenceType type, std::istream& is);

            template<typename T>
            T readSingleValue(std::istream& is) {
                char buffer[32];
                memset(buffer, 0, 32);
                is.read(buffer, sizeof (T));
                return *reinterpret_cast<T*> (buffer);
            }

            template<typename T>
            std::complex<T> readComplexValue(std::istream& is) {
                return std::complex<T > (readSingleValue<T > (is), readSingleValue<T > (is));
            }

            template<typename T>
            void readSequence(std::istream& is, boost::any& value, size_t size) {
                value = std::vector<T > (size);
                std::vector<T>& result = boost::any_cast<std::vector<T>& >(value);
                for (size_t i = 0; i < size; ++i) {
                    result[i] = readSingleValue<T > (is);
                }
            }

            boost::any readSingleValue(std::istream& is, const karabo::util::Types::ReferenceType type);

            void readSequence(std::istream& is, boost::any&, const karabo::util::Types::ReferenceType type);

            template<typename T>
            void writeSingleValue(std::ostream& os, const T& value) {
                os.write((char*) &value, sizeof (T));
            }

            template<typename T>
            void writeComplexValue(std::ostream& os, const std::complex<T>& value) {
                writeSingleValue(os, value.imag());
                writeSingleValue(os, value.real());
            }

            template<typename T>
            void writeSequence(std::ostream& os, const std::vector<T>& vect) {
                writeSize(os, vect.size());
                for (size_t i = 0; i < vect.size(); ++i) {
                    writeSingleValue(os, vect[i]);
                }
            }

            void writeSingleValue(std::ostream& os, const boost::any&, const karabo::util::Types::ReferenceType type);

            void writeSequence(std::ostream& os, const boost::any&, const karabo::util::Types::ReferenceType type);

            void writeSize(std::ostream& os, const size_t size);

            size_t readSize(std::istream& is);

            void writeKey(std::ostream& os, const std::string& str);

            std::string readKey(std::istream& is);

            void writeType(std::ostream& os, const karabo::util::Types::ReferenceType type);

            karabo::util::Types::ReferenceType readType(std::istream& is);

        };

        template<>
        std::string HashBinarySerializer::readSingleValue(std::istream& is);

        template<>
        std::complex<double> HashBinarySerializer::readSingleValue(std::istream& is);

        template<>
        std::complex<float> HashBinarySerializer::readSingleValue(std::istream& is);

        template<>
        karabo::util::Hash HashBinarySerializer::readSingleValue(std::istream& is);

        template<>
        void HashBinarySerializer::writeSingleValue(std::ostream& os, const std::string&);

        template<>
        void HashBinarySerializer::writeSingleValue(std::ostream& os, const karabo::util::Hash&);

        template<>
        void HashBinarySerializer::writeSingleValue(std::ostream& os, const std::complex<float>&);

        template<>
        void HashBinarySerializer::writeSingleValue(std::ostream& os, const std::complex<double>&);

    }
}

#endif
