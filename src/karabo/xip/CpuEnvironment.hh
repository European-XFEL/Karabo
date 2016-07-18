/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_XIP_CPUENVIRONMENT_HH
#define	KARABO_XIP_CPUENVIRONMENT_HH

#include <karabo/util/Factory.hh>
#include "Environment.hh"
#include "CpuImage.hh"

namespace karabo {
    namespace xip {

        template <class TPix>
        class CpuEnvironment : public Environment<TPix> {


            typedef typename boost::shared_ptr<AbstractImage<TPix> > AbstractImagePointer;

        public:

            KARABO_CLASSINFO(CpuEnvironment, "cpu", "1.0")

            virtual ~CpuEnvironment() {
            }

            static void expectedParameters(karabo::util::Schema& expected) {
            }

            void configure(const karabo::util::Hash & input) {
            }

            void printInfo() const {
                std::cout << "\nInitialized regular CPU environment\n" << std::endl;
            }

            /***************************************
             *          Image Constructors         *
             ***************************************/

            virtual AbstractImagePointer image() {
                return AbstractImagePointer(new CpuImage<TPix > ());
            }

            virtual AbstractImagePointer image(const std::string& filename) {
                return AbstractImagePointer(new CpuImage<TPix > (filename));
            }

            virtual AbstractImagePointer image(const size_t dx, const size_t dy = 1, const size_t dz = 1) {
                return AbstractImagePointer(new CpuImage<TPix > (dx, dy, dz));
            }

            virtual AbstractImagePointer image(const size_t dx, const size_t dy, const size_t dz, const TPix& value) {
                return AbstractImagePointer(new CpuImage<TPix > (dx, dy, dz, value));
            }

            virtual AbstractImagePointer image(const size_t dx, const size_t dy, const size_t dz, const std::string& values, const bool repeatValues) {
                return AbstractImagePointer(new CpuImage<TPix > (dx, dy, dz, values, repeatValues));
            }

            virtual AbstractImagePointer image(const TPix * const dataBuffer, const size_t dx, const size_t dy, const size_t dz) {
                return AbstractImagePointer(new CpuImage<TPix > (dataBuffer, dx, dy, dz));
            }

            virtual AbstractImagePointer image(const std::vector<TPix>& dataBuffer, const size_t dx, const size_t dy, const size_t dz) {
                return AbstractImagePointer(new CpuImage<TPix > (dataBuffer, dx, dy, dz));
            }

            virtual AbstractImagePointer image(const karabo::util::Hash& header) {
                return AbstractImagePointer(new CpuImage<TPix > (header));
            }

            virtual AbstractImagePointer image(const karabo::util::Hash& header, const TPix& value) {
                return AbstractImagePointer(new CpuImage<TPix > (header, value));
            }

        };
    }
}

#endif

