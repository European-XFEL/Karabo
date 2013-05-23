/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "Group.hh"
#include <karabo/util/SimpleElement.hh>

using namespace karabo::io::h5;
using namespace karabo::util;

namespace karabo {
    namespace io {
        namespace h5 {


            KARABO_REGISTER_FOR_CONFIGURATION(Element, Group)

            void Group::expectedParameters(karabo::util::Schema& expected) {

                STRING_ELEMENT(expected)
                        .key("type")
                        .displayedName("Type")
                        .description("Hash or vector<Hash>")
                        .options("HASH, VECTOR_HASH")
                        .tags("persistent")
                        .assignmentOptional().noDefaultValue()
                        .commit();

                UINT64_ELEMENT(expected)
                        .key("size")
                        .displayedName("Vector Size")
                        .description("Number of Hashes in the Vector")
                        .tags("persistent")
                        .minExc(0)
                        .assignmentOptional().noDefaultValue()
                        .commit();
            }


            void Group::create(hsize_t chunkSize) {
                hid_t lcpl = H5Pcreate(H5P_LINK_CREATE);
                KARABO_CHECK_HDF5_STATUS(lcpl);
                KARABO_CHECK_HDF5_STATUS(H5Pset_create_intermediate_group(lcpl, 1));
                hid_t gcpl = H5Pcreate(H5P_GROUP_CREATE);
                KARABO_CHECK_HDF5_STATUS(H5Pset_link_creation_order(gcpl, H5P_CRT_ORDER_TRACKED));
                m_group = H5Gcreate(m_parentGroup, m_h5name.c_str(), lcpl, gcpl, H5P_DEFAULT);
                KARABO_CHECK_HDF5_STATUS(m_group);
                H5Gclose(m_group); //???
            }


            void Group::create(hid_t tableGroup) {
                hid_t lcpl = H5Pcreate(H5P_LINK_CREATE);
                KARABO_CHECK_HDF5_STATUS(lcpl);
                KARABO_CHECK_HDF5_STATUS(H5Pset_create_intermediate_group(lcpl, 1));
                hid_t gcpl = H5Pcreate(H5P_GROUP_CREATE);
                KARABO_CHECK_HDF5_STATUS(H5Pset_link_creation_order(gcpl, H5P_CRT_ORDER_TRACKED));
                m_group = H5Gcreate(tableGroup, m_h5PathName.c_str(), lcpl, gcpl, H5P_DEFAULT);
                KARABO_CHECK_HDF5_STATUS(m_group);
                H5Gclose(m_group); 
            }


            void Group::createAttributes(hid_t element) {
                for (size_t i = 0; i < m_attributes.size(); ++i) {
                    m_attributes[i]->create(m_group);
                }
            }


            hid_t Group::openElement(hid_t group) {
                m_group = H5Gopen(group, m_h5PathName.c_str(), H5P_DEFAULT);
                KARABO_CHECK_HDF5_STATUS(m_group);
                return m_group;
            }


            void Group::close() {
                KARABO_CHECK_HDF5_STATUS(H5Gclose(m_group));
            }


            void Group::bind(karabo::util::Hash& data) {
                if (!data.has(m_key, '/')) {
                    if (m_isVectorHash) {
                        vector<Hash>& vh = data.bindReference<vector<Hash> > (m_key, '/');
                        vh.resize(m_vectorSize);
                    } else {
                        Hash h = data.bindReference<Hash>(m_key, '/');
                    }

                }
            }


            void Group::bind(karabo::util::Hash& data, hsize_t len) {
                if (!data.has(m_key, '/')) {
                    if (m_isVectorHash) {
                        vector<Hash>& vh = data.bindReference<vector<Hash> > (m_key, '/');
                        vh.resize(m_vectorSize * len);
                    } else {
                        vector<Hash>& vh = data.bindReference<vector<Hash> > (m_key, '/');
                        vh.resize(len);
                    }
                }
            }

        }
    }
}
