/* 
 * File:   ImageData.cc
 * Author: 
 * 
 * Created on April 20, 2015, 19:35 PM
 */

#include "ImageData.hh"


namespace karabo {
    namespace xms {

        // Specialize AND Register
        KARABO_REGISTER_FOR_CONFIGURATION(Data, ImageData<bool>);
        KARABO_REGISTER_FOR_CONFIGURATION(Data, ImageData<signed char>);
        KARABO_REGISTER_FOR_CONFIGURATION(Data, ImageData<signed short>);
        KARABO_REGISTER_FOR_CONFIGURATION(Data, ImageData<int>);
        KARABO_REGISTER_FOR_CONFIGURATION(Data, ImageData<long long>);
        KARABO_REGISTER_FOR_CONFIGURATION(Data, ImageData<unsigned char>);
        KARABO_REGISTER_FOR_CONFIGURATION(Data, ImageData<unsigned short>);
        KARABO_REGISTER_FOR_CONFIGURATION(Data, ImageData<unsigned int>);
        KARABO_REGISTER_FOR_CONFIGURATION(Data, ImageData<unsigned long long>);
        KARABO_REGISTER_FOR_CONFIGURATION(Data, ImageData<float>);
        KARABO_REGISTER_FOR_CONFIGURATION(Data, ImageData<double>);

    }
}
