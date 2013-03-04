/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */




#ifndef KARABO_IO_HDF5_SCALARFILTER_HH
#define	KARABO_IO_HDF5_SCALARFILTER_HH

#include <karabo/util/Factory.hh>
#include <karabo/util/Configurator.hh>



namespace karabo {
  
  namespace io {
        
    namespace h5 {

      template<class T> class Scalar;
      
      
      template<class T>
      class ScalarFilter {
      public:

        KARABO_CLASSINFO(ScalarFilter, "ScalarFilter", "1.0")
        KARABO_CONFIGURATION_BASE_CLASS
        

        virtual ~ScalarFilter() {
        }

        virtual void write(const Scalar<T>& element, const boost::any& any, const size_t len) {}
        virtual void write(const Scalar<T>& element, const karabo::util::Hash::Node& node, const size_t len){}
        virtual void read(const Scalar<T>& element, boost::any& any, size_t len) {}

      };      
    }
  }
}

//KARABO_REGISTER_FACTORY_BASE_HH(karabo::io::hdf5::ScalarFilter<karabo::util::Hash>, TEMPLATE_IO, DECLSPEC_IO)

#endif	/* KARABO_IO_HDF5_SCALARFILTER_HH */

