/*
 * $Id$
 *
 * Author: <andrea.parenti@xfel.eu>
 *
 * Copyright (c) 2010-2012 European XFEL GmbH Hamburg. All rights reserved.
 */

#include <vector>

#include "Base64.hh"

namespace karabo {
    namespace util {


        // Base64 Index Table
        const std::string b64_char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        std::string base64Encode(const unsigned char* bytes_to_encode, const size_t len) {
            unsigned char i0, i1, i2; // Bytes from input string
            unsigned char o0, o1, o2, o3; // Bytes to output string
            unsigned short pad; // Number of padding chars
            std::string out = ""; // Output string

            // Empty input string => Empty output string
            if (len == 0) return "";

            // Loop over input string
            for (size_t i = 0; i < len; i += 3) {

                // Find out how many padding chars are needed
                if (i + 1 == len)
                    pad = 2;
                else if (i + 2 == len)
                    pad = 1;
                else
                    pad = 0;

                // Read 3 bytes from input string (and pad if necessary)
                i0 = bytes_to_encode[i];
                if (pad < 2)
                    i1 = bytes_to_encode[i + 1];
                else
                    i1 = 0;
                if (pad < 1)
                    i2 = bytes_to_encode[i + 2];
                else
                    i2 = 0;

                // Encode input bytes
                o0 = ((i0) >> 2);
                o1 = ((i0 & 0x03) << 4) + ((i1 & 0xF0) >> 4);
                o2 = ((i1 & 0x0F) << 2) + ((i2 & 0xC0) >> 6);
                o3 = ((i2 & 0x3F));

                // Write encoded bytes to output (and pad if necessary)
                out += b64_char[o0];
                out += b64_char[o1];
                if (pad < 2)
                    out += b64_char[o2];
                else
                    out += "=";
                if (pad < 1)
                    out += b64_char[o3];
                else
                    out += "=";
            }

            return out;
        }


        void base64Decode(const std::string& in, std::vector<unsigned char>& out) {
            unsigned char i0, i1, i2, i3; // Bytes from input string
            unsigned char o0, o1, o2; // Bytes to output string
            unsigned short pad; // Number of padding chars
            size_t t0, t1, t2, t3;
            size_t len = in.size();
            
            // Empty input string => Empty output string
            if (len == 0) return;
            else out.reserve(len);

            // Loop over input string
            for (size_t i = 0; i < len; i += 4) {

                // Find out how many padding chars are there
                if (i + 1 == len || in[i + 1] == '=')
                    pad = 3;
                else if (i + 2 == len || in[i + 2] == '=')
                    pad = 2;
                else if (i + 3 == len || in[i + 3] == '=')
                    pad = 1;
                else
                    pad = 0;

                // Read 4 bytes from input string
                // and decode them
                i0 = t0 = b64_char.find(in[i]);
                if (pad < 3)
                    i1 = t1 = b64_char.find(in[i + 1]);
                else
                    i1 = t1 = 0;
                if (pad < 2)
                    i2 = t2 = b64_char.find(in[i + 2]);
                else
                    i2 = t2 = 0;
                if (pad < 1)
                    i3 = t3 = b64_char.find(in[i + 3]);
                else
                    i3 = t3 = 0;

                // Check that all input bytes are base64
                if (t0 == std::string::npos || t1 == std::string::npos ||
                        t2 == std::string::npos || t3 == std::string::npos) {
                    throw (std::string)"base64_decode: Non-base64 found in the string to be decoded";
                }

                // Decode input bytes
                o0 = ((i0) << 2) + (i1 >> 4);
                o1 = ((i1 & 0x0F) << 4) + (i2 >> 2);
                o2 = ((i2 & 0x0F) << 6) + (i3);

                // Write decoded bytes to output
                 out.push_back(o0);
                if (pad < 2)
                     out.push_back(o1);
                if (pad < 1)
                     out.push_back(o2);
            }
        }
    }
}
