/* 
 * File:   StringStreamWriter.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on May 24, 2011, 2:41 PM
 */

#ifndef STRINGSTREAMWRITER_HH
#define	STRINGSTREAMWRITER_HH

#include <iostream>
#include <string>
#include <sstream>
#include "Format.hh"
#include "Writer.hh"

namespace karabo {
    namespace io {

        /**
         * TO DISUSS: (during code review/refactoring)
         * Think about this pattern as a concept. It needs a complete 
         * instantiation per string that is read. This is VERY expensive!!!
         */
        template <class T>
        class StringStreamWriter : public Writer<T> {
        public:

            KARABO_CLASSINFO(StringStreamWriter<T>, "StringStream", "1.0")

            typedef boost::shared_ptr<Format<T> > FormatPointer;

            StringStreamWriter()
            {}

            StringStreamWriter(const FormatPointer& format) : m_format(format)
            {}

            virtual ~StringStreamWriter()
            {}
            
            static void expectedParameters(karabo::util::Schema& expected) {

                using namespace karabo::util;

                CHOICE_ELEMENT<Format<T> > (expected).key("format")
                        .displayedName("Format")
                        .description("Select the format which should be used to interpret the data")
                        .assignmentOptional().noDefaultValue()
                        .commit();

                INTERNAL_ANY_ELEMENT(expected).key("stringPointer")
                        .description("Expect a pointer to an initialized string object")
                        .init()
                        .commit();
            }

            void configure(const karabo::util::Hash& input) {
                if (input.has("format")) {
                    m_format = Format<T>::createChoice("format", input);
                }
                if (input.has("stringPointer")) {
                    m_strptr = input.get<std::string* >("stringPointer");
                }
            }

            void write(const T& data) {
                m_format->convert(data, m_stream);
                (*m_strptr).append(m_stream.str());
            }

        private:

            std::stringstream m_stream;
            FormatPointer m_format;
            std::string* m_strptr;
        };

    }
}

#endif	/* STRINGSTREAMWRITER_HH */

