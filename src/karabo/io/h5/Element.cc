/*00
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#include "Element.hh"

#include <boost/algorithm/string/replace.hpp>
#include <karabo/io/h5/Attribute.hh>
#include <karabo/log/Logger.hh>
#include <karabo/util/Configurator.hh>
#include <karabo/util/Factory.hh>
#include <karabo/util/SimpleElement.hh>

#include "Dataset.hh"
#include "ErrorHandler.hh"
#include "karabo/util/ListElement.hh"


using namespace karabo::util;


namespace karabo {
    namespace io {
        namespace h5 {


            void Element::expectedParameters(Schema& expected) {
                STRING_ELEMENT(expected)
                      .key("h5name")
                      .tags("persistent")
                      .displayedName("H5 Name")
                      .description("Group or dataset name. i.e.: d1, g4.d2")
                      .assignmentMandatory()
                      .reconfigurable()
                      .commit();


                STRING_ELEMENT(expected)
                      .key("h5path")
                      .tags("persistent")
                      .displayedName("H5 Path")
                      .description("Path to that element. i.e. instrument.XXX.LPD")
                      .assignmentOptional()
                      .noDefaultValue()
                      .reconfigurable()
                      .commit();

                STRING_ELEMENT(expected)
                      .key("key")
                      .displayedName("Hash key")
                      .description("Path to the data element in Hash")
                      .assignmentOptional()
                      .noDefaultValue()
                      .reconfigurable()
                      .commit();

                LIST_ELEMENT(expected)
                      .key("attributes")
                      .displayedName("Attributes")
                      .description("Definition of hdf5 attributes.")
                      .appendNodesOfConfigurationBase<Attribute>()
                      .assignmentOptional()
                      .noDefaultValue()
                      .commit();
            }


            Element::Element(const Hash& input) : m_h5obj(-1) {
                try {
                    m_h5name = input.get<std::string>("h5name");
                    m_h5path = "";
                    if (input.has("h5path")) {
                        m_h5path = input.get<std::string>("h5path");
                    }
                    if (m_h5path != "") m_h5PathName = m_h5path + "/" + m_h5name;
                    else m_h5PathName = m_h5name;

                    if (input.has("key")) {
                        m_key = boost::replace_all_copy(input.get<std::string>("key"), ".", "/");
                    } else {
                        m_key = m_h5PathName;
                    }

                    if (m_key.size() == 0 || m_h5name.size() == 0) {
                        throw KARABO_PARAMETER_EXCEPTION("Name cannot be an empty string");
                    }
                    m_config = input;

                    if (input.has("attributes")) {
                        m_attributes = Configurator<Attribute>::createList("attributes", input, false);
                    }

                } catch (...) {
                    KARABO_RETHROW_AS("Error setting Element ");
                }
            }


            void Element::createAttributes() {
                for (size_t i = 0; i < m_attributes.size(); ++i) {
                    m_attributes[i]->create(m_h5obj);
                }
            }


            void Element::openAttributes() {
                KARABO_LOG_FRAMEWORK_TRACE_CF << "opening attributes for element " << m_h5PathName;
                // if the element is Hash within a vector it cannot have attributes
                // and we cannot use getNode
                int len = m_key.length() - 1;
                if (m_key[len] == ']') {
                    return;
                }
                openH5(m_tableGroup);
                for (size_t i = 0; i < m_attributes.size(); ++i) {
                    m_attributes[i]->open(m_h5obj);
                }
            }


            void Element::writeAttributes(const karabo::util::Hash& data) {
                if (data.has(m_key, '/')) {
                    const Hash::Node& node = data.getNode(m_key, '/');
                    for (size_t i = 0; i < m_attributes.size(); ++i) {
                        m_attributes[i]->write(node);
                    }
                }
            }


            void Element::readAttributes(karabo::util::Hash& data) {
                int len = m_key.length() - 1;
                if (m_key[len] == ']') {
                    return;
                }
                KARABO_LOG_FRAMEWORK_TRACE_CF << "reading attributes";
                Hash::Node& node = data.getNode(m_key, '/');
                for (size_t i = 0; i < m_attributes.size(); ++i) {
                    m_attributes[i]->read(node);
                }
            }


            void Element::closeAttributes() {
                KARABO_LOG_FRAMEWORK_TRACE_CF << "closing attributes";
                int len = m_key.length() - 1;
                if (m_key[len] == ']') {
                    return;
                }
                for (size_t i = 0; i < m_attributes.size(); ++i) {
                    m_attributes[i]->close();
                }
            }


            void Element::saveAttributes(hid_t tableGroup, const karabo::util::Hash& data) {
                try {
                    // if the element is Hash within a vector it cannot have attributes
                    // and we cannot use getNode
                    int len = m_key.length() - 1;
                    if (m_key[len] == ']') {
                        return;
                    }

                    size_t numAttributes = m_attributes.size();
                    if (numAttributes == 0) return;
                    if (data.has(m_key, '/')) {
                        const karabo::util::Hash::Node& node = data.getNode(m_key, '/');
                        openH5(tableGroup);
                        for (size_t i = 0; i < numAttributes; ++i) {
                            m_attributes[i]->save(node, m_h5obj);
                        }
                        closeH5();
                    }

                } catch (karabo::util::Exception& ex) {
                    KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot save Hash attributes for element " +
                                                                  this->m_key + " to dataset /" + this->m_h5PathName));
                }
            }

        } // namespace h5
    }     // namespace io
} // namespace karabo
