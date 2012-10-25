/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 30, 2011, 7:43 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 25, 2011, 8:57 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef EXFEL_XIP_GPUIMAGE_CU
#define	EXFEL_XIP_GPUIMAGE_CU

#include <exfel/util/Types.hh>
#include <cuda_runtime_api.h>

namespace ut = exfel::util;

// Multiprocessors
#define BLOCK_SIZE 16

template<typename T>
__global__ void kernel_fill(T* img, int size, T value) {
  const int gridSize = blockDim.x * gridDim.x;
  int idx = threadIdx.x + blockDim.x * blockIdx.x;

  while (idx < size) {
    img[idx] = value;
    idx += gridSize;
  };
}

extern "C"
void cudaFill(ut::Types::Type type, void* img, int size, void* value) {
  switch (type) {
    case ut::Types::INT16:
      kernel_fill << <64, 128 >> > ((short*)img, size, *((short*)value));
      break;
    case ut::Types::INT32:
      kernel_fill << <64, 128 >> > ((int*)img, size, *((int*)value));
      break;
    case ut::Types::FLOAT:
      kernel_fill << <64, 128 >> > ((float*)img, size, *((float*)value));
      break;
  }
}



#endif

