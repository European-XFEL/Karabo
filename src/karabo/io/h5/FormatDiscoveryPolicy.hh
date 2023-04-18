/*
 * $Id: FormatDiscoveryPolicy.hh 9598 2013-05-05 10:52:42Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#ifndef KARABO_IO_H5_FORMATDISCOVERYPOLICY_HH
#define KARABO_IO_H5_FORMATDISCOVERYPOLICY_HH

#include <karabo/util/Configurator.hh>
#include <string>


namespace karabo {

    namespace io {

        namespace h5 {

            /**
             * @class FormatDiscoveryPolicy
             * @brief This class specifies default to be used durng Format discovery
             */
            class FormatDiscoveryPolicy {
               public:
                KARABO_CLASSINFO(FormatDiscoveryPolicy, "Policy", "1.0")
                KARABO_CONFIGURATION_BASE_CLASS


                /**
                 * FormatDiscoveryPolicy's expected parameters:
                 *
                 * - chunkSize: default chunk size to use when chunking data in HDF5 (1)
                 * - compressionLevel: default compression level to use in HDF5 (0)
                 * @param expected
                 */
                static void expectedParameters(karabo::util::Schema& expected);

                FormatDiscoveryPolicy(const karabo::util::Hash& input);

                virtual ~FormatDiscoveryPolicy() {}

                virtual void discover() {}

                /**
                 * Return the default compression level
                 * @return
                 */
                int getDefaultCompressionLevel() const {
                    return m_defaultCompressionLevel;
                }

                /**
                 * Return the default chunk size
                 * @return
                 */
                unsigned long long getDefaultChunkSize() const {
                    return m_defaultChunkSize;
                }

               private:
                int m_defaultCompressionLevel;
                unsigned long long m_defaultChunkSize;
            };


        } // namespace h5
    }     // namespace io
} // namespace karabo

#endif
