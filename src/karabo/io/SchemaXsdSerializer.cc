/*
 * $Id: SchemaXsdSerializer.cc 6764 2012-07-18 09:29:46Z heisenb $
 *
 * File:   SchemaXsdSerializer.cc
 * Author: <irina.kozlova@xfel.eu>
 *
 * Created on September 10, 2010, 10:31 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <karabo/util/util.hh>
#include "SchemaXsdSerializer.hh"

using namespace std;
using namespace karabo::util;

namespace karabo {
    namespace io {


        KARABO_REGISTER_FOR_CONFIGURATION(TextSerializer<Schema>, SchemaXsdSerializer)

        void SchemaXsdSerializer::expectedParameters(karabo::util::Schema& expected) {

            INT32_ELEMENT(expected)
                    .key("indentation")
                    .description("Set the indent characters for printing. Value -1: the most dense formatting without linebreaks. "
                                 "Value 0: no indentation, value 1/2/3: one/two/three space indentation. If not set, default is 2 spaces.")
                    .displayedName("Indentation")
                    .options("-1 0 1 2 3 4")
                    .assignmentOptional().defaultValue(2)
                    .advanced()
                    .commit();

            STRING_ELEMENT(expected)
                    .key("xmlns")
                    .description("Sets the default XML namespace")
                    .displayedName("XML Namespace")
                    .assignmentOptional().defaultValue("http://xfel.eu/config")
                    .advanced()
                    .commit();
        }


        SchemaXsdSerializer::SchemaXsdSerializer(const karabo::util::Hash& input) {
            if (input.has("indentation")) {
                input.get("indentation", m_indentation);
            } else {
                m_indentation = 2;
            }
            m_defaultNamespace = input.get<string > ("xmlns");
        }


        void SchemaXsdSerializer::load(karabo::util::Schema& object, const std::string& archive) {
            throw KARABO_NOT_SUPPORTED_EXCEPTION("Loading (de-serialization) of an XSD file into a Schema object is not supported");
        }


        void SchemaXsdSerializer::save(const Schema& object, std::string& archive) {


        }
    } // namespace io
} // namespace karabo
