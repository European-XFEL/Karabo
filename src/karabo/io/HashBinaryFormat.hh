/* 
 * File:   HashBinaryFormat.hh
 * Author: esenov
 *
 * Created on June 14, 2011, 5:27 PM
 */

#ifndef KARABO_IO_HASHBINARYFORMAT_HH
#define	KARABO_IO_HASHBINARYFORMAT_HH

#include "HashBaseFormat.hh"

namespace karabo {
    namespace io {

        class HashBinaryFormat : public HashBaseFormat {

            public:

            HashBinaryFormat() {
            }

            virtual ~HashBinaryFormat() {
            }

            virtual int readStringValue(std::istream& is, std::string& value) {
                int len;
                int size = this->readFrom(is, &len, sizeof (int));
                {
                    boost::scoped_ptr<char> buffer(new char[len]);
                    is.read(buffer.get(), len);
                    size += len;
                    value.assign(buffer.get(), len);
                }
                return size;
            }

            virtual void writeStringValue(std::ostream& os, const std::string& value) const {
                int vlen = value.length();
                this->writeTo(os, &vlen, sizeof (int));
                os.write(value.c_str(), vlen);
            }

            virtual int readVectorString(std::istream& is, std::vector<std::string>& values) {
                int vsize;
                int size = this->readFrom(is, &vsize, sizeof (int)); // read vector size
                values.clear();
                for (int i = 0; i < vsize; i++) {
                    std::string value;
                    size += readStringValue(is, value); // read string value
                    values.push_back(value);
                }
                return size;
            }

            virtual void writeVectorString(std::ostream& os, const std::vector<std::string>& values) const {
                int vlen = values.size();
                this->writeTo(os, &vlen, sizeof (int));
                for (std::vector<std::string>::const_iterator it = values.begin(); it != values.end(); it++) {
                    writeStringValue(os, *it);
                }
            }

            virtual int readKey(std::istream& is, std::string& path) = 0;
            virtual void writeKey(std::ostream& os, const std::string& path) = 0;
            virtual int readType(std::istream& is, karabo::util::Types::ReferenceType& id) = 0;
            virtual void writeType(std::ostream& os, karabo::util::Types::ReferenceType id) = 0;
        };
    }
}

#endif	/* KARABO_IO_HASHBINARYFORMAT_HH */

