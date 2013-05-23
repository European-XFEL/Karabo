/*
 * $Id$
 *
 * File:   QuaternionTransform.hh
 * Author: <your.email@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_XIP_QUATERNIONTRANSFORM_HH
#define	KARABO_XIP_QUATERNIONTRANSFORM_HH

#include <karabo/util/Factory.hh>

#include "SingleProcessor.hh"
#include "CpuImage.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package xip
     */
    namespace xip {

        template <class TPix>
        class QuaternionTransform : public SingleProcessor< CpuImage<TPix> > {

        public:

            QuaternionTransform(double x, double y, double z, double w);

            /**
             * Necessary method as part of the factory/configuration system
             * @param expected [out] Description of expected parameters for this object (Schema)
             */
            static void expectedParameters(karabo::util::Schema & expected);


            /**
             * If this object is constructed using the factory/configuration system this method is called
             * @param input Validated (@see expectedParameters) and default-filled configuration
             */
            void configure(const karabo::util::Hash & input);

            virtual void processInPlace(CpuImage<TPix> & image);

            virtual CpuImage<TPix> process(const CpuImage<TPix>& image) const;




        };

    }
}

#endif	/* KARABO_XIP_QUATERNIONTRANSFORM_HH */
