/**
 * Copyright (C) Mellanox Technologies Ltd. 2021.  ALL RIGHTS RESERVED.
 *
 * See file LICENSE for terms.
 */

#include <ucc/api/ucc.h>
#include <iostream>
#include "config.h"
extern "C" {
#include "utils/ucc_malloc.h"
}

#define STR(x) #x
#define UCCCHECK_GOTO(_call, _label, _status)                                  \
    do {                                                                       \
        _status = (_call);                                                     \
        if (UCC_OK != _status) {                                               \
            std::cerr << "UCC perftest error: " << ucc_status_string(_status)  \
                      << " in " << STR(_call) << "\n";                         \
            goto _label;                                                       \
        }                                                                      \
    } while (0)

#define UCC_MALLOC_CHECK_GOTO(_obj, _label, _status)                           \
    do {                                                                       \
        if (!(_obj)) {                                                         \
            _status = UCC_ERR_NO_MEMORY;                                       \
            std::cerr << "UCC perftest error: " << ucc_status_string(_status)  \
                      << "\n";                                                 \
            goto _label;                                                       \
        }                                                                      \
    } while (0)

#ifdef HAVE_CUDA
#include <cuda_runtime_api.h>
#define CUDA_CHECK_GOTO(_call, _label, _status)                                \
    do {                                                                       \
        _status = (_call);                                                     \
        if (cudaSuccess != _status) {                                          \
            std::cerr << "UCC perftest error: " << cudaGetErrorString(_status) \
                      << " in " << STR(_call) << "\n";                         \
            goto _label;                                                       \
        }                                                                      \
    } while (0)

#endif

#ifdef HAVE_ROCM
#include <hip/hip_runtime_api.h>
#define HIP_CHECK_GOTO(_call, _label, _status)                                 \
    do {                                                                       \
        _status = (_call);                                                     \
        if (hipSuccess != _status) {                                           \
            std::cerr << "UCC perftest error: " << hipGetErrorString(_status)  \
                      << " in " << STR(_call) << "\n";                         \
            goto _label;                                                       \
        }                                                                      \
    } while (0)

#endif
