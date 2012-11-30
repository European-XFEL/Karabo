/*
 * $Id: File.hh 5395 2012-03-07 16:10:07Z wegerk $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */




#ifndef KARABO_IO_FILE_HH
#define	KARABO_IO_FILE_HH

#include <karabo/util/Factory.hh>
#include "hdf5/RecordFormat.hh"


namespace karabo {
  namespace io {


    class File {
    public:
      KARABO_CLASSINFO(File, "File", "1.0")
      KARABO_FACTORY_BASE_CLASS

    private:

    protected:


    public:

      virtual ~File(){
      }
      
      //virtual void openNew(karabo::io::RecordFormat::Pointer dataSetDescription) = 0;
      virtual void openReadOnly(const karabo::util::Hash& dataSetDescription) = 0;
      virtual void read(karabo::util::Hash& data) = 0;
      virtual void write(const karabo::util::Hash& data) = 0; 
      virtual void close() = 0;


    };
  }
}

KARABO_REGISTER_FACTORY_BASE_HH(karabo::io::File, TEMPLATE_IO, DECLSPEC_IO)

#endif	/* KARABO_IO_FILE_HH */

