/*
 * $Id: Format.cc 6095 2012-05-08 10:05:56Z boukhele $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "Element.hh"
#include "karabo/util/ListElement.hh"
#include <karabo/util/Factory.hh>
#include <karabo/util/Configurator.hh>
#include <karabo/util/SimpleElement.hh>
#include <karabo/io/h5/Attribute.hh>


using namespace karabo::util;


namespace karabo {
    namespace io {
        namespace h5 {

            void Element::expectedParameters(Schema& expected) {

                STRING_ELEMENT(expected)
                        .key("name")
                        .displayedName("Name")
                        .description("Group or dataset name. i.e.: d1, g4.d2")
                        .assignmentOptional().defaultValue("aa")
                        //.assignmentMandatory()
                        .reconfigurable() // ???
                        .commit();


                STRING_ELEMENT(expected)
                        .key("path")
                        .displayedName("Path")
                        .description("Path to that element. i.e. instrument.XXX.LPD")
                        .assignmentOptional().defaultValue("instrument")
                        //.assignmentMandatory()
                        .reconfigurable() //???
                        .commit();

                STRING_ELEMENT(expected)
                        .key("type")
                        .displayedName("Type")
                        .description("Type")
                        .assignmentOptional().defaultValue("INT32")
                        //.assignmentMandatory()
                        .reconfigurable() //???
                        .commit();

//                INT32_ELEMENT(expected)
//                        .key("compressionLevel")
//                        .displayedName("Use Compression Level")
//                        .description("Defines compression level: [0-9]. 0 - no compression (default), 9 - attempt the best compression.")
//                        .minInc(0).maxInc(9)
//                        .assignmentOptional().defaultValue(0)
//                        .reconfigurable() // ???
//                        .commit();

                LIST_ELEMENT(expected)
                        .key("attributes")
                        .displayedName("Attributes")
                        .description("Definition of hdf5 objects.")
                        .appendNodesOfConfigurationBase<Attribute > ()
                        .assignmentOptional().noDefaultValue() //TODO
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
                m_key = input.get<string > ("name");
                m_path = input.get<string > ("path");
                if (m_path != "") m_path_key = m_path + "/" + m_key;
                else m_path_key = m_key;
                
                if (m_key.size() == 0) {
                    throw KARABO_PARAMETER_EXCEPTION("Name cannot be an empty string");
                }
            }

            const string& Element::getName() {
                return m_key;
            }

            void Element::getElement(Hash& element) {
                element.set(m_key, shared_from_this());
            }

            void Element::openParentGroup(std::map<std::string, boost::shared_ptr<H5::Group> >& groups) {

                typedef std::map<std::string, boost::shared_ptr<H5::Group> > H5GroupsMap;

                try {

                    H5GroupsMap::iterator it = groups.find(m_path);
                    if (it != groups.end()) {
                        m_group = it->second;
                    } else {
                        //

                        std::vector<std::string> tokens;
                        boost::split(tokens, m_path, boost::is_any_of("/"));
                        boost::shared_ptr<H5::Group> groupPtr = groups[""];
                        std::string relativePath;

                        for (size_t i = 0; i < tokens.size(); ++i) {
                            // skip empty tokens (like in: "/a/b//c" -> "a","b","","c") 
                            if (tokens[i].size() == 0) continue;
                            relativePath += "/" + tokens[i];
                            if (H5Lexists(groupPtr->getLocId(), relativePath.c_str(), H5P_DEFAULT) != 0) {
                                continue;
                            } else {
                                m_group = boost::shared_ptr<H5::Group > (new H5::Group(groupPtr->createGroup(m_path.c_str())));
                                groups[m_path] = m_group;
                                return;
                            }
                        }
                        m_group = boost::shared_ptr<H5::Group > (new H5::Group(groupPtr->openGroup(m_path.c_str())));
                        groups[m_path] = m_group;

                    }

                } catch (...) {
                    KARABO_RETHROW
                }
            }




        }
    }
}

