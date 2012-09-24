/*
 * $Id: FLArrayFilter.hh 6095 2012-05-08 10:05:56Z boukhele $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */




#ifndef EXFEL_IO_HDF5_FLARRAYFILTER_HH
#define	EXFEL_IO_HDF5_FLARRAYFILTER_HH

#include <karabo/util/Factory.hh>
#include <boost/any.hpp>
#include "../iodll.hh"


namespace exfel {

  namespace io {


    class ArrayDimensions;

    namespace hdf5 {

      template<class T> class FixedLengthArray;

      template<class T>
      class FLArrayFilter {
      public:

        EXFEL_CLASSINFO(FLArrayFilter, "FLArrayFilter", "1.0")
        EXFEL_FACTORY_BASE_CLASS

        virtual ~FLArrayFilter() {
        }

        virtual void write(const FixedLengthArray<T>& element, const boost::any& any, const ArrayDimensions& dims) = 0;
        virtual void read(const FixedLengthArray<T>& element, boost::any& any, ArrayDimensions& dims) = 0;

      };
    }
  }
}

EXFEL_REGISTER_FACTORY_BASE_HH(exfel::io::hdf5::FLArrayFilter<exfel::util::Hash>, TEMPLATE_IO, DECLSPEC_IO)

#endif	/* EXFEL_IO_HDF5_FLARRAYFILTER_HH */

