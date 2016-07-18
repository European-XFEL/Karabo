/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_XIP_XIP_HH
#define	KARABO_XIP_XIP_HH

#include <karabo/util/Factory.hh>
#include <karabo/core/Module.hh>

/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package xip
     */
    namespace xip {

        class Xip : public karabo::core::Module {

            public:
            KARABO_CLASSINFO("Xip")

            Xip();
            virtual ~Xip();


            static void expectedParameters(karabo::util::Config& expected);
            void configure(const karabo::util::Config& input);

            void compute();

        };

    }
}

#endif	/* KARABO_XIP_XIP_HH */
