/*
 * $Id: ErrorHandler.hh 8843 2013-03-04 17:24:53Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_IO_H5_ERRORHANDLER_HH
#define	KARABO_IO_H5_ERRORHANDLER_HH

#include <string>
#include <hdf5/hdf5.h>



namespace karabo {

    namespace io {

        namespace h5 {

            //hid_t errId = H5Eget_current_stack();

            herr_t karaboH5Errorhandler(unsigned n, const H5E_error2_t *err_desc, void* client_data);

#define KARABO_CHECK_HDF5_STATUS(status)\
                if(status < 0){\
                karabo::util::HdfIOException ex("","","",0);\
                H5Ewalk2(H5E_DEFAULT, H5E_WALK_DOWNWARD, karabo::io::h5::karaboH5Errorhandler, &ex);\
                throw ex;\
            }



        }
    }
}

#endif	
