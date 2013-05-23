/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on November 22, 2011, 7:10 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef EXFEL_XIP_ELLISOID3D_HH
#define	EXFEL_XIP_ELLISOID3D_HH

#include <exfel/util/Factory.hh>

#include "Generator.hh"
#include "CpuImage.hh"

namespace exfel {
    namespace xip {

        template <class TPix>
        class Cylinder3d : public Generator< CpuImage<TPix> > {

        public:

            EXFEL_CLASSINFO(Cylinder3d, "Cylinder3d", "1.0")

            /**
             * Necessary method as part of the factory/configuration system
             * @param expected [out] Description of expected parameters for this object (Schema)
             */
            static void expectedParameters(exfel::util::Schema& expected) {
                using namespace exfel::util;

                INT32_ELEMENT(expected).key("radius")
                        .displayedName("Radius")
                        .description("Cylinder radius")
                        .minInc(1)
                        .unitName("pixel")
                        .unitSymbol("px")
                        .assignmentOptional().defaultValue(10)
                        .commit();

                INT32_ELEMENT(expected).key("height")
                        .displayedName("Height")
                        .description("Cylinder height")
                        .minInc(1)
                        .unitName("pixel")
                        .unitSymbol("px")
                        .assignmentOptional().defaultValue(20)
                        .commit();

                INT32_ELEMENT(expected).key("resolution")
                        .displayedName("Resolution")
                        .description("The number of recursive subdivisions from an initial stretched icosahedron")
                        .assignmentOptional().defaultValue(200)
                        .commit();
            }

            /**
             * If this object is constructed using the factory/configuration system this method is called
             * @param input Validated (@see expectedParameters) and default-filled configuration
             */
            void configure(const exfel::util::Hash & input) {
                input.get("radius", m_radius);
                input.get("height", m_height);
                input.get("resolution", m_res);

            }

            void generate(CpuImage<TPix>& image) const {
                using namespace cimg_library;
                CImgList<TPix> faces3d;
                CImg<TPix> points3d = CImg<TPix>::cylinder3d(faces3d, m_radius, m_height, m_res);
                image.swap(CpuImage<TPix>(points3d));
            }

        private:

            int m_radius, m_height, m_res;

        };

    }
}



#endif

