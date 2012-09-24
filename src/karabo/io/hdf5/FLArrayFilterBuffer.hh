/*
 * $Id: FLArrayFilterBuffer.hh 6095 2012-05-08 10:05:56Z boukhele $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */




#ifndef EXFEL_IO_HDF5_FLARRAYFILTERBUFFER_HH
#define	EXFEL_IO_HDF5_FLARRAYFILTERBUFFER_HH

#include <karabo/util/Factory.hh>
#include "../iodll.hh"


namespace exfel {

  namespace io {


    class ArrayDimensions;

    namespace hdf5 {

      template<class T> class FixedLengthArray;

      template<class T>
      class FLArrayFilterBuffer {
      public:

        EXFEL_CLASSINFO(FLArrayFilterBuffer, "FLArrayFilterBuffer", "1.0")
        EXFEL_FACTORY_BASE_CLASS

        virtual ~FLArrayFilterBuffer() {
        }

        virtual void write(const FixedLengthArray<T>& element, const boost::any& any, const ArrayDimensions& dims, size_t len) = 0;
        virtual void read(const FixedLengthArray<T>& element, boost::any& any, ArrayDimensions& dims, size_t len) = 0;

      };

    }
  }
}

EXFEL_REGISTER_FACTORY_BASE_HH(exfel::io::hdf5::FLArrayFilterBuffer<exfel::util::Hash>, TEMPLATE_IO, DECLSPEC_IO)

#endif	/* EXFEL_IO_HDF5_FLARRAYFILTER_HH */

