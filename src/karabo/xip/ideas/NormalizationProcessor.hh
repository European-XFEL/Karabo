/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_XIP_NORMALIZATIONPROCESSOR_HH
#define	KARABO_XIP_NORMALIZATIONPROCESSOR_HH

#include <karabo/util/Factory.hh>
#include "SingleProcessor.hh"
#include "CpuImage.hh"

namespace karabo {
    namespace xip {

        template <class TPix>
        class NormalizationProcessor : public SingleProcessor< CpuImage<TPix> > {

        public:

            KARABO_CLASSINFO(NormalizationProcessor, "Normalization", "1.0")

            /**
             * Necessary method as part of the factory/configuration system
             * @param expected[out] Description of expected parameters for this object (Schema)
             */
            static void expectedParameters(karabo::util::Schema & expected) {

                using namespace karabo::util;

                FLOAT_ELEMENT(expected).key("mean")
                        .displayedName("Mean")
                        .description("New arithmetic mean of the image")
                        .assignmentOptional().defaultValue(0.0)
                        .commit();

                FLOAT_ELEMENT(expected).key("sigma")
                        .displayedName("Sigma")
                        .description("New standard deviation (only applied if sigma > 0.0")
                        .assignmentOptional().defaultValue(0.0)
                        .commit();
            }

            /**
             * If this object is constructed using the factory/configuration system this method is called
             * @param input Validated (@see expectedParameters) and default-filled configuration
             */
            void configure(const karabo::util::Hash & input) {
                input.get("mean", m_mean);
                input.get("sigma", m_sigma);
            }

            virtual void processInPlace(CpuImage<TPix>& inOut) {
                size_t size = inOut.size();

                double sum = 0.0;
                double sumSq = 0.0;
                double mean = 0.0;
                double var = 0.0;
                for (size_t i = 0; i < size; ++i) {
                    double val = inOut[i];
                    sum += val;
                    sumSq += val*val;
                }
                mean = sum / (double) size;
                var = sumSq / (double) size - mean*mean;
                float oldSigma = std::sqrt(var);
                float factor = 1.0;
                if (m_sigma > 0.0 && oldSigma != 0.0) factor = m_sigma / oldSigma;
                for (size_t i = 0; i < size; ++i) {
                    inOut[i] = ((inOut[i] - mean) * factor) + m_mean;
                }
            }

            virtual CpuImage<TPix> process(const CpuImage<TPix>& in) {
                CpuImage<TPix> out(in);
                processInPlace(out);
                return out;
            }

        private:

            float m_mean;
            float m_sigma;

        };

    }
}

#endif

