/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_XIP_NOISEPROCESSOR_HH
#define	KARABO_XIP_NOISEPROCESSOR_HH

#include <karabo/util/Factory.hh>
#include "SingleProcessor.hh"
#include "CpuImage.hh"

namespace karabo {
    namespace xip {

        template <class TImage>
        class NoiseType {

        public:

            KARABO_CLASSINFO(NoiseType, "NoiseType", "1.0")
            KARABO_CONFIGURATION_BASE_CLASS


            virtual void processInPlace(TImage& image) = 0;

            virtual TImage process(const TImage& image) = 0;
        };

        template <class TPix>
        class PoissonNoise : public NoiseType< CpuImage<TPix> > {

        public:

            KARABO_CLASSINFO(PoissonNoise, "Poisson", "1.0")

            static void expectedParameters(karabo::util::Schema& expected) {
            }

            PoissonNoise(const karabo::util::Hash & input) {
            }

            virtual void processInPlace(CpuImage<TPix>& image) {
                image.getCImg().noise(1.0, 3);
            }

            virtual CpuImage<TPix> process(const CpuImage<TPix>& image) {
                CpuImage<TPix> tmp(image);
                this->processInPlace(tmp);
                return tmp;
            }

        };

        template <class TPix>
        class GaussianNoise : public NoiseType< CpuImage<TPix> > {

            float m_sigma;

        public:

            KARABO_CLASSINFO(GaussianNoise, "Gaussian", "1.0")

            /**
             * Necessary method as part of the factory/configuration system
             * @param expected [out] Description of expected parameters for this object (Schema)
             */
            static void expectedParameters(karabo::util::Schema& expected) {

                using namespace karabo::util;

                FLOAT_ELEMENT(expected).key("sigma")
                        .displayedName("Sigma")
                        .description("Amplitude of the random additive noise")
                        .assignmentOptional().defaultValue(1.0)
                        .commit();
            }

            
            GaussianNoise(const karabo::util::Hash & input) {
                input.get("sigma", m_sigma);
            }

            void processInPlace(CpuImage<TPix>& image) {
                image.getCImg().noise(m_sigma, 0);
            }

            CpuImage<TPix> process(const CpuImage<TPix>& image) {
                CpuImage<TPix> tmp(image);
                this->processInPlace(tmp);
                return tmp;
            }

        };

        template <class TPix>
        class NoiseProcessor : public SingleProcessor< CpuImage<TPix> > {

        public:

            KARABO_CLASSINFO(NoiseProcessor, "Noise", "1.0")

            /**
             * Necessary method as part of the factory/configuration system
             * @param expected[out] Description of expected parameters for this object (Schema)
             */
            static void expectedParameters(karabo::util::Schema & expected) {

                using namespace karabo::util;

                CHOICE_ELEMENT<NoiseType<CpuImage<TPix> > > (expected).key("type")
                        .displayedName("Type")
                        .description("The type of noise to be added")
                        .assignmentOptional().defaultValue("Poisson")
                        .commit();
            }

            /**
             * If this object is constructed using the factory/configuration system this method is called
             * @param input Validated (@see expectedParameters) and default-filled configuration
             */
            void configure(const karabo::util::Hash & input) {
                m_noiseType = NoiseType<CpuImage<TPix> >::createChoice("type", input);
            }

            virtual void processInPlace(CpuImage<TPix>& image) {
                m_noiseType->processInPlace(image);
            }

            virtual CpuImage<TPix> process(const CpuImage<TPix>& image) {
                return m_noiseType->process(image);
            }

        private:

            typename NoiseType< CpuImage<TPix> >::Pointer m_noiseType;
            //boost::shared_ptr<NoiseType<CpuImage<TPix> > > m_noiseType;

        };

    }
}

#endif

