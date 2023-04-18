/*
 * $Id: ErrorHandler.cc 8843 2013-03-04 17:24:53Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */

#include "ErrorHandler.hh"

#include <karabo/util/Exception.hh>
#include <sstream>


using namespace karabo::io::h5;
using namespace karabo::util;
using namespace std;

namespace karabo {
    namespace io {
        namespace h5 {


            herr_t karaboH5Errorhandler(unsigned n, const H5E_error2_t* err_desc, void* client_data) {
                karabo::util::HdfIOException* ex = static_cast<karabo::util::HdfIOException*>(client_data);

                ssize_t lenClassId = 0, lenMajor = 0, lenMinor = 0;
                if ((lenClassId = H5Eget_class_name(err_desc->cls_id, NULL, 0)) < 0) return -1;

                if ((lenMajor = H5Eget_msg(err_desc->maj_num, NULL, NULL, 0)) < 0) return -1;

                if ((lenMinor = H5Eget_msg(err_desc->min_num, NULL, NULL, 0)) < 0) return -1;

                /* Get descriptions for the major and minor error numbers */
                vector<char> cls(lenClassId + 1, '\0');
                if (H5Eget_class_name(err_desc->cls_id, &cls[0], lenClassId + 1) < 0) return -1;

                vector<char> maj(lenMajor + 1, '\0');
                if (H5Eget_msg(err_desc->maj_num, NULL, &maj[0], lenMajor + 1) < 0) return -1;

                vector<char> min(lenMinor + 1, '\0');
                if (H5Eget_msg(err_desc->min_num, NULL, &min[0], lenMinor + 1) < 0) return -1;

                ostringstream oss;
                oss << "#";
                oss.width(3);
                oss.fill('0');
                oss << n << " " << err_desc->desc;
                oss << ". Major: " << &maj[0] << " Minor: " << &min[0];

                ex->set(oss.str(), err_desc->file_name, err_desc->func_name, err_desc->line);
                return 0;
            }

            //            herr_t karaboH5Errorhandler(unsigned n, const H5E_error2_t *err_desc, void* client_data) {
            //                std::ostringstream* oss = static_cast<std::ostringstream*> (client_data);
            //
            //                ssize_t lenClassId = 0, lenMajor=0, lenMinor=0;
            //                if ((lenClassId = H5Eget_class_name(err_desc->cls_id, NULL, 0)) < 0)
            //                    return -1;
            //                clog << "classId len: " << lenClassId << endl;
            //
            //                if ((lenMajor = H5Eget_msg(err_desc->maj_num, NULL, NULL, 0)) < 0)
            //                    return -1;
            //                clog << "major len: " << lenMajor << endl;
            //
            //                if ((lenMinor = H5Eget_msg(err_desc->min_num, NULL, NULL, 0)) < 0)
            //                    return -1;
            //                clog << "minor len: " << lenMinor << endl;
            //
            //                /* Get descriptions for the major and minor error numbers */
            ////                if (H5Eget_class_name(err_desc->cls_id, cls, lenClassId) < 0)
            ////                    return -1;
            //
            //                vector<char> maj(lenMajor+1,'\0');
            //                if (H5Eget_msg(err_desc->maj_num, NULL, &maj[0], lenMajor+1) < 0)
            //                    return -1;
            //
            //                vector<char> min(lenMinor+1,'\0');
            //                if (H5Eget_msg(err_desc->min_num, NULL, &min[0], lenMinor+1) < 0)
            //                    return -1;
            //
            //                *oss << "#";
            //                oss->width(3);
            //                oss->fill('0');
            //                *oss << n << " " << err_desc->file_name << " line: " << err_desc->line;
            //                *oss << " in " << err_desc->func_name << "(), " << err_desc->desc;
            //                *oss << ". Major: " << &maj[0] << " Minor: " << &min[0] << std::endl;
            //                return 0;
            //
            //            }


        } // namespace h5
    }     // namespace io
} // namespace karabo
