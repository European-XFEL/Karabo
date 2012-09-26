/*
 * $Id: DataTypes.hh 6095 2012-05-08 10:05:56Z boukhele $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */




#ifndef KARABO_IO_HDF5_DATATYPES_HH
#define	KARABO_IO_HDF5_DATATYPES_HH

#include <karabo/util/Factory.hh>
#include "../iodll.hh"


namespace karabo {
  
  namespace io {
    
    class ArrayDimensions;
    
    namespace hdf5 {
  
      /*
       * Used for auto discovering of the data format
       */
      class DataTypes {
      public:

        KARABO_CLASSINFO(DataTypes, "DataTypes", "1.0")
        KARABO_FACTORY_BASE_CLASS

        virtual ~DataTypes() {
        }

        virtual karabo::io::ArrayDimensions getDims(const boost::any& any) = 0;  
        virtual std::string getElementClassId() = 0;
      };
          
    }
  }
}

KARABO_REGISTER_FACTORY_BASE_HH(karabo::io::hdf5::DataTypes, TEMPLATE_IO, DECLSPEC_IO)

#endif	

