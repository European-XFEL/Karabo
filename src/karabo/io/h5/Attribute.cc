/*
 * $Id: Format.cc 6095 2012-05-08 10:05:56Z boukhele $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "Attribute.hh"
#include "karabo/util/ListElement.hh"
#include <karabo/util/Factory.hh>
#include <karabo/util/Configurator.hh>
#include <karabo/util/SimpleElement.hh>


using namespace karabo::util;


namespace karabo {
    namespace io {
        namespace h5 {

            void Attribute::expectedParameters(Schema& expected) {

                STRING_ELEMENT(expected)
                        .key("name")
                        .displayedName("Name")
                        .description("Group or dataset name. i.e.: d1, g4.d2")
                        .assignmentOptional().defaultValue("aa")
                        //.assignmentMandatory()
                        .reconfigurable() // ???
                        .commit();


//                STRING_ELEMENT(expected)
//                        .key("path")
//                        .displayedName("Path")
//                        .description("Path to that element. i.e. instrument.XXX.LPD")
//                        .assignmentOptional().defaultValue("instrument")
//                        //.assignmentMandatory()
//                        .reconfigurable() //???
//                        .commit();

                STRING_ELEMENT(expected)
                        .key("type")
                        .displayedName("Type")
                        .description("Type")
                        .assignmentOptional().defaultValue("INT32")
                        //.assignmentMandatory()
                        .reconfigurable() //???
                        .commit();

            }

            Attribute::Attribute(const Hash& input) {
                m_key = input.get<string > ("name");
                m_path = input.get<string > ("path");
                if (m_path != "") m_path_key = m_path + "/" + m_key;
                else m_path_key = m_key;
                
                if (m_key.size() == 0) {
                    throw KARABO_PARAMETER_EXCEPTION("Name cannot be an empty string");
                }
            }

            const string& Attribute::getName() {
                return m_key;
            }

            void Attribute::getAttribute(Hash& element) {
                element.set(m_key, shared_from_this());
            }




        }
    }
}

