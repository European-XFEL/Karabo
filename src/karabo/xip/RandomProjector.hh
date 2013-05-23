/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on November 17, 2011, 6:00 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_XIP_RANDOMPROJECTOR_HH
#define	KARABO_XIP_RANDOMPROJECTOR_HH

#include <karabo/util/Factory.hh>
#include "Projector.hh"
#include "SingleProcessor.hh"
#include "CpuImage.hh"
#include "CpuImageList.hh"

namespace karabo {
    namespace xip {

        template <class TPix>
        class RandomProjector : public Projector< CpuImage<TPix> > {

        public:

            KARABO_CLASSINFO(RandomProjector, "Random", "1.0")

            /**
             * Necessary method as part of the factory/configuration system
             * @param expected [out] Description of expected parameters for this object (Schema)
             */
            static void expectedParameters(karabo::util::Schema& expected) {
                using namespace karabo::util;

                INT32_ELEMENT(expected).key("nProjections")
                        .displayedName("Number of Projections")
                        .description("The total number of 2D projections from the 3D model")
                        .minInc(1)
                        .unitName("number")
                        .unitSymbol("#")
                        .assignmentOptional().defaultValue(1024)
                        .commit();

                CHOICE_ELEMENT<SingleProcessor<CpuImage<TPix> > > (expected).key("processProjection")
                        .displayedName("Process Projection")
                        .description("Post process the 2D projections with this processor")
                        .assignmentOptional().noDefaultValue()
                        .commit();

            }

            /**
             * If this object is constructed using the factory/configuration system this method is called
             * @param input Validated (@see expectedParameters) and default-filled configuration
             */
            void configure(const karabo::util::Hash & input) {


            }

            CpuImageList<TPix> project(const CpuImage<TPix>& volume3d, const CpuImage<TPix>& referencePlane = CpuImage<TPix>()) const {

                for (int i = 0; i < m_nRotations; ++i) {

                    CpuImage<TPix> projection();

                    const CpuImage<TPix> rotMat = getRandomRotation();

                    for (int j = 0; j < referencePlane.dimX(); ++j) {
                        CpuImage<TPix> vRef = CpuImage<TPix>::vector(referencePlane(j, 0), referencePlane(j, 1), referencePlane(j, 2));
                        CpuImage<TPix> vRot = rotMat * vRef;


                    }


                }


            }


        private: // functions

            CpuImage<TPix> getRandomRotation() {

                std::vector<double> quat(4, 0.0);

                double qq;
                do {
                    qq = 0.0;
                    for (int i = 0; i < 4; ++i) {
                        quat[i] = ((double) rand()) / RAND_MAX - 0.5;
                        qq += quat[i] * quat[i];
                    }
                } while (qq > .25);

                qq = std::sqrt(qq);
                for (int i = 0; i < 4; ++i) {
                    quat[i] /= qq;
                }
                return CpuImage<TPix>::rotationMatrix3x3(quat[0], quat[1], quat[2], quat[3], true);
            }

        private: // members

            int m_nRotations;

        };
    }
}

#endif	

