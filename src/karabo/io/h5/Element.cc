/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "Element.hh"
#include "karabo/util/ListElement.hh"
#include "ErrorHandler.hh"
#include <karabo/util/Factory.hh>
#include <karabo/util/Configurator.hh>
#include <karabo/util/SimpleElement.hh>
#include <karabo/io/h5/Attribute.hh>
#include <boost/algorithm/string/replace.hpp>


using namespace karabo::util;


namespace karabo {
    namespace io {
        namespace h5 {

            void Element::expectedParameters(Schema& expected) {

                STRING_ELEMENT(expected)
                        .key("h5name")
                        .displayedName("H5 Name")
                        .description("Group or dataset name. i.e.: d1, g4.d2")
                        .assignmentMandatory()
                        .reconfigurable()
                        .commit();


                STRING_ELEMENT(expected)
                        .key("h5path")
                        .displayedName("H5 Path")
                        .description("Path to that element. i.e. instrument.XXX.LPD")
                        .assignmentMandatory()
                        .reconfigurable()
                        .commit();

                STRING_ELEMENT(expected)
                        .key("key")
                        .displayedName("Hash key")
                        .description("Path to the data element in Hash")
                        .assignmentOptional().noDefaultValue()
                        .reconfigurable()
                        .commit();

//                STRING_ELEMENT(expected)
//                        .key("type")
//                        .displayedName("Type")
//                        .description("Data Type in Hash")
//                        .assignmentMandatory()
//                        .reconfigurable()
//                        .commit();


                LIST_ELEMENT(expected)
                        .key("attributes")
                        .displayedName("Attributes")
                        .description("Definition of hdf5 attributes.")
                        .appendNodesOfConfigurationBase<Attribute > ()
                        .assignmentOptional().noDefaultValue()
                        .commit();


                /*BOOL_ELEMENT(expected)
                        .key("enable")
                        .displayedName("Enable")
                        .description("Flag indicating if this element will be written to HDF5 file")
                        .assignmentOptional().defaultValue(true)                        
                        .reconfigurable()
                        .commit();
                 */
            }

            Element::Element(const Hash& input) {
                m_h5name = input.get<string > ("h5name");
                m_h5path = input.get<string > ("h5path");
                if (m_h5path != "") m_h5PathName = m_h5path + "/" + m_h5name;
                else m_h5PathName = m_h5name;

                if (input.has("key")) {
                    m_key = boost::replace_all_copy(input.get<string > ("key"), ".", "/");                    
                } else {
                    m_key = m_h5PathName;
                }

                if (m_key.size() == 0 || m_h5name.size() == 0) {
                    throw KARABO_PARAMETER_EXCEPTION("Name cannot be an empty string");
                }
                m_config = input;
            }

            const string& Element::getFullName() {
                return m_h5PathName;
            }

            void Element::getElement(Hash& element) {
                element.set(m_h5name, shared_from_this());
            }

            void Element::openParentGroup(std::map<std::string, hid_t >& groups) {

                typedef std::map<std::string, hid_t> H5GroupsMap;

                try {

                    H5GroupsMap::iterator it = groups.find(m_h5path);
                    if (it != groups.end()) {
                        m_parentGroup = it->second;
                    } else {
                        //

                        std::vector<std::string> tokens;
                        boost::split(tokens, m_h5path, boost::is_any_of("/"));
                        hid_t groupId = groups[""];
                        std::string relativePath;

                        for (size_t i = 0; i < tokens.size(); ++i) {
                            // skip empty tokens (like in: "/a/b//c" -> "a","b","","c") 
                            if (tokens[i].size() == 0) continue;
                            relativePath += "/" + tokens[i];
                            if (H5Lexists(groupId, relativePath.c_str(), H5P_DEFAULT) != 0) {
                                continue;
                            } else {
                                m_parentGroup = H5Gcreate(groupId, m_h5path.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                                KARABO_CHECK_HDF5_STATUS(m_parentGroup);
                                groups[m_h5path] = m_parentGroup;
                                return;
                            }
                        }
                        m_parentGroup = H5Gopen(groupId, m_h5path.c_str(), H5P_DEFAULT);
                        KARABO_CHECK_HDF5_STATUS(m_parentGroup);
                        groups[m_h5path] = m_parentGroup;
                    }

                } catch (...) {
                    KARABO_RETHROW_AS( KARABO_PROPAGATED_EXCEPTION("Could not create one of the parent group"));
                }
            }




        }
    }
}

