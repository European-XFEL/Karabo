/*
 * $Id: FormatDiscoveryPolicy.hh 9598 2013-05-05 10:52:42Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_IO_H5_FORMATDISCOVERYPOLICY_HH
#define	KARABO_IO_H5_FORMATDISCOVERYPOLICY_HH

#include <string>
#include <karabo/util/Configurator.hh>


namespace karabo {

    namespace io {

        namespace h5 {

            class FormatDiscoveryPolicy {

                public:

                KARABO_CLASSINFO(FormatDiscoveryPolicy, "Policy", "1.0")
                KARABO_CONFIGURATION_BASE_CLASS


                static void expectedParameters(karabo::util::Schema& expected);

                FormatDiscoveryPolicy(const karabo::util::Hash& input);

                virtual ~FormatDiscoveryPolicy() {
                }

                virtual void discover() {
                }

                int getDefaultCompressionLevel() const {
                    return m_defaultCompressionLevel;
                }

                unsigned long long getDefaultChunkSize() const {
                    return m_defaultChunkSize;
                }

            private:
                int m_defaultCompressionLevel;
                unsigned long long m_defaultChunkSize;

            };


        }
    }
}

#endif	
