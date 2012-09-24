/*
 * $Id: Reader.hh 4981 2012-01-20 14:13:02Z wegerk $
 *
 * File:   Reader.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on August 16, 2010, 3:15 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef EXFEL_IO_READER_HH
#define	EXFEL_IO_READER_HH

#include <karabo/util/Factory.hh>
#include "iodll.hh"

/**
 * The main European XFEL namespace
 */
namespace exfel {

    /**
     * Namespace for package io
     */
    namespace io {

        /**
         * The Reader class.
         */
        template <class Tdata>
        class Reader {

            protected:

            typedef boost::function<void (const Tdata&) > ReadHandler;
            
            public:

            EXFEL_CLASSINFO(Reader<Tdata>, "Reader", "1.0")

            EXFEL_FACTORY_BASE_CLASS

            Reader() {
            }

            virtual ~Reader() {
            }

            virtual void read(Tdata& data) = 0;

            virtual void readAsync(const ReadHandler& readHandler) {
            }

            virtual size_t size() const {
                throw NOT_SUPPORTED_EXCEPTION("This reader does not support size determination prior to reading");
            }
        };    

    } // namespace io
} // namespace exfel

EXFEL_REGISTER_FACTORY_BASE_HH(exfel::io::Reader<exfel::util::Hash>, TEMPLATE_IO, DECLSPEC_IO)
EXFEL_REGISTER_FACTORY_BASE_HH(exfel::io::Reader<exfel::util::Schema>, TEMPLATE_IO, DECLSPEC_IO)

#endif	/* EXFEL_IO_READER_HH */
