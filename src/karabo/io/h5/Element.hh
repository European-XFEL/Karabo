/*
 * $Id: Format.hh 5491 2012-03-09 17:27:25Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */





#ifndef KARABO_IO_H5_ELEMENT_HH
#define	KARABO_IO_H5_ELEMENT_HH

#include <karabo/util/Factory.hh>
#include <karabo/util/Hash.hh>
#include <karabo/util/Configurator.hh>
#include <karabo/util/ToLiteral.hh>

#include <karabo/util/Types.hh>
#include <karabo/util/FromTypeInfo.hh>

#include <hdf5/hdf5.h>


#include <iostream>
#include <string>
#include <vector>

namespace karabo {
    namespace io {
        namespace h5 {


            class Element {
            public:
		   KARABO_CLASSINFO(Element, "Element", "1.0");
		   
		   virtual void write(const karabo::util::Hash::Node& node, hsize_t recordId){
		   }

	        	   
	    private:
            };


            template <class T>
            class Scalar : public Element {
            public:

                
		    KARABO_CLASSINFO(Scalar, karabo::util::ToType<karabo::util::ToLiteral>::to( karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid(T)) ) , "1.0");

               
		    Scalar() {           
		    }


		    virtual ~Scalar() {
		    }


		   
		    void write(const karabo::util::Hash::Node& node, hsize_t recordId){
			    std::clog << "Writing " << karabo::util::ToType<karabo::util::ToLiteral>::to( karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid(T)) ) << std::endl;
			    std::clog << "Type : " << node.getType() << std::endl;
			    std::clog << "Key  : " << node.getKey() << std::endl;
			    std::clog << "Value: " << node.getValue<T>() << std::endl;

		      
		    }






            private:



            };


        }
    }
}


#endif

