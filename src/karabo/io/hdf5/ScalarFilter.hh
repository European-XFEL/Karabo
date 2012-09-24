/*
 * $Id: ScalarFilter.hh 6095 2012-05-08 10:05:56Z boukhele $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */




#ifndef EXFEL_IO_HDF5_SCALARFILTER_HH
#define	EXFEL_IO_HDF5_SCALARFILTER_HH

#include <karabo/util/Factory.hh>
#include "../ArrayView.hh"
#include "../iodll.hh"



namespace exfel {
  
  namespace io {
        
    namespace hdf5 {

      template<class T> class Scalar;
      
      
      template<class T>
      class ScalarFilter {
      public:

        EXFEL_CLASSINFO(ScalarFilter, "ScalarFilter", "1.0")
        EXFEL_FACTORY_BASE_CLASS

        virtual ~ScalarFilter() {
        }

        virtual void write(const Scalar<T>& element, const boost::any& any, const size_t len) = 0;
        virtual void read(const Scalar<T>& element, boost::any& any, size_t len) = 0;

      };      
    }
  }
}

EXFEL_REGISTER_FACTORY_BASE_HH(exfel::io::hdf5::ScalarFilter<exfel::util::Hash>, TEMPLATE_IO, DECLSPEC_IO)

#endif	/* EXFEL_IO_HDF5_SCALARFILTER_HH */

