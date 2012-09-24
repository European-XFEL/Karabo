/*
 * $Id: Writer.hh 4981 2012-01-20 14:13:02Z wegerk $
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on August 26, 2010, 1:38 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef EXFEL_IO_WRITER_HH
#define	EXFEL_IO_WRITER_HH

#include <karabo/util/Factory.hh>
#include "iodll.hh"

/**
 * The main European XFEL namespace
 */
namespace exfel {
    namespace io {

        /**
         * The Writer class.
         */
        template <class Tdata>
        class Writer {
        public:

            EXFEL_CLASSINFO(Writer<Tdata>, "Writer", "1.0")

            EXFEL_FACTORY_BASE_CLASS

            Writer() {
            }

            virtual ~Writer() {
            }

            virtual void write(const Tdata& data) = 0;

            virtual void update() {
                // Does nothing per default
            }

        };
    }
}

EXFEL_REGISTER_FACTORY_BASE_HH(exfel::io::Writer<exfel::util::Hash>, TEMPLATE_IO, DECLSPEC_IO)
EXFEL_REGISTER_FACTORY_BASE_HH(exfel::io::Writer<exfel::util::Schema>, TEMPLATE_IO, DECLSPEC_IO)

#endif
