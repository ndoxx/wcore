#ifndef NOISE_POLICY_H
#define NOISE_POLICY_H

#include <functional>
#include <vector>
#include <cassert>
#include <algorithm>
#include <random>
#include <iterator>
#include <array>

#include "math3d.h"

/*
 * A speed-improved simplex noise algorithm for 2D, 3D and 4D in C++.
 *
 * >Forked from: http://www.itn.liu.se/~stegu/simplexnoise/SimplexNoise.java
 * >by Boris Garcia (Translation from Java to C++, minor changes, adaptation to
 * >a generic templated noise generator for the WEngine game engine.
 *
 * Based on example code by Stefan Gustavson (stegu@itn.liu.se).
 * Optimisations by Peter Eastman (peastman@drizzle.stanford.edu).
 * Better rank ordering method by Stefan Gustavson in 2012.
 *
 * This could be speeded up even further, but it's useful as it is.
 *
 * Version 2015-07-01
 *
 * This code was placed in the public domain by its original author,
 * Stefan Gustavson. You may use it as you see fit, but
 * attribution is appreciated.
 *
 */

// Skewing and unskewing factors
#define F2 0.5*(sqrt(3.0)-1.0)
#define G2 (3.0-sqrt(3.0))/6.0
#define F3 1.0/3.0
#define G3 1.0/6.0
#define F4 (sqrt(5.0)-1.0)/4.0
#define G4 (5.0-sqrt(5.0))/20.0

static const short p[256] = {151,160,137,91,90,15,
131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180};


// The gradients are the midpoints of the vertices of a cube.
static const int grad3[12][3] =
{
    {1,1,0}, {-1,1,0}, {1,-1,0}, {-1,-1,0},
    {1,0,1}, {-1,0,1}, {1,0,-1}, {-1,0,-1},
    {0,1,1}, {0,-1,1}, {0,1,-1}, {0,-1,-1}
};

// The gradients are the midpoints of the vertices of a hypercube.
static const int grad4[32][4]= {
    {0,1,1,1},  {0,1,1,-1},  {0,1,-1,1},  {0,1,-1,-1},
    {0,-1,1,1}, {0,-1,1,-1}, {0,-1,-1,1}, {0,-1,-1,-1},
    {1,0,1,1},  {1,0,1,-1},  {1,0,-1,1},  {1,0,-1,-1},
    {-1,0,1,1}, {-1,0,1,-1}, {-1,0,-1,1}, {-1,0,-1,-1},
    {1,1,0,1},  {1,1,0,-1},  {1,-1,0,1},  {1,-1,0,-1},
    {-1,1,0,1}, {-1,1,0,-1}, {-1,-1,0,1}, {-1,-1,0,-1},
    {1,1,1,0},  {1,1,-1,0},  {1,-1,1,0},  {1,-1,-1,0},
    {-1,1,1,0}, {-1,1,-1,0}, {-1,-1,1,0}, {-1,-1,-1,0}
};

// Simplex noise generator for any Floating point type
template <typename Float=float,
          class = typename std::enable_if<std::is_floating_point<Float>::value>::type>
class SimplexNoise
{
private:
    // This method is [SUPPOSED TO BE] a *lot* faster than using (int)floor(x)
    static int fastfloor(Float x)
    {
        int xi = (int)x;
        return x<xi ? xi-1 : xi;
    }

    // Dot products
    static Float dot(const int* g, Float x, Float y)
    {
        return g[0]*x + g[1]*y;
    }
    static Float dot(const int* g, Float x, Float y, Float z)
    {
        return g[0]*x + g[1]*y + g[2]*z;
    }
    static Float dot(const int* g, Float x, Float y, Float z, Float w)
    {
        return g[0]*x + g[1]*y + g[2]*z + g[3]*w;
    }


    std::array<short,255> randp_;
    short* perm_;
    short* perm_mod_12_;

public:
    SimplexNoise()
    {
        // To remove the need for index wrapping, double the permutation table length
        perm_ = new short[512];
        perm_mod_12_ = new short[512];
    }

    ~SimplexNoise()
    {
        delete[] perm_;
        delete[] perm_mod_12_;
    }

    void init(std::mt19937& rng)
    {
        // Random permutation table
        for(short ii=0; ii<=255; ++ii)
            randp_[ii] = ii;
        std::shuffle(randp_.begin(), randp_.end(), rng);

        // Initialize permutation table
        for(int ii=0; ii<512; ++ii)
        {
            perm_[ii] = randp_[ii & 255];
            perm_mod_12_[ii] = (short)(perm_[ii] % 12);
        }
    }

    Float operator()(Float xin, Float yin)
    {
        Float n0, n1, n2; // Noise contributions from the three corners

        // Skew the input space to determine which simplex cell we're in
        Float s = (xin+yin)*F2; // Hairy factor for 2D
        int i = fastfloor(xin+s);
        int j = fastfloor(yin+s);
        Float t = (i+j)*G2;
        Float X0 = i-t; // Unskew the cell origin back to (x,y) space
        Float Y0 = j-t;
        Float x0 = xin-X0; // The x,y distances from the cell origin
        Float y0 = yin-Y0;

        // For the 2D case, the simplex shape is an equilateral triangle.
        // Determine which simplex we are in.
        int i1, j1; // Offsets for second (middle) corner of simplex in (i,j) coords
        if(x0>y0) {i1=1; j1=0;} // lower triangle, XY order: (0,0)->(1,0)->(1,1)
        else {i1=0; j1=1;}      // upper triangle, YX order: (0,0)->(0,1)->(1,1)
        // A step of (1,0) in (i,j) means a step of (1-c,-c) in (x,y), and
        // a step of (0,1) in (i,j) means a step of (-c,1-c) in (x,y), where
        // c = (3-sqrt(3))/6

        Float x1 = x0 - i1 + G2; // Offsets for middle corner in (x,y) unskewed coords
        Float y1 = y0 - j1 + G2;
        Float x2 = x0 - 1.0 + 2.0 * G2; // Offsets for last corner in (x,y) unskewed coords
        Float y2 = y0 - 1.0 + 2.0 * G2;

        // Work out the hashed gradient indices of the three simplex corners
        int ii = i & 255;
        int jj = j & 255;
        int gi0 = perm_mod_12_[ii+perm_[jj]];
        int gi1 = perm_mod_12_[ii+i1+perm_[jj+j1]];
        int gi2 = perm_mod_12_[ii+1+perm_[jj+1]];

        // Calculate the contribution from the three corners
        Float t0 = 0.5 - x0*x0-y0*y0;
        if(t0<0)
            n0 = 0.0;
        else
        {
            t0 *= t0;
            n0 = t0 * t0 * dot(grad3[gi0], x0, y0);  // (x,y) of grad3 used for 2D gradient
        }
        Float t1 = 0.5 - x1*x1-y1*y1;
        if(t1<0)
            n1 = 0.0;
        else
        {
            t1 *= t1;
            n1 = t1 * t1 * dot(grad3[gi1], x1, y1);
        }
        Float t2 = 0.5 - x2*x2-y2*y2;
        if(t2<0)
            n2 = 0.0;
        else
        {
            t2 *= t2;
            n2 = t2 * t2 * dot(grad3[gi2], x2, y2);
        }

        // Add contributions from each corner to get the final noise value.
        // The result is scaled to return values in the interval [-1,1].
        return 70.0 * (n0 + n1 + n2);
    }

    // 3D raw Simplex noise
    Float operator()(Float xin, Float yin, Float zin)
    {
        Float n0, n1, n2, n3; // Noise contributions from the four corners

        // Skew the input space to determine which simplex cell we're in
        Float s = (xin+yin+zin)*F3; // Very nice and simple skew factor for 3D
        int i = fastfloor(xin+s);
        int j = fastfloor(yin+s);
        int k = fastfloor(zin+s);

        Float t = (i+j+k)*G3;
        Float X0 = i-t; // Unskew the cell origin back to (x,y,z) space
        Float Y0 = j-t;
        Float Z0 = k-t;
        Float x0 = xin-X0; // The x,y,z distances from the cell origin
        Float y0 = yin-Y0;
        Float z0 = zin-Z0;

        // For the 3D case, the simplex shape is a slightly irregular tetrahedron.
        // Determine which simplex we are in.
        int i1, j1, k1; // Offsets for second corner of simplex in (i,j,k) coords
        int i2, j2, k2; // Offsets for third corner of simplex in (i,j,k) coords

        if(x0>=y0)
        {
            if(y0>=z0) { i1=1; j1=0; k1=0; i2=1; j2=1; k2=0; } // X Y Z order
            else if(x0>=z0) { i1=1; j1=0; k1=0; i2=1; j2=0; k2=1; } // X Z Y order
            else { i1=0; j1=0; k1=1; i2=1; j2=0; k2=1; } // Z X Y order
        }
        else
        { // x0<y0
            if(y0<z0) { i1=0; j1=0; k1=1; i2=0; j2=1; k2=1; } // Z Y X order
            else if(x0<z0) { i1=0; j1=1; k1=0; i2=0; j2=1; k2=1; } // Y Z X order
            else { i1=0; j1=1; k1=0; i2=1; j2=1; k2=0; } // Y X Z order
        }

        // A step of (1,0,0) in (i,j,k) means a step of (1-c,-c,-c) in (x,y,z),
        // a step of (0,1,0) in (i,j,k) means a step of (-c,1-c,-c) in (x,y,z), and
        // a step of (0,0,1) in (i,j,k) means a step of (-c,-c,1-c) in (x,y,z), where
        // c = 1/6.
        Float x1 = x0 - i1 + G3; // Offsets for second corner in (x,y,z) coords
        Float y1 = y0 - j1 + G3;
        Float z1 = z0 - k1 + G3;
        Float x2 = x0 - i2 + 2.0*G3; // Offsets for third corner in (x,y,z) coords
        Float y2 = y0 - j2 + 2.0*G3;
        Float z2 = z0 - k2 + 2.0*G3;
        Float x3 = x0 - 1.0 + 3.0*G3; // Offsets for last corner in (x,y,z) coords
        Float y3 = y0 - 1.0 + 3.0*G3;
        Float z3 = z0 - 1.0 + 3.0*G3;

        // Work out the hashed gradient indices of the four simplex corners
        int ii = i & 255;
        int jj = j & 255;
        int kk = k & 255;
        int gi0 = perm_[ii+perm_[jj+perm_[kk]]] % 12;
        int gi1 = perm_[ii+i1+perm_[jj+j1+perm_[kk+k1]]] % 12;
        int gi2 = perm_[ii+i2+perm_[jj+j2+perm_[kk+k2]]] % 12;
        int gi3 = perm_[ii+1+perm_[jj+1+perm_[kk+1]]] % 12;

        // Calculate the contribution from the four corners
        // >Replaced 0.6 term by 0.5 for continuity at simplex boundaries
        Float t0 = 0.5 - x0*x0 - y0*y0 - z0*z0;
        if(t0<0) n0 = 0.0;
        else {
        t0 *= t0;
        n0 = t0 * t0 * dot(grad3[gi0], x0, y0, z0);
        }

        Float t1 = 0.5 - x1*x1 - y1*y1 - z1*z1;
        if(t1<0) n1 = 0.0;
        else {
        t1 *= t1;
        n1 = t1 * t1 * dot(grad3[gi1], x1, y1, z1);
        }

        Float t2 = 0.5 - x2*x2 - y2*y2 - z2*z2;
        if(t2<0) n2 = 0.0;
        else {
        t2 *= t2;
        n2 = t2 * t2 * dot(grad3[gi2], x2, y2, z2);
        }

        Float t3 = 0.5 - x3*x3 - y3*y3 - z3*z3;
        if(t3<0) n3 = 0.0;
        else {
        t3 *= t3;
        n3 = t3 * t3 * dot(grad3[gi3], x3, y3, z3);
        }

        // Add contributions from each corner to get the final noise value.
        // The result is scaled to stay just inside [-1,1]
        return 32.0*(n0 + n1 + n2 + n3);
    }


    // 4D raw Simplex noise
    Float operator()(Float xin, Float yin, Float zin, Float win)
    {
        Float n0, n1, n2, n3, n4; // Noise contributions from the five corners
        // Skew the (x,y,z,w) space to determine which cell of 24 simplices we're in
        Float s = (xin+yin+zin+win) * F4; // Factor for 4D skewing
        int i = fastfloor(xin + s);
        int j = fastfloor(yin + s);
        int k = fastfloor(zin + s);
        int l = fastfloor(win + s);
        Float t = (i + j + k + l) * G4; // Factor for 4D unskewing
        Float X0 = i - t; // Unskew the cell origin back to (x,y,z,w) space
        Float Y0 = j - t;
        Float Z0 = k - t;
        Float W0 = l - t;
        Float x0 = xin - X0;  // The x,y,z,w distances from the cell origin
        Float y0 = yin - Y0;
        Float z0 = zin - Z0;
        Float w0 = win - W0;
        // For the 4D case, the simplex is a 4D shape I won't even try to describe.
        // To find out which of the 24 possible simplices we're in, we need to
        // determine the magnitude ordering of x0, y0, z0 and w0.
        // Six pair-wise comparisons are performed between each possible pair
        // of the four coordinates, and the results are used to rank the numbers.
        int rankx = 0;
        int ranky = 0;
        int rankz = 0;
        int rankw = 0;
        if(x0 > y0) rankx++; else ranky++;
        if(x0 > z0) rankx++; else rankz++;
        if(x0 > w0) rankx++; else rankw++;
        if(y0 > z0) ranky++; else rankz++;
        if(y0 > w0) ranky++; else rankw++;
        if(z0 > w0) rankz++; else rankw++;
        int i1, j1, k1, l1; // The integer offsets for the second simplex corner
        int i2, j2, k2, l2; // The integer offsets for the third simplex corner
        int i3, j3, k3, l3; // The integer offsets for the fourth simplex corner
        // simplex[c] is a 4-vector with the numbers 0, 1, 2 and 3 in some order.
        // Many values of c will never occur, since e.g. x>y>z>w makes x<z, y<w and x<w
        // impossible. Only the 24 indices which have non-zero entries make any sense.
        // We use a thresholding to set the coordinates in turn from the largest magnitude.
        // Rank 3 denotes the largest coordinate.
        i1 = rankx >= 3 ? 1 : 0;
        j1 = ranky >= 3 ? 1 : 0;
        k1 = rankz >= 3 ? 1 : 0;
        l1 = rankw >= 3 ? 1 : 0;
        // Rank 2 denotes the second largest coordinate.
        i2 = rankx >= 2 ? 1 : 0;
        j2 = ranky >= 2 ? 1 : 0;
        k2 = rankz >= 2 ? 1 : 0;
        l2 = rankw >= 2 ? 1 : 0;
        // Rank 1 denotes the second smallest coordinate.
        i3 = rankx >= 1 ? 1 : 0;
        j3 = ranky >= 1 ? 1 : 0;
        k3 = rankz >= 1 ? 1 : 0;
        l3 = rankw >= 1 ? 1 : 0;
        // The fifth corner has all coordinate offsets = 1, so no need to compute that.
        Float x1 = x0 - i1 + G4; // Offsets for second corner in (x,y,z,w) coords
        Float y1 = y0 - j1 + G4;
        Float z1 = z0 - k1 + G4;
        Float w1 = w0 - l1 + G4;
        Float x2 = x0 - i2 + 2.0*G4; // Offsets for third corner in (x,y,z,w) coords
        Float y2 = y0 - j2 + 2.0*G4;
        Float z2 = z0 - k2 + 2.0*G4;
        Float w2 = w0 - l2 + 2.0*G4;
        Float x3 = x0 - i3 + 3.0*G4; // Offsets for fourth corner in (x,y,z,w) coords
        Float y3 = y0 - j3 + 3.0*G4;
        Float z3 = z0 - k3 + 3.0*G4;
        Float w3 = w0 - l3 + 3.0*G4;
        Float x4 = x0 - 1.0 + 4.0*G4; // Offsets for last corner in (x,y,z,w) coords
        Float y4 = y0 - 1.0 + 4.0*G4;
        Float z4 = z0 - 1.0 + 4.0*G4;
        Float w4 = w0 - 1.0 + 4.0*G4;
        // Work out the hashed gradient indices of the five simplex corners
        int ii = i & 255;
        int jj = j & 255;
        int kk = k & 255;
        int ll = l & 255;
        int gi0 = perm_[ii+perm_[jj+perm_[kk+perm_[ll]]]] % 32;
        int gi1 = perm_[ii+i1+perm_[jj+j1+perm_[kk+k1+perm_[ll+l1]]]] % 32;
        int gi2 = perm_[ii+i2+perm_[jj+j2+perm_[kk+k2+perm_[ll+l2]]]] % 32;
        int gi3 = perm_[ii+i3+perm_[jj+j3+perm_[kk+k3+perm_[ll+l3]]]] % 32;
        int gi4 = perm_[ii+1+perm_[jj+1+perm_[kk+1+perm_[ll+1]]]] % 32;
        // Calculate the contribution from the five corners
        // >Replaced 0.6 term by 0.5 for continuity at simplex boundaries
        // >Putting back 0.6 might enhance quality but I prefer continuity for now
        Float t0 = 0.5 - x0*x0 - y0*y0 - z0*z0 - w0*w0;
        if(t0<0) n0 = 0.0;
        else {
        t0 *= t0;
        n0 = t0 * t0 * dot(grad4[gi0], x0, y0, z0, w0);
        }
        Float t1 = 0.5 - x1*x1 - y1*y1 - z1*z1 - w1*w1;
        if(t1<0) n1 = 0.0;
        else {
        t1 *= t1;
        n1 = t1 * t1 * dot(grad4[gi1], x1, y1, z1, w1);
        }
        Float t2 = 0.5 - x2*x2 - y2*y2 - z2*z2 - w2*w2;
        if(t2<0) n2 = 0.0;
        else {
        t2 *= t2;
        n2 = t2 * t2 * dot(grad4[gi2], x2, y2, z2, w2);
        }
        Float t3 = 0.5 - x3*x3 - y3*y3 - z3*z3 - w3*w3;
        if(t3<0) n3 = 0.0;
        else {
        t3 *= t3;
        n3 = t3 * t3 * dot(grad4[gi3], x3, y3, z3, w3);
        }
        Float t4 = 0.5 - x4*x4 - y4*y4 - z4*z4 - w4*w4;
        if(t4<0) n4 = 0.0;
        else {
        t4 *= t4;
        n4 = t4 * t4 * dot(grad4[gi4], x4, y4, z4, w4);
        }
        // Sum up and scale the result to cover the range [-1,1]
        return 27.0 * (n0 + n1 + n2 + n3 + n4);
    }
};

/*
 * Cell noise implementation fork from:
 * https://github.com/jube/mapmaker/blob/master/src/lib/cell_noise.cc
 */

 /*
 * Copyright (c) 2014, Julien Bernard
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
// Cell noise generator for any Floating point type
template <typename Float=float,
          class = typename std::enable_if<std::is_floating_point<Float>::value>::type>
class CellNoise
{
public:
    typedef std::function<Float(const math::vec2&, const math::vec2&)> DistFunc;
    CellNoise(std::mt19937& engine, size_t count, DistFunc distance, std::vector<Float> coeffs)
    : count_(count)
    , distance_(distance)
    , coeffs_(std::move(coeffs))
    {
        // generate cells
        cells_.reserve(count_ * 4);
        std::uniform_real_distribution<double> dist_coord(0.0, 1.0);
        for (size_t ii=0; ii<count_; ++ii)
        {
            auto x = dist_coord(engine);
            auto y = dist_coord(engine);

            cells_.push_back({x, y});

            if (x < 0.5)
            {
                if (y < 0.5)
                {
                    cells_.push_back({x + 1.0, y      });
                    cells_.push_back({x      , y + 1.0});
                    cells_.push_back({x + 1.0, y + 1.0});
                }
                else
                {
                    cells_.push_back({x + 1.0, y      });
                    cells_.push_back({x      , y - 1.0});
                    cells_.push_back({x + 1.0, y - 1.0});
                }
            }
            else
            {
                if (y < 0.5)
                {
                    cells_.push_back({x - 1.0, y      });
                    cells_.push_back({x      , y + 1.0});
                    cells_.push_back({x - 1.0, y + 1.0});
                }
                else
                {
                    cells_.push_back({x - 1.0, y      });
                    cells_.push_back({x      , y - 1.0});
                    cells_.push_back({x - 1.0, y - 1.0});
                }
            }
        }

        // some sanity checks
        if (coeffs_.empty()) {
        coeffs_.push_back(1.0);
        }

        if (coeffs_.size() > cells_.size()) {
        coeffs_.resize(cells_.size());
        }
    }

    Float operator()(Float x, Float y)
    {
        Float rx = std::fmod(x, 1);
        Float ry = std::fmod(y, 1);

        auto size = coeffs_.size();

        math::vec2 here(rx,ry);

        std::partial_sort(cells_.begin(), cells_.begin() + size, cells_.end(), [&here, this](const math::vec2& lhs, const math::vec2& rhs)
        {
            return distance_(here,lhs) < distance_(here, rhs);
        });

        Float value = 0.0;

        for (decltype(size) i = 0; i < size; ++i)
            value += coeffs_[i] * distance_(here, cells_[i]);

        return value;
    }

private:
    size_t                 count_;
    DistFunc               distance_;
    std::vector<Float>     coeffs_;
    std::vector<math::vec2> cells_;
};

#endif // NOISE_POLICY_H
