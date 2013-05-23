/*
 * $Id$
 *
 * File:   Quaternion.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on November 16, 2011, 1:53 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_XIP_QUATERNION_HH
#define	KARABO_XIP_QUATERNION_HH

#include "CpuImage.hh"

namespace karabo {
    namespace xip {

        class Quaternion {

        public:

            Quaternion(const double x, const double y, const double z, const double w) : m_x(x), m_y(y), m_z(z), m_w(w) {
            }

            template <class TPix>
            explicit Quaternion(const CpuImage<TPix> image) {
                if (image.size() < 4) throw IMAGE_DIMENSION_EXCEPTION("Quaternion construction expects at least four elements");
                m_x = image[0];
                m_y = image[1];
                m_z = image[2];
                m_w = image[3];
            }

            template <class TPix>
            Quaternion(const CpuImage<TPix> axis, double angle) {
                const double norm = std::sqrt(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2] + axis[3] * axis[3]);
                axis /= norm;
                double sa = std::sin(0.5 * angle);
                m_w = std::cos(0.5 * angle);
                m_x = axis[0] * sa;
                m_y = axis[1] * sa;
                m_z = axis[2] * sa;
            }

            CpuImage<double> getTransform() const {
                CpuImage<double> m(4, 4);
                double Nq = m_x * m_x + m_y * m_y + m_z * m_z + m_w * m_w;
                double s = (Nq > 0.0) ? (2.0 / Nq) : 0.0;
                double xs = m_x * s, ys = m_y * s, zs = m_z * s;
                double wx = m_w * xs, wy = m_w * ys, wz = m_w * zs;
                double xx = m_x * xs, xy = m_x * ys, xz = m_x * zs;
                double yy = m_y * ys, yz = m_y * zs, zz = m_z * zs;

                m(0, 0) = 1.0 - (yy + zz);
                m(0, 1) = xy - wz;
                m(0, 2) = xz + wy;
                m(1, 0) = xy + wz;
                m(1, 1) = 1.0 - (xx + zz);
                m(1, 2) = yz - wx;
                m(2, 0) = xz - wy;
                m(2, 1) = yz + wx;
                m(2, 2) = (1.0 - (xx + yy));
                m(3, 0) = m(3, 1) = m(3, 2) = m(0, 3) = m(1, 3) = m(2, 3) = 0.0;
                m(3, 3) = 1.0;
                return m;
            }

            static CpuImage<double> getTransform(const double x, const double y, const double z, const double w) {
                CpuImage<double> m(4, 4);
                double Nq = x * x + y * y + z * z + w * w;
                double s = (Nq > 0.0) ? (2.0 / Nq) : 0.0;
                double xs = x * s, ys = y * s, zs = z * s;
                double wx = w * xs, wy = w * ys, wz = w * zs;
                double xx = x * xs, xy = x * ys, xz = x * zs;
                double yy = y * ys, yz = y * zs, zz = z * zs;

                m(0, 0) = 1.0 - (yy + zz);
                m(0, 1) = xy - wz;
                m(0, 2) = xz + wy;
                m(1, 0) = xy + wz;
                m(1, 1) = 1.0 - (xx + zz);
                m(1, 2) = yz - wx;
                m(2, 0) = xz - wy;
                m(2, 1) = yz + wx;
                m(2, 2) = (1.0 - (xx + yy));
                m(3, 0) = m(3, 1) = m(3, 2) = m(0, 3) = m(1, 3) = m(2, 3) = 0.0;
                m(3, 3) = 1.0;
                return m;
            }



        private: // members

            double m_x;
            double m_y;
            double m_z;
            double m_w;



        };


    }
}



#endif	/* KARABO_PACKAGENAME_QUATERNION_HH */

