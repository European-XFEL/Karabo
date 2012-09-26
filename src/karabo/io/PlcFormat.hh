/* 
 * File:   PlcFormat.hh
 * Author: <serguei.essenov at xfel.eu>
 *
 * Created on May 23, 2011, 4:06 PM
 */

#ifndef KARABO_IO_PLCFORMAT_HH
#define	KARABO_IO_PLCFORMAT_HH

#include <sstream>
#include "HashBinaryFormat.hh"

namespace karabo {
    namespace io {

        class PlcFormat : public HashBinaryFormat {
        public:

            KARABO_CLASSINFO(PlcFormat, "Plc", "1.0")

            PlcFormat() {
            }

            virtual ~PlcFormat() {
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
            virtual int readType(std::istream& is, karabo::util::Types::Type& id);
            virtual void writeType(std::ostream& os, karabo::util::Types::Type id);

        private:
            karabo::util::Schema m_config;
            std::string m_sep;
        };
    }
}

#endif	/* KARABO_IO_PLCFORMAT_HH */

