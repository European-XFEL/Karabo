/*
 * $Id: FLArrayFilter.hh 6095 2012-05-08 10:05:56Z boukhele $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */




#ifndef KARABO_IO_HDF5_FLARRAYFILTER_HH
#define	KARABO_IO_HDF5_FLARRAYFILTER_HH

#include <karabo/util/Factory.hh>
#include <boost/any.hpp>
#include "../iodll.hh"


namespace karabo {

  namespace io {


    class ArrayDimensions;

    namespace hdf5 {

      template<class T> class FixedLengthArray;

      template<class T>
      class FLArrayFilter {
      public:

        KARABO_CLASSINFO(FLArrayFilter, "FLArrayFilter", "1.0")
        KARABO_FACTORY_BASE_CLASS

        virtual ~FLArrayFilter() {
        }

        virtual void write(const FixedLengthArray<T>& element, const boost::any& any, const ArrayDimensions& dims) = 0;
        virtual void read(const FixedLengthArray<T>& element, boost::any& any, ArrayDimensions& dims) = 0;

      };
    }
  }
}

//KARABO_REGISTER_FACTORY_BASE_HH(karabo::io::hdf5::FLArrayFilter<karabo::util::Hash>, TEMPLATE_IO, DECLSPEC_IO)

#endif	/* KARABO_IO_HDF5_FLARRAYFILTER_HH */

