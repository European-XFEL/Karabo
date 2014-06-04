/* 
 * Author: heisenb
 *
 * Created on May 21, 2014, 1:18 PM
 */

#ifndef KARABO_XIP_IMAGEENUMS_HH
#define	KARABO_XIP_IMAGEENUMS_HH

namespace karabo {

    namespace xip {

        namespace Encoding {

            enum EncodingType {

                UNDEFINED = -1,
                GRAY,
                RGB,
                RGBA,
                BGR,
                BGRA,
                CMYK,
                YUV,
                BAYER,
                JPEG,
                PNG,
            };
        }

        typedef Encoding::EncodingType EncodingType;

        namespace ChannelSpace {

            enum ChannelSpaceType {

                UNDEFINED = -1,
                u_8_1, // unsigned, 8 bits per color-channel, 1 byte per pixel
                s_8_1,
                u_10_2,
                s_10_2, // signed, 10 bits per color-channel, 2 bytes per pixel
                u_12_2,
                s_12_2,
                u_12_1p5, // unsigned, 12 bits per color-channel, 1.5 bytes per pixel (i.e. 3 bytes encode 2 pixels, 2 x 12 bits must be read)
                s_12_1p5,
                u_16_2,
                s_16_2,
                f_16_2,
                u_32_4,
                s_32_4,
                f_32_4,
                u_64_8,
                s_64_8,
                f_64_8,
            };
        }

        typedef ChannelSpace::ChannelSpaceType ChannelSpaceType;

        namespace Endianness {

            enum EndiannessType {

                UNDEFINED = -1,
                LSB,
                MSB
            };
        }

        typedef Endianness::EndiannessType EndiannessType;
    }
}

#endif
