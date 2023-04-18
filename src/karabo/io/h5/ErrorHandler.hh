/*
 * $Id: ErrorHandler.hh 8843 2013-03-04 17:24:53Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#ifndef KARABO_IO_H5_ERRORHANDLER_HH
#define KARABO_IO_H5_ERRORHANDLER_HH

#include <hdf5/hdf5.h>

#include <string>

#include "karabo/log/Logger.hh"


namespace karabo {

    namespace io {

        namespace h5 {

            // hid_t errId = H5Eget_current_stack();

            /**
             * This function demangles HDF5 errors into a Karabo Exception
             * @param n
             * @param err_desc
             * @param client_data
             * @return
             */
            herr_t karaboH5Errorhandler(unsigned n, const H5E_error2_t* err_desc, void* client_data);

            // Use this macro if you want to throw exception
#define KARABO_CHECK_HDF5_STATUS(status)                                                     \
    if (status < 0) {                                                                        \
        karabo::util::HdfIOException ex("", "", "", 0);                                      \
        H5Ewalk2(H5E_DEFAULT, H5E_WALK_DOWNWARD, karabo::io::h5::karaboH5Errorhandler, &ex); \
        throw ex;                                                                            \
    }

            // Use this macro just for reporting, for example,  in destructors
#define KARABO_CHECK_HDF5_STATUS_NO_THROW(status)                                            \
    if (status < 0) {                                                                        \
        karabo::util::HdfIOException ex("", "", "", 0);                                      \
        H5Ewalk2(H5E_DEFAULT, H5E_WALK_DOWNWARD, karabo::io::h5::karaboH5Errorhandler, &ex); \
        KARABO_LOG_FRAMEWORK_ERROR << "*** HDF IO Exception: " << ex.detailedMsg();          \
    }


        } // namespace h5
    }     // namespace io
} // namespace karabo

#endif
