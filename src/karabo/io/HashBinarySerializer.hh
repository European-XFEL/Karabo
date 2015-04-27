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
            
            bool m_nodesAsSharedPtr;
            
        public:

            KARABO_CLASSINFO(HashBinarySerializer, "Bin", "1.0")

            static void expectedParameters(karabo::util::Schema& expected);
            

            HashBinarySerializer(const karabo::util::Hash& input);

            virtual void save(const karabo::util::Hash& object, std::vector<char>& archive) {
                // TODO We will introduce a copy here, which could be avoided with some effort
                std::stringstream os;
                this->writeHash(object, os);
                os.seekg(0);
                archive.resize(os.rdbuf()->in_avail());
                os.read(&archive[0], archive.size());
            }

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
            virtual ~HashBinarySerializer() {
            };


        private: // members


        private: // functions

            void writeHash(const karabo::util::Hash& hash, std::ostream& os);
            void writeNode(const karabo::util::Hash::Node& element, std::ostream& os);
            void writeAttributes(const karabo::util::Hash::Attributes& attributes, std::ostream& os);
            void writeAny(const boost::any& value, const karabo::util::Types::ReferenceType type, std::ostream& os);

            template<typename T>
            inline void writeSingleValue(std::ostream& os, const T& value) {
                os.write((char*) &value, sizeof (T));
            }

            template<typename T>
            inline void writeComplexValue(std::ostream& os, const std::complex<T>& value) {
                writeSingleValue(os, value.imag());
                writeSingleValue(os, value.real());
            }

            template<typename T>
            inline void writeSequenceBulk(std::ostream& os, const std::vector<T>& vect) {
                writeSize(os, vect.size());
                os.write((char*) &(vect[0]), vect.size() * sizeof (T));
            }

            template<typename T>
            inline void writeSequence(std::ostream& os, const std::vector<T>& vect) {
                writeSize(os, vect.size());
                for (unsigned i = 0; i < vect.size(); ++i) {
                    writeSingleValue(os, vect[i]);
                }
            }

            template<typename T>
            inline void writeRawArray(std::ostream& os, const std::pair<const T*, size_t>& raw) {
                writeSize(os, raw.second);
                os.write((char*) raw.first, raw.second * sizeof (T));
            }

            inline void writeSingleValue(std::ostream& os, const boost::any&, const karabo::util::Types::ReferenceType type);

            inline void writeSequence(std::ostream& os, const boost::any&, const karabo::util::Types::ReferenceType type);

            inline void writeRawArray(std::ostream& os, const boost::any&, const karabo::util::Types::ReferenceType type);

            static inline void writeKey(std::ostream& os, const std::string& str);

            inline void writeType(std::ostream& os, const karabo::util::Types::ReferenceType type);

            static inline void writeSize(std::ostream& os, const unsigned size);

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
                return std::complex<T > (readSingleValue<T > (is), readSingleValue<T > (is));
            }

            template<typename T>
            inline void readSequenceBulk(std::istream& is, boost::any& value, unsigned size) {
                value = std::vector<T > (size);
                std::vector<T>& result = boost::any_cast<std::vector<T>& >(value);
                is.read(reinterpret_cast<char*> (&result[0]), size * sizeof (T));
            }

            template<typename T>
            inline void readSequence(std::istream& is, boost::any& value, unsigned size) {
                value = std::vector<T > (size);
                std::vector<T>& result = boost::any_cast<std::vector<T>& >(value);
                for (unsigned i = 0; i < size; ++i) {
                    result[i] = readSingleValue<T > (is);
                }
            }

            inline boost::any readSingleValue(std::istream& is, const karabo::util::Types::ReferenceType type);

            inline void readSequence(std::istream& is, boost::any&, const karabo::util::Types::ReferenceType type);



            static inline unsigned readSize(std::istream& is);

            static inline std::string readKey(std::istream& is);

            inline karabo::util::Types::ReferenceType readType(std::istream& is);
        };

        template<>
        void HashBinarySerializer::writeSingleValue(std::ostream& os, const std::string&);       

        template<>
        void HashBinarySerializer::writeSingleValue(std::ostream& os, const std::complex<float>&);

        template<>
        void HashBinarySerializer::writeSingleValue(std::ostream& os, const std::complex<double>&);

        template<>
        void HashBinarySerializer::writeSingleValue(std::ostream& os, const karabo::util::Schema&);
        
        template<>
        void HashBinarySerializer::writeSingleValue(std::ostream& os, const karabo::util::CppNone& value);

        template<>
        std::string HashBinarySerializer::readSingleValue(std::istream& is);

        template<>
        std::complex<double> HashBinarySerializer::readSingleValue(std::istream& is);

        template<>
        std::complex<float> HashBinarySerializer::readSingleValue(std::istream& is);

        template<>
        karabo::util::Schema HashBinarySerializer::readSingleValue(std::istream& is);



    }
}

KARABO_REGISTER_CONFIGURATION_BASE_CLASS(karabo::io::BinarySerializer<karabo::util::Hash>)

#endif
