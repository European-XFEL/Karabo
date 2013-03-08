/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "Group.hh"
using namespace karabo::io::h5;

namespace karabo {
    namespace io {
        namespace h5 {

            KARABO_REGISTER_FOR_CONFIGURATION(Element, Group)

            void Group::expectedParameters(karabo::util::Schema& expected) {
            }

            void Group::create(hsize_t chunkSize) {
                    m_group = H5Gcreate(m_parentGroup, m_h5name.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                    KARABO_CHECK_HDF5_STATUS(m_group);
                    H5Gclose(m_group);
            }

            void Group::open(hid_t group) {

                m_group = H5Gopen(group, m_h5PathName.c_str(), H5P_DEFAULT);
                KARABO_CHECK_HDF5_STATUS(m_parentGroup);
            }
            
            void Group::close() {
                KARABO_CHECK_HDF5_STATUS(H5Gclose(m_group));
            }



        }
    }
}
