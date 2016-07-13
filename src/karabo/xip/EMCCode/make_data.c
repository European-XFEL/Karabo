/*

Generates simulated detector data given a cubic intensity file and a file specifying the positions
of the detector pixels within the intensity cube. There is zero background. Output is written in
sparse format.

Written by V. Elser; last modified November 2008.


compile:
gcc -O3 make_data.c -lm -o make_data

usage:
make_data num mean_count (num = number of diffraction patterns, mean_count = mean number of photons per diffraction pattern)

needs:
detector.dat, intensity.dat

makes:
photons.dat

 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

double rot[3][3];
double quat[4];
double (*pix)[3];
int ones, multi, *place_ones, *place_multi, *count;
double ***intens;
int q_max, size, m_pix, m_stop;
double mean_count;

int setup();
void free_mem();
void make_rot();
void rand_quat();
int poisson(double);

int main(int argc, char* argv[]) {
    int num, d, t;
    int i, j, k;
    double rot_pix[3];
    int x, y, z;
    double tx, ty, tz, fx, fy, fz, cx, cy, cz;
    int photons;
    double intens_val, intens_ave = 0.;
    int m_ave = 1000;
    FILE *fp;

    if (argc == 3) {
        num = atoi(argv[1]);
        mean_count = atof(argv[2]);
    } else {
        printf("expected two arguments: num, mean_count\n");
        return 0;
    }

    if (!setup())
        return 0;

    srand(time(0));

    fp = fopen("photons.dat", "w");

    for (d = 0; d < num + m_ave; ++d) {
        rand_quat();
        make_rot();

        ones = 0;
        multi = 0;

        if (d == m_ave) {
            intens_ave /= m_ave;

            fprintf(fp, "%d  %f\n\n", num, intens_ave);

            for (i = 0; i < size; ++i)
                for (j = 0; j < size; ++j)
                    for (k = 0; k < size; ++k)
                        intens[i][j][k] *= mean_count / intens_ave;
        }

        for (t = 0; t < m_pix; ++t) {
            for (i = 0; i < 3; ++i) {
                rot_pix[i] = 0.;
                for (j = 0; j < 3; ++j)
                    rot_pix[i] += rot[i][j] * pix[t][j];
            }

            tx = rot_pix[0] + q_max;
            ty = rot_pix[1] + q_max;
            tz = rot_pix[2] + q_max;

            x = tx;
            y = ty;
            z = tz;

            fx = tx - x;
            fy = ty - y;
            fz = tz - z;

            cx = 1. - fx;
            cy = 1. - fy;
            cz = 1. - fz;

            intens_val = cx * (cy * (cz * intens[x][y][z] + fz * intens[x][y][z + 1]) + fy * (cz * intens[x][y + 1][z] + fz * intens[x][y + 1][z + 1])) + fx * (cy * (cz * intens[x + 1][y][z] + fz * intens[x + 1][y][z + 1]) + fy * (cz * intens[x + 1][y + 1][z] + fz * intens[x + 1][y + 1][z + 1]));

            if (d < m_ave) {
                intens_ave += intens_val;
                continue;
            }

            photons = poisson(intens_val);

            if (!photons)
                continue;

            if (photons == 1) {
                place_ones[ones] = t;
                ++ones;
            } else {
                place_multi[multi] = t;
                count[multi] = photons;
                ++multi;
            }
        }

        if (d < m_ave)
            continue;

        fprintf(fp, "%d\n", ones);
        for (t = 0; t < ones; ++t)
            fprintf(fp, "%d ", place_ones[t]);
        fprintf(fp, "\n");

        fprintf(fp, "%d\n", multi);
        for (t = 0; t < multi; ++t)
            fprintf(fp, "%d %d  ", place_multi[t], count[t]);
        fprintf(fp, "\n\n");
    }

    fclose(fp);

    free_mem();

    return 0;
}

int setup() {
    FILE *fp;
    int i, j, k, t;

    fp = fopen("detector.dat", "r");
    if (!fp) {
        printf("cannot open detector.dat\n");
        return 0;
    }

    fscanf(fp, "%d %d %d", &q_max, &m_pix, &m_stop);

    size = 2 * q_max + 1;

    place_ones = malloc(m_pix * sizeof (int*));
    place_multi = malloc(m_pix * sizeof (int*));
    count = malloc(m_pix * sizeof (int*));

    pix = malloc(m_pix * sizeof (*pix));

    for (t = 0; t < m_pix; ++t)
        for (i = 0; i < 3; ++i)
            fscanf(fp, "%lf", &pix[t][i]);

    fclose(fp);

    fp = fopen("intensity.dat", "r");
    if (!fp) {
        printf("cannot open intensity.dat\n");
        return 0;
    }

    intens = malloc(size * sizeof (double**));
    for (i = 0; i < size; ++i) {
        intens[i] = malloc(size * sizeof (double*));

        for (j = 0; j < size; ++j) {
            intens[i][j] = malloc(size * sizeof (double));

            for (k = 0; k < size; ++k)
                fscanf(fp, "%lf", &intens[i][j][k]);
        }
    }

    fclose(fp);

    return 1;
}

void free_mem() {
    int i, j;

    free(place_ones);
    free(place_multi);
    free(count);

    free(pix);

    for (i = 0; i < size; ++i) {
        for (j = 0; j < size; ++j)
            free(intens[i][j]);

        free(intens[i]);
    }

    free(intens);
}

void make_rot() {
    double q0, q1, q2, q3, q01, q02, q03, q11, q12, q13, q22, q23, q33;

    q0 = quat[0];
    q1 = quat[1];
    q2 = quat[2];
    q3 = quat[3];

    q01 = q0*q1;
    q02 = q0*q2;
    q03 = q0*q3;
    q11 = q1*q1;
    q12 = q1*q2;
    q13 = q1*q3;
    q22 = q2*q2;
    q23 = q2*q3;
    q33 = q3*q3;

    rot[0][0] = 1. - 2. * (q22 + q33);
    rot[0][1] = 2. * (q12 + q03);
    rot[0][2] = 2. * (q13 - q02);
    rot[1][0] = 2. * (q12 - q03);
    rot[1][1] = 1. - 2. * (q11 + q33);
    rot[1][2] = 2. * (q01 + q23);
    rot[2][0] = 2. * (q02 + q13);
    rot[2][1] = 2. * (q23 - q01);
    rot[2][2] = 1. - 2. * (q11 + q22);
}

void rand_quat() {
    int i;
    double qq;

    do {
        qq = 0.;
        for (i = 0; i < 4; ++i) {
            quat[i] = ((double) rand()) / RAND_MAX - .5;
            qq += quat[i] * quat[i];
        }
    } while (qq > .25);

    qq = sqrt(qq);
    for (i = 0; i < 4; ++i)
        quat[i] /= qq;
}

int poisson(double m) {
    int i = 0;
    double p, q, r;

    r = exp(-m);
    p = r;
    q = ((double) rand()) / RAND_MAX;

    while (p < q) {
        ++i;
        r *= m / i;
        p += r;
    }

    return i;
}
