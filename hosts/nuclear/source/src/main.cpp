#include <iostream>
#include <cmath>
#include "math3d.h"
#include "bounding_boxes.h"
#include "logger.h"

using namespace wcore;
using namespace math;

/*
int glhUnProjectf(float winx, float winy, float winz, float *modelview, float *projection, int *viewport, float *objectCoordinate)
{
  // Transformation matrices
  float m[16], A[16];
  float in[4], out[4];
  // Calculation for inverting a matrix, compute projection x modelview
  // and store in A[16]
  MultiplyMatrices4by4OpenGL_FLOAT(A, projection, modelview);
  // Now compute the inverse of matrix A
  if(glhInvertMatrixf2(A, m)==0)
     return 0;
  // Transformation of normalized coordinates between -1 and 1
  in[0]=(winx-(float)viewport[0])/(float)viewport[2]*2.0-1.0;
  in[1]=(winy-(float)viewport[1])/(float)viewport[3]*2.0-1.0;
  in[2]=2.0*winz-1.0;
  in[3]=1.0;
  // Objects coordinates
  MultiplyMatrixByVector4by4OpenGL_FLOAT(out, m, in);
  if(out[3]==0.0)
     return 0;
  out[3]=1.0/out[3];
  objectCoordinate[0]=out[0]*out[3];
  objectCoordinate[1]=out[1]*out[3];
  objectCoordinate[2]=out[2]*out[3];
  return 1;
}*/

int main()
{
    float scr_width = 1024;
    float scr_height = 768;
    float aspect = scr_width/scr_height;
    float NEAR = 0.1f;
    float FAR = 100.0f;

    math::Frustum frustum({-aspect*NEAR, aspect*NEAR, -NEAR, NEAR, -NEAR, -FAR});

    mat4 P;
    init_frustum(P, frustum);

    return 0;
}
