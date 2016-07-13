/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */



#ifndef KARABO_XIP_ENVIRONMENT_HH
#define	KARABO_XIP_ENVIRONMENT_HH

#include <karabo/util/Factory.hh>
#include "AbstractImage.hh"

namespace karabo {
    namespace xip {

        template <class TPix>
        class Environment {


            typedef typename boost::shared_ptr<AbstractImage<TPix> > AbstractImagePointer;

        public:

            KARABO_CLASSINFO(Environment, "Environment", "1.0")


            virtual void printInfo() const = 0;

            /***************************************
             *          Image Constructors         *
             ***************************************/

            virtual AbstractImagePointer image() = 0;

            virtual AbstractImagePointer image(const std::string& filename) = 0;

            virtual AbstractImagePointer image(const size_t dx, const size_t dy = 1, const size_t dz = 1) = 0;

            virtual AbstractImagePointer image(const size_t dx, const size_t dy, const size_t dz, const TPix& value) = 0;

            virtual AbstractImagePointer image(const size_t dx, const size_t dy, const size_t dz, const std::string& values, const bool repeatValues) = 0;

            virtual AbstractImagePointer image(const TPix * const dataBuffer, const size_t dx, const size_t dy, const size_t dz) = 0;

            virtual AbstractImagePointer image(const std::vector<TPix>& dataBuffer, const size_t dx, const size_t dy, const size_t dz) = 0;

            virtual AbstractImagePointer image(const karabo::util::Hash& header) = 0;

            virtual AbstractImagePointer image(const karabo::util::Hash& header, const TPix& value) = 0;

        };



    }
}

#endif

