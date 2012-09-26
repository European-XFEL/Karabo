/*
 * $Id: FLArrayFilterBuffer.hh 6095 2012-05-08 10:05:56Z boukhele $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */




#ifndef KARABO_IO_HDF5_FLARRAYFILTERBUFFER_HH
#define	KARABO_IO_HDF5_FLARRAYFILTERBUFFER_HH

#include <karabo/util/Factory.hh>
#include "../iodll.hh"


namespace karabo {

  namespace io {


    class ArrayDimensions;

    namespace hdf5 {

      template<class T> class FixedLengthArray;

      template<class T>
      class FLArrayFilterBuffer {
      public:

        KARABO_CLASSINFO(FLArrayFilterBuffer, "FLArrayFilterBuffer", "1.0")
        KARABO_FACTORY_BASE_CLASS

        virtual ~FLArrayFilterBuffer() {
        }

        virtual void write(const FixedLengthArray<T>& element, const boost::any& any, const ArrayDimensions& dims, size_t len) = 0;
        virtual void read(const FixedLengthArray<T>& element, boost::any& any, ArrayDimensions& dims, size_t len) = 0;

      };

    }
  }
}

KARABO_REGISTER_FACTORY_BASE_HH(karabo::io::hdf5::FLArrayFilterBuffer<karabo::util::Hash>, TEMPLATE_IO, DECLSPEC_IO)

#endif	/* KARABO_IO_HDF5_FLARRAYFILTER_HH */

