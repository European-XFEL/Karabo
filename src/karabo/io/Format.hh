/*
 * $Id: Format.hh 5133 2012-02-14 14:52:12Z heisenb $
 *
 * File:   Format.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on August 20, 2010, 10:35 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_IO_FORMAT_HH
#define	KARABO_IO_FORMAT_HH

#include <sstream>
#include <karabo/util/Factory.hh>
#include "iodll.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {
  namespace io {

    template <class Tdata>
    class Format {
    public:

      KARABO_CLASSINFO(Format, "Format", "1.0")

      KARABO_FACTORY_BASE_CLASS

      /**
       * Converts from data object to buffer
       * @param from Data object to be converted from
       * @param to Buffered version of data object
       */
      virtual void convert(const Tdata& in, std::stringstream& out) {
      }

      /**
       * Converts a buffer to data object
       * @param from Buffer containing object data
       * @param to Data object being filled by the buffer
       */
      virtual void convert(std::stringstream& in, Tdata& out) {
      }
      
      Tdata unserialize(const std::string& in) {
          std::stringstream ss;
          ss << in;
          Tdata ret;
          convert(ss, ret);
          return ret;
      }
      
      std::string serialize(const Tdata& in) {
          std::stringstream ss;
          convert(in, ss);
          return ss.str();
      }

      virtual ~Format() {
      }

    };
  } 
} 

KARABO_REGISTER_FACTORY_BASE_HH(karabo::io::Format<karabo::util::Hash>, TEMPLATE_IO, DECLSPEC_IO)

#endif
