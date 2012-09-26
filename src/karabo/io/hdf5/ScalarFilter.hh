/*
 * $Id: ScalarFilter.hh 6095 2012-05-08 10:05:56Z boukhele $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */




#ifndef KARABO_IO_HDF5_SCALARFILTER_HH
#define	KARABO_IO_HDF5_SCALARFILTER_HH

#include <karabo/util/Factory.hh>
#include "../ArrayView.hh"
#include "../iodll.hh"



namespace karabo {
  
  namespace io {
        
    namespace hdf5 {

      template<class T> class Scalar;
      
      
      template<class T>
      class ScalarFilter {
      public:

        KARABO_CLASSINFO(ScalarFilter, "ScalarFilter", "1.0")
        KARABO_FACTORY_BASE_CLASS

        virtual ~ScalarFilter() {
        }

        virtual void write(const Scalar<T>& element, const boost::any& any, const size_t len) = 0;
        virtual void read(const Scalar<T>& element, boost::any& any, size_t len) = 0;

      };      
    }
  }
}

KARABO_REGISTER_FACTORY_BASE_HH(karabo::io::hdf5::ScalarFilter<karabo::util::Hash>, TEMPLATE_IO, DECLSPEC_IO)

#endif	/* KARABO_IO_HDF5_SCALARFILTER_HH */

