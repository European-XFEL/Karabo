/* 
 * File:   HeaderFormat.hh
 * Author: esenov
 *
 * Created on June 14, 2011, 3:27 PM
 */

#ifndef EXFEL_IO_HEADERHASHFORMAT_HH
#define	EXFEL_IO_HEADERHASHFORMAT_HH

#include "HashBinaryFormat.hh"

namespace exfel {
    namespace io {

        class HeaderFormat : public HashBinaryFormat {
        public:
            
            EXFEL_CLASSINFO(HeaderFormat, "Bin", "1.0")

            HeaderFormat() {
            }
            
            virtual ~HeaderFormat() {
            }

            static void expectedParameters(exfel::util::Schema& expected);
            void configure(const exfel::util::Hash& input);

            virtual void convert(const exfel::util::Hash& in, std::stringstream& out) {
                this->writeStream(out, in, m_sep);
            }

            virtual void convert(std::stringstream& in, exfel::util::Hash& out) {
                this->readStream(in, out, m_sep);
            }
            
            virtual int readKey(std::istream& is, std::string& path);
            virtual void writeKey(std::ostream& os, const std::string& path);
            
            virtual int readType(std::istream& is, exfel::util::Types::Type& id) {
                return HashBinaryFormat::readFrom(is, &id, sizeof(id));
            }
            
            virtual void writeType(std::ostream& os, exfel::util::Types::Type id) {
                HashBinaryFormat::writeTo(os, &id, sizeof(id));
            }

        private:
            std::string m_sep;
        };
    }
}

#endif	/* EXFEL_IO_HEADERHASHFORMAT_HH */

