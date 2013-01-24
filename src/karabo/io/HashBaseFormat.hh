/* 
 * File:   HashBaseFormat.hh
 * Author: esenov
 *
 * Created on June 14, 2011, 11:40 AM
 */

#ifndef KARABO_IO_HASHBASEFORMAT_HH
#define	KARABO_IO_HASHBASEFORMAT_HH

#include <istream>
#include <ostream>
#include <string>
#include <vector>
#include <karabo/util/Hash.hh>
#include "Format.hh"

namespace karabo {
    namespace io {

        class HashBaseFormat : public Format<karabo::util::Hash> {
            Format<karabo::util::Schema>::Pointer m_schemaFormat;

        public:

            HashBaseFormat() {
                m_schemaFormat = Format<karabo::util::Schema>::create("Xml");
            }

            virtual ~HashBaseFormat() {
            }


        public:

            int readFrom(std::istream& is, void* buffer, std::size_t size) {
                is.read(reinterpret_cast<char*> (buffer), size);
                return size;
            }

            void writeTo(std::ostream& os, const void* buffer, const std::size_t size) const {
                os.write(reinterpret_cast<const char*> (buffer), size);
            }

            int readHashValue(std::istream& is, karabo::util::Hash& value, const std::string& sep) {
                int hashLength;
                int size = readFrom(is, &hashLength, sizeof (int));
                std::string sbuf;
                sbuf.resize(hashLength);
                {
                    char* buffer = new char[hashLength];
                    size += readFrom(is, buffer, hashLength);
                    sbuf.assign(buffer, hashLength);
                    delete [] buffer;
                }
                std::stringstream in(sbuf, std::stringstream::in);
                readStream(in, value, sep);
                return size;
            }

            void writeHashValue(std::ostream& os, const karabo::util::Hash& hash, const std::string& sep) {
                std::stringstream out(std::stringstream::out);
                writeStream(out, hash, sep);
                const std::string& value = out.str();
                int vlen = value.length();
                this->writeTo(os, &vlen, sizeof (int));
                this->writeTo(os, value.c_str(), vlen);
            }

            int readVectorHash(std::istream& is, std::vector<karabo::util::Hash>& values, const std::string& sep) {
                int vsize;
                int size = this->readFrom(is, &vsize, sizeof (int)); // read vector size
                for (int i = 0; i < vsize; i++) {
                    karabo::util::Hash value;
                    size += this->readHashValue(is, value, sep); // read Hash object
                    values.push_back(value); // put it into vector
                }
                return size;
            }

            void writeVectorHash(std::ostream& os, const std::vector<karabo::util::Hash>& values, const std::string& sep) {
                int vlen = values.size();
                this->writeTo(os, &vlen, sizeof (int)); // write number of entries in vector<Hash>
                for (std::vector<karabo::util::Hash>::const_iterator it = values.begin(); it != values.end(); it++) {
                    writeHashValue(os, *it, sep); // from hash to os
                }
            }

            virtual int readStringValue(std::istream& is, std::string& value) = 0;
            virtual void writeStringValue(std::ostream& os, const std::string& value) const = 0;

            virtual int readVectorString(std::istream& is, std::vector<std::string>& values) = 0;
            virtual void writeVectorString(std::ostream& os, const std::vector<std::string>& values) const = 0;

            virtual int readKey(std::istream& is, std::string& path) = 0;
            virtual void writeKey(std::ostream& os, const std::string& path) = 0;
            virtual int readType(std::istream& is, karabo::util::Types::ReferenceType& id) = 0;
            virtual void writeType(std::ostream& os, karabo::util::Types::ReferenceType id) = 0;

        protected:

            template <class T>
            int readValue(std::istream& is, T& value) {
                int size = this->readFrom(is, &value, sizeof (T));
                return size;
            }

            template <class T>
            void writeValue(std::ostream& os, const T& value) const {
                this->writeTo(os, &value, sizeof (T));
            }

            template <class T>
            int readVectorValue(std::istream& is, std::vector<T>& values) {
                unsigned int vlen;
                int length = this->readFrom(is, &vlen, sizeof (unsigned int));
                values.resize(vlen);
                typename std::vector<T>::iterator it = values.begin();
                length += this->readFrom(is, &(*it), vlen * sizeof (T));
                return length;
            }

            int readVectorOfBoolValue(std::istream& is, std::deque<bool>& values) {
                int vlen;
                int length = this->readFrom(is, &vlen, sizeof (int));
                values.resize(vlen);
                std::deque<bool>::iterator it = values.begin();
                length += this->readFrom(is, &(*it), vlen * sizeof (bool));
                return length;
            }

            template <class T>
            void writeVectorValue(std::ostream& os, const std::vector<T>& values) const {
                int vlen = values.size();
                writeTo(os, &vlen, sizeof (int));
                typename std::vector<T>::const_iterator it = values.begin();
                writeTo(os, &(*it), vlen * sizeof (T));
            }

            void writeVectorOfBoolValue(std::ostream& os, const std::deque<bool>& values) const {
                int vlen = values.size();
                writeTo(os, &vlen, sizeof (int));
                std::deque<bool>::const_iterator it = values.begin();
                writeTo(os, &(*it), vlen * sizeof (bool));
            }

            void readStream(std::istream& is, karabo::util::Hash& hash, const std::string& sep) {
                // calculate the size of a stream
                is.seekg(0, std::istream::end);
                int streamSize = is.tellg();
                is.seekg(0);
                while (streamSize > 0) {
                    std::string path;
                    int rc = this->readKey(is, path);

                    if (rc <= 0) break;
                    streamSize -= rc;
                    if (streamSize <= 0) break;

                    karabo::util::Types::ReferenceType id;
                    rc = this->readType(is, id);

                    if (rc <= 0) break;
                    streamSize -= rc;
                    if (streamSize <= 0) break;
                    //cout << "fromStream: streamSize " << streamSize << ", tellg " << is.tellg() << endl;
                    if (is.tellg() < 0) break;

                    switch (id) {
                        case karabo::util::Types::BOOL:
                        {
                            bool value;
                            rc = this->readValue<bool>(is, value);
                            hash.setFromPath(path, value, sep);
                            break;
                        }
                        case karabo::util::Types::CHAR:
                        {
                            char value;
                            rc = this->readValue<char>(is, value);
                            hash.setFromPath(path, value, sep);
                            break;
                        }
                        case karabo::util::Types::INT8:
                        {
                            signed char value;
                            rc = this->readValue<signed char>(is, value);
                            hash.setFromPath(path, value, sep);
                            break;
                        }
                        case karabo::util::Types::UINT8:
                        {
                            unsigned char value;
                            rc = this->readValue<unsigned char>(is, value);
                            hash.setFromPath(path, value, sep);
                            break;
                        }
                        case karabo::util::Types::INT16:
                        {
                            short value;
                            rc = this->readValue<short>(is, value);
                            hash.setFromPath(path, value, sep);
                            break;
                        }
                        case karabo::util::Types::UINT16:
                        {
                            unsigned short value;
                            rc = this->readValue<unsigned short>(is, value);
                            hash.setFromPath(path, value, sep);
                            break;
                        }
                        case karabo::util::Types::INT32:
                        {
                            int value;
                            rc = this->readValue<int>(is, value);
                            hash.setFromPath(path, value, sep);
                            break;
                        }
                        case karabo::util::Types::UINT32:
                        {
                            unsigned int value;
                            rc = this->readValue<unsigned int>(is, value);
                            hash.setFromPath(path, value, sep);
                            break;
                        }
                        case karabo::util::Types::INT64:
                        {
                            long long value;
                            rc = this->readValue<long long>(is, value);
                            hash.setFromPath(path, value, sep);
                            break;
                        }
                        case karabo::util::Types::UINT64:
                        {
                            unsigned long long value;
                            rc = this->readValue<unsigned long long>(is, value);
                            hash.setFromPath(path, value, sep);
                            break;
                        }
                        case karabo::util::Types::FLOAT:
                        {
                            float value;
                            rc = this->readValue<float>(is, value);
                            hash.setFromPath(path, value, sep);
                            break;
                        }
                        case karabo::util::Types::DOUBLE:
                        {
                            double value;
                            rc = this->readValue<double>(is, value);
                            hash.setFromPath(path, value, sep);
                            break;
                        }
                        case karabo::util::Types::STRING:
                        {
                            std::string value;
                            rc = this->readStringValue(is, value);
                            hash.setFromPath(path, value, sep);
                            break;
                        }
                        case karabo::util::Types::HASH:
                        {
                            karabo::util::Hash value;
                            rc = this->readHashValue(is, value, sep);
                            hash.setFromPath(path, value, sep);
                            break;
                        }
                        case karabo::util::Types::VECTOR_STRING:
                        {
                            std::vector<std::string> value;
                            rc = this->readVectorString(is, value);
                            hash.setFromPath(path, value, sep);
                            break;
                        }
                        case karabo::util::Types::VECTOR_CHAR:
                        {
                            std::vector<char> value;
                            rc = this->readVectorValue<char>(is, value);
                            hash.setFromPath(path, value, sep);
                            break;
                        }
                        case karabo::util::Types::VECTOR_INT8:
                        {
                            std::vector<signed char> value;
                            rc = this->readVectorValue<signed char>(is, value);
                            hash.setFromPath(path, value, sep);
                            break;
                        }
                        case karabo::util::Types::VECTOR_UINT8:
                        {
                            std::vector<unsigned char> value;
                            rc = this->readVectorValue<unsigned char>(is, value);
                            hash.setFromPath(path, value, sep);
                            break;
                        }
                        case karabo::util::Types::VECTOR_INT16:
                        {
                            std::vector<short> value;
                            rc = this->readVectorValue<short>(is, value);
                            hash.setFromPath(path, value, sep);
                            break;
                        }
                        case karabo::util::Types::VECTOR_UINT16:
                        {
                            std::vector<unsigned short> value;
                            rc = this->readVectorValue<unsigned short>(is, value);
                            hash.setFromPath(path, value, sep);
                            break;
                        }
                        case karabo::util::Types::VECTOR_INT32:
                        {
                            std::vector<int> value;
                            rc = this->readVectorValue<int>(is, value);
                            hash.setFromPath(path, value, sep);
                            break;
                        }
                        case karabo::util::Types::VECTOR_UINT32:
                        {
                            std::vector<unsigned int> value;
                            rc = this->readVectorValue<unsigned int>(is, value);
                            hash.setFromPath(path, value, sep);
                            break;
                        }
                        case karabo::util::Types::VECTOR_INT64:
                        {
                            std::vector<long long> value;
                            rc = this->readVectorValue<long long>(is, value);
                            hash.setFromPath(path, value, sep);
                            break;
                        }
                        case karabo::util::Types::VECTOR_UINT64:
                        {
                            std::vector<unsigned long long> value;
                            rc = this->readVectorValue<unsigned long long>(is, value);
                            hash.setFromPath(path, value, sep);
                            break;
                        }
                        case karabo::util::Types::VECTOR_BOOL:
                        {
                            std::deque<bool> value;
                            rc = this->readVectorOfBoolValue(is, value);
                            hash.setFromPath(path, value, sep);
                            break;
                        }
                        case karabo::util::Types::VECTOR_DOUBLE:
                        {
                            std::vector<double> value;
                            rc = this->readVectorValue<double>(is, value);
                            hash.setFromPath(path, value, sep);
                            break;
                        }
                        case karabo::util::Types::VECTOR_FLOAT:
                        {
                            std::vector<float> value;
                            rc = this->readVectorValue<float>(is, value);
                            hash.setFromPath(path, value, sep);
                            break;
                        }
                        case karabo::util::Types::VECTOR_HASH:
                        {
                            std::vector<karabo::util::Hash> value;
                            rc = this->readVectorHash(is, value, sep);
                            hash.setFromPath(path, value, sep);
                            break;
                        }
                        case karabo::util::Types::SCHEMA:
                        {
                            std::string value;
                            rc = this->readStringValue(is, value);
                            hash.setFromPath(path, m_schemaFormat->unserialize(value), sep);
                            break;
                        }
                        default:
                            std::ostringstream os;
                            os << id;
                            throw KARABO_NOT_SUPPORTED_EXCEPTION("No conversion exists for Protocol type \"" + os.str() + "\"");
                    }
                    if (rc <= 0) break;
                    streamSize -= rc;
                }
            }

            void l_writeStream(std::ostream& os, const karabo::util::Hash& hash, karabo::util::Hash::const_iterator& it, const std::string& key, const std::string& sep) {
                writeKey(os, key);
                karabo::util::Types::ReferenceType id = hash.getTypeAsId(it);
                this->writeType(os, id);
                switch (id) {
                    case karabo::util::Types::STRING:
                    case karabo::util::Types::CONST_CHAR_PTR:
                    {
                        std::string value = hash.getAsString(it);
                        this->writeStringValue(os, value);
                        break;
                    }
                    case karabo::util::Types::HASH:
                    {
                        const karabo::util::Hash& value = hash.get<karabo::util::Hash > (it);
                        this->writeHashValue(os, value, sep);
                        break;
                    }
                    case karabo::util::Types::VECTOR_HASH:
                    {
                        const std::vector<karabo::util::Hash>& values = hash.get<std::vector<karabo::util::Hash> > (it);
                        this->writeVectorHash(os, values, sep);
                        break;
                    }
                    case karabo::util::Types::VECTOR_CHAR:
                    {
                        const std::vector<char>& values = hash.get<std::vector<char> > (it);
                        this->writeVectorValue<char>(os, values);
                        break;
                    }
                    case karabo::util::Types::VECTOR_INT8:
                    {
                        const std::vector<signed char>& values = hash.get < std::vector<signed char> > (it);
                        this->writeVectorValue<signed char>(os, values);
                        break;
                    }
                    case karabo::util::Types::VECTOR_UINT8:
                    {
                        const std::vector<unsigned char>& values = hash.get<std::vector<unsigned char> > (it);
                        this->writeVectorValue<unsigned char>(os, values);
                        break;
                    }
                    case karabo::util::Types::VECTOR_INT16:
                    {
                        const std::vector<short>& values = hash.get<std::vector<short> > (it);
                        this->writeVectorValue<short>(os, values);
                        break;
                    }
                    case karabo::util::Types::VECTOR_UINT16:
                    {
                        const std::vector<unsigned short>& values = hash.get<std::vector<unsigned short> > (it);
                        this->writeVectorValue<unsigned short>(os, values);
                        break;
                    }
                    case karabo::util::Types::VECTOR_INT32:
                    {
                        const std::vector<int>& values = hash.get<std::vector<int> > (it);
                        this->writeVectorValue<int>(os, values);
                        break;
                    }
                    case karabo::util::Types::VECTOR_UINT32:
                    {
                        const std::vector<unsigned int>& values = hash.get<std::vector<unsigned int> > (it);
                        this->writeVectorValue<unsigned int>(os, values);
                        break;
                    }
                    case karabo::util::Types::VECTOR_INT64:
                    {
                        const std::vector<long long>& values = hash.get<std::vector<long long> > (it);
                        this->writeVectorValue<long long>(os, values);
                        break;
                    }
                    case karabo::util::Types::VECTOR_UINT64:
                    {
                        const std::vector<unsigned long long>& values = hash.get<std::vector<unsigned long long> > (it);
                        this->writeVectorValue<unsigned long long>(os, values);
                        break;
                    }
                    case karabo::util::Types::VECTOR_FLOAT:
                    {
                        const std::vector<float>& values = hash.get<std::vector<float> > (it);
                        this->writeVectorValue<float>(os, values);
                        break;
                    }
                    case karabo::util::Types::VECTOR_DOUBLE:
                    {
                        const std::vector<double>& values = hash.get<std::vector<double> > (it);
                        this->writeVectorValue<double>(os, values);
                        break;
                    }
                    case karabo::util::Types::VECTOR_STRING:
                    {
                        const std::vector<std::string>& values = hash.get<std::vector<std::string> > (it);
                        this->writeVectorString(os, values);
                        break;
                    }
                    case karabo::util::Types::VECTOR_BOOL:
                    {
                        const std::deque<bool>& values = hash.get < std::deque<bool> >(it);
                        this->writeVectorOfBoolValue(os, values);
                        break;
                    }
                    case karabo::util::Types::BOOL:
                    {
                        bool value = hash.get<bool > (it);
                        this->writeValue<bool >(os, value);
                        break;
                    }
                    case karabo::util::Types::CHAR:
                    {
                        char value = hash.get<char>(it);
                        this->writeValue<char >(os, value);
                        break;
                    }
                    case karabo::util::Types::INT8:
                    {
                        signed char value = hash.get<signed char>(it);
                        this->writeValue<signed char >(os, value);
                        break;
                    }
                    case karabo::util::Types::UINT8:
                    {
                        unsigned char value = hash.get<unsigned char>(it);
                        this->writeValue<unsigned char >(os, value);
                        break;
                    }
                    case karabo::util::Types::INT16:
                    {
                        short value = hash.get<short>(it);
                        this->writeValue<short >(os, value);
                        break;
                    }
                    case karabo::util::Types::UINT16:
                    {
                        unsigned short value = hash.get<unsigned short>(it);
                        this->writeValue<unsigned short >(os, value);
                        break;
                    }
                    case karabo::util::Types::INT32:
                    {
                        int value = hash.get<int>(it);
                        this->writeValue<int >(os, value);
                        break;
                    }
                    case karabo::util::Types::UINT32:
                    {
                        unsigned int value = hash.get<unsigned int>(it);
                        this->writeValue<unsigned int >(os, value);
                        break;
                    }
                    case karabo::util::Types::INT64:
                    {
                        long long value = hash.get<long long>(it);
                        this->writeValue<long long >(os, value);
                        break;
                    }
                    case karabo::util::Types::UINT64:
                    {
                        unsigned long long value = hash.get<unsigned long long>(it);
                        this->writeValue<unsigned long long >(os, value);
                        break;
                    }
                    case karabo::util::Types::FLOAT:
                    {
                        float value = hash.get<float>(it);
                        this->writeValue<float >(os, value);
                        break;
                    }
                    case karabo::util::Types::DOUBLE:
                    {
                        double value = hash.get<double>(it);
                        this->writeValue<double >(os, value);
                        break;
                    }
                    case karabo::util::Types::SCHEMA:
                    {
                        std::string value = m_schemaFormat->serialize(hash.get<karabo::util::Schema > (it));
                        this->writeStringValue(os, value);
                        break;
                    }
                    default:
                        throw KARABO_NOT_SUPPORTED_EXCEPTION("No conversion exists for datatype \""
                                + hash.getTypeAsString(it) + "\"");
                }
            }

            void r_writeStream(std::ostream& os, const karabo::util::Hash& hash, const std::string& prefix, const std::string& sep) {
                for (karabo::util::Hash::const_iterator it = hash.begin(); it != hash.end(); ++it) {
                    std::string key = it->first;
                    karabo::util::Types::ReferenceType id = hash.getTypeAsId(it);
                    if (id == karabo::util::Types::HASH) {
                        r_writeStream(os, hash.get<karabo::util::Hash > (it), prefix + key + sep, sep);
                    } else {
                        l_writeStream(os, hash, it, prefix + key, sep);
                    }
                }
            }

            void writeStream(std::ostream& os, const karabo::util::Hash& hash, const std::string& sep) {
                this->r_writeStream(os, hash, std::string(""), sep);
            }
        };
    }
}

#endif	/* KARABO_IO_HASHBASEFORMAT_HH */

