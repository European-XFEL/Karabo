/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <sstream> 
#include "Dataset.hh"
#include <karabo/util/SimpleElement.hh>

using namespace karabo::io::h5;
using namespace karabo::util;

namespace karabo {
    namespace io {
        namespace h5 {

            void Dataset::expectedParameters(Schema & expected) {

                INT32_ELEMENT(expected)
                        .key("compressionLevel")
                        .displayedName("Use Compression Level")
                        .description("Defines compression level: [0-9]. 0 - no compression (default), 9 - attempt the best compression.")
                        .minInc(0).maxInc(9)
                        .assignmentOptional().defaultValue(0)
                        .reconfigurable()
                        .commit();
            }

            void Dataset::open(hid_t group) {
                m_dataSet = H5Dopen2(group, m_h5PathName.c_str(), H5P_DEFAULT);
                KARABO_CHECK_HDF5_STATUS(m_dataSet);
                m_fileDataSpace = H5Dget_space(m_dataSet);
                KARABO_CHECK_HDF5_STATUS(m_fileDataSpace);
            }

            void Dataset::close() {
                KARABO_CHECK_HDF5_STATUS(H5Pclose(m_dataSetProperties));
                KARABO_CHECK_HDF5_STATUS(H5Dclose(m_dataSet));
            }

        }
    }
}
