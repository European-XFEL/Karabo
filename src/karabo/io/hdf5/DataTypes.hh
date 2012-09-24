/*
 * $Id: DataTypes.hh 6095 2012-05-08 10:05:56Z boukhele $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */




#ifndef EXFEL_IO_HDF5_DATATYPES_HH
#define	EXFEL_IO_HDF5_DATATYPES_HH

#include <karabo/util/Factory.hh>
#include "../iodll.hh"


namespace exfel {
  
  namespace io {
    
    class ArrayDimensions;
    
    namespace hdf5 {
  
      /*
       * Used for auto discovering of the data format
       */
      class DataTypes {
      public:

        EXFEL_CLASSINFO(DataTypes, "DataTypes", "1.0")
        EXFEL_FACTORY_BASE_CLASS

        virtual ~DataTypes() {
        }

        virtual exfel::io::ArrayDimensions getDims(const boost::any& any) = 0;  
        virtual std::string getElementClassId() = 0;
      };
          
    }
  }
}

EXFEL_REGISTER_FACTORY_BASE_HH(exfel::io::hdf5::DataTypes, TEMPLATE_IO, DECLSPEC_IO)

#endif	

