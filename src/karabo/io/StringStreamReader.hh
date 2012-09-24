/* 
 * File:   StringStreamReader.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on May 24, 2011, 2:40 PM
 */

#ifndef STRINGSTREAMREADER_HH
#define	STRINGSTREAMREADER_HH

#include <iostream>
#include <string>
#include <sstream>
#include <karabo/util/Factory.hh>
#include "Format.hh"
#include "Reader.hh"

namespace exfel {
    namespace io {

        template <class T>
        class StringStreamReader : public Reader<T> {
        public:

            EXFEL_CLASSINFO(StringStreamReader<T>, "StringStream", "1.0")

            typedef boost::shared_ptr<Format<T> > FormatPointer;
            typedef exfel::util::Factory<Format<T> > FormatFactory;

            StringStreamReader()
            {}

            StringStreamReader(const FormatPointer& format) : m_format(format)
            {}

            virtual ~StringStreamReader()
            {}

            static void expectedParameters(exfel::util::Schema& expected) {

                using namespace exfel::util;

                CHOICE_ELEMENT<Format<T> >(expected).key("format")
                        .displayedName("Format")
                        .description("Select the format which should be used to interprete the data")
                        .assignmentOptional().noDefaultValue()
                        .commit();

                INTERNAL_ANY_ELEMENT(expected).key("string")
                        .description("Expects an initialized string object")
                        .commit();
            }

            void configure(const exfel::util::Hash& input) {
                if (input.has("format")) {
                    m_format = Format<T>::createChoice("format", input);
                }
                if (input.has("string")) {
                    m_string = input.get<std::string >("string");
                }
            }

            void read(T& data) {
                m_stream.seekg(0);
                m_stream << m_string;
                m_format->convert(m_stream, data);
            }

        private:

            std::stringstream m_stream;
            FormatPointer m_format;
            std::string m_string;

        };
        
    }
}

#endif	/* STRINGSTREAMREADER_HH */

