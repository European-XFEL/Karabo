/*
 * $Id: File.hh 5395 2012-03-07 16:10:07Z wegerk $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */




#ifndef EXFEL_IO_FILE_HH
#define	EXFEL_IO_FILE_HH

#include <karabo/util/Factory.hh>
#include "RecordFormat.hh"


namespace exfel {
  namespace io {


    class File {
    public:
      EXFEL_CLASSINFO(File, "File", "1.0")
      EXFEL_FACTORY_BASE_CLASS

    private:

    protected:


    public:

      virtual ~File(){
      }
      
      //virtual void openNew(exfel::io::RecordFormat::Pointer dataSetDescription) = 0;
      virtual void openReadOnly(const exfel::util::Hash& dataSetDescription) = 0;
      virtual void read(exfel::util::Hash& data) = 0;
      virtual void write(const exfel::util::Hash& data) = 0; 
      virtual void close() = 0;


    };
  }
}

EXFEL_REGISTER_FACTORY_BASE_HH(exfel::io::File, TEMPLATE_IO, DECLSPEC_IO)

#endif	/* EXFEL_IO_FILE_HH */

