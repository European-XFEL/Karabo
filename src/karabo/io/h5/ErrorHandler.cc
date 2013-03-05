/*
 * $Id: ErrorHandler.cc 8843 2013-03-04 17:24:53Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <sstream> 
#include "ErrorHandler.hh"


using namespace karabo::io::h5;
using namespace karabo::util;

namespace karabo {
    namespace io {
        namespace h5 {


            #define MSG_SIZE 64 

            herr_t karaboH5Errorhandler(unsigned n, const H5E_error2_t *err_desc, void* client_data) {
                //FILE *stream = (FILE *) client_data;
                std::ostringstream* oss = static_cast<std::ostringstream*> (client_data);
                char maj[MSG_SIZE];
                char min[MSG_SIZE];
                char cls[MSG_SIZE];


                /* Get descriptions for the major and minor error numbers */
                if (H5Eget_class_name(err_desc->cls_id, cls, MSG_SIZE) < 0)
                    return -1;

                if (H5Eget_msg(err_desc->maj_num, NULL, maj, MSG_SIZE) < 0)
                    return -1;

                if (H5Eget_msg(err_desc->min_num, NULL, min, MSG_SIZE) < 0)
                    return -1;

                *oss << "#";
                oss->width(3);
                oss->fill('0');
                *oss << n << " " << err_desc->file_name << " line: " << err_desc->line;
                *oss << " in " << err_desc->func_name << "(), " << err_desc->desc;
                *oss << ". Major: " << maj << " Minor: " << min << std::endl;
                return 0;

            }


        }
    }
}
