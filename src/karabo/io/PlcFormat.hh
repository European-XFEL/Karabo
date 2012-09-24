/* 
 * File:   PlcFormat.hh
 * Author: <serguei.essenov at xfel.eu>
 *
 * Created on May 23, 2011, 4:06 PM
 */

#ifndef EXFEL_IO_PLCFORMAT_HH
#define	EXFEL_IO_PLCFORMAT_HH

#include <sstream>
#include "HashBinaryFormat.hh"

namespace exfel {
    namespace io {

        class PlcFormat : public HashBinaryFormat {
        public:

            EXFEL_CLASSINFO(PlcFormat, "Plc", "1.0")

            PlcFormat() {
            }

            virtual ~PlcFormat() {
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
            virtual int readType(std::istream& is, exfel::util::Types::Type& id);
            virtual void writeType(std::ostream& os, exfel::util::Types::Type id);

        private:
            exfel::util::Schema m_config;
            std::string m_sep;
        };
    }
}

#endif	/* EXFEL_IO_PLCFORMAT_HH */

