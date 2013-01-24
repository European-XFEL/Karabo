/* 
 * File:   HeaderFormat.hh
 * Author: esenov
 *
 * Created on June 14, 2011, 3:27 PM
 */

#ifndef KARABO_IO_HEADERHASHFORMAT_HH
#define	KARABO_IO_HEADERHASHFORMAT_HH

#include "HashBinaryFormat.hh"

namespace karabo {
    namespace io {

        class HeaderFormat : public HashBinaryFormat {
        public:
            
            KARABO_CLASSINFO(HeaderFormat, "Bin", "1.0")

            HeaderFormat() {
            }
            
            virtual ~HeaderFormat() {
            }

            static void expectedParameters(karabo::util::Schema& expected);
            void configure(const karabo::util::Hash& input);

            virtual void convert(const karabo::util::Hash& in, std::stringstream& out) {
                this->writeStream(out, in, m_sep);
            }

            virtual void convert(std::stringstream& in, karabo::util::Hash& out) {
                this->readStream(in, out, m_sep);
            }
            
            virtual int readKey(std::istream& is, std::string& path);
            virtual void writeKey(std::ostream& os, const std::string& path);
            
            virtual int readType(std::istream& is, karabo::util::Types::ReferenceType& id) {
                return HashBinaryFormat::readFrom(is, &id, sizeof(id));
            }
            
            virtual void writeType(std::ostream& os, karabo::util::Types::ReferenceType id) {
                HashBinaryFormat::writeTo(os, &id, sizeof(id));
            }

        private:
            std::string m_sep;
        };
    }
}

#endif	/* KARABO_IO_HEADERHASHFORMAT_HH */

