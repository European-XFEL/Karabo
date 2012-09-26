/*
 * $Id: DataFormat.hh 5491 2012-03-09 17:27:25Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */




#ifndef KARABO_IO_HDF5_DATAFORMAT_HH
#define	KARABO_IO_HDF5_DATAFORMAT_HH

#include <karabo/util/Factory.hh>
#include "RecordFormat.hh"
#include "DataBlock.hh"
#include "../ArrayView.hh"

#include <string>

namespace karabo {
  namespace io {
    namespace hdf5 {

      class DataFormat {
      public:

        KARABO_CLASSINFO(DataFormat, "DataFormat", "1.0")
        KARABO_FACTORY_BASE_CLASS



        DataFormat() {
        }

        virtual ~DataFormat() {
        }

        static void expectedParameters(karabo::util::Schema& expected);
        void configure(const karabo::util::Hash& input);

        static DataFormat::Pointer discoverFromData(const karabo::util::Hash& data);        
        const RecordFormat::Pointer getRecordFormat();
        const karabo::util::Hash& getConfig() const;
        


      private:
        karabo::util::Hash m_config;


        static std::string getClassIdAsString(const boost::any& any);        
        static karabo::io::ArrayDimensions arraySize(const boost::any& any);



      };


    }
  }
}

KARABO_REGISTER_FACTORY_BASE_HH(karabo::io::hdf5::DataFormat, TEMPLATE_IO, DECLSPEC_IO)

#endif	/* KARABO_IO_HDF5_DATAFORMAT_HH */

