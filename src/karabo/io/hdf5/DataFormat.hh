/*
 * $Id: DataFormat.hh 5491 2012-03-09 17:27:25Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */




#ifndef EXFEL_IO_HDF5_DATAFORMAT_HH
#define	EXFEL_IO_HDF5_DATAFORMAT_HH

#include <karabo/util/Factory.hh>
#include "RecordFormat.hh"
#include "DataBlock.hh"
#include "../ArrayView.hh"

#include <string>

namespace exfel {
  namespace io {
    namespace hdf5 {

      class DataFormat {
      public:

        EXFEL_CLASSINFO(DataFormat, "DataFormat", "1.0")
        EXFEL_FACTORY_BASE_CLASS



        DataFormat() {
        }

        virtual ~DataFormat() {
        }

        static void expectedParameters(exfel::util::Schema& expected);
        void configure(const exfel::util::Hash& input);

        static DataFormat::Pointer discoverFromData(const exfel::util::Hash& data);        
        const RecordFormat::Pointer getRecordFormat();
        const exfel::util::Hash& getConfig() const;
        


      private:
        exfel::util::Hash m_config;


        static std::string getClassIdAsString(const boost::any& any);        
        static exfel::io::ArrayDimensions arraySize(const boost::any& any);



      };


    }
  }
}

EXFEL_REGISTER_FACTORY_BASE_HH(exfel::io::hdf5::DataFormat, TEMPLATE_IO, DECLSPEC_IO)

#endif	/* EXFEL_IO_HDF5_DATAFORMAT_HH */

