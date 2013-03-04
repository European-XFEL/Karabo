/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "Dataset.hh"
#include <karabo/util/SimpleElement.hh>

using namespace karabo::io::h5;
using namespace karabo::util;

namespace karabo {
    namespace io {
        namespace h5 {

            void Dataset::expectedParameters(Schema& expected) {

                INT32_ELEMENT(expected)
                        .key("compressionLevel")
                        .displayedName("Use Compression Level")
                        .description("Defines compression level: [0-9]. 0 - no compression (default), 9 - attempt the best compression.")
                        .minInc(0).maxInc(9)
                        .assignmentOptional().defaultValue(0)
                        .reconfigurable()
                        .commit();

            }


        }
    }
}
