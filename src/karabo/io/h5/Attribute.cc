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
                        .key("h5name")
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
                        .reconfigurable() //???
                        .commit();

            }

            Attribute::Attribute(const Hash& input) {
                m_h5name = input.get<string > ("h5name");
                m_h5path = input.get<string > ("h5path");
                if (m_h5path != "") m_h5PathName = m_h5path + "/" + m_h5name;
                else m_h5PathName = m_h5name;

                if (input.has("key")) {
                    m_key = input.get<string > ("key");
                } else {
                    m_key = m_h5PathName;
                }

                if (m_key.size() == 0 || m_h5name.size() == 0) {
                    throw KARABO_PARAMETER_EXCEPTION("Name cannot be an empty string");
                }
                m_config = input;        
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

