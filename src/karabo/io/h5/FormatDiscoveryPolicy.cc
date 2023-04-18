/*
 * $Id: FormatDiscoveryPolicy.cc 9598 2013-05-05 10:52:42Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#include "FormatDiscoveryPolicy.hh"

#include <karabo/log/Logger.hh>
#include <karabo/util/SimpleElement.hh>

using namespace karabo::io::h5;
using namespace karabo::util;

KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::FormatDiscoveryPolicy)

namespace karabo {
    namespace io {
        namespace h5 {

            void FormatDiscoveryPolicy::expectedParameters(karabo::util::Schema& expected) {
                UINT64_ELEMENT(expected)
                      .key("chunkSize")
                      .displayedName("Default Chunk Size")
                      .description("Default chunk size for discovery")
                      .assignmentOptional()
                      .defaultValue(1)
                      .commit();

                UINT32_ELEMENT(expected)
                      .key("compressionLevel")
                      .displayedName("Default compression Level")
                      .description("Default compression Level")
                      .minInc(0)
                      .maxInc(9)
                      .assignmentOptional()
                      .defaultValue(0)
                      .commit();
            }


            FormatDiscoveryPolicy::FormatDiscoveryPolicy(const karabo::util::Hash& input) {
                input.get("chunkSize", m_defaultChunkSize);
                m_defaultCompressionLevel = input.getAs<int>("compressionLevel");
            }

        } // namespace h5
    }     // namespace io
} // namespace karabo
