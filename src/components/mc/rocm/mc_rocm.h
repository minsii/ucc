/**
 * Copyright (C) Mellanox Technologies Ltd. 2021.  ALL RIGHTS RESERVED.
 * Copyright (C) Advanced Micro Devices, Inc. 2022. ALL RIGHTS RESERVED.
 *
 * See file LICENSE for terms.
 */

#ifndef UCC_MC_ROCM_H_
#define UCC_MC_ROCM_H_

#include "components/mc/base/ucc_mc_base.h"
#include "components/mc/ucc_mc_log.h"
#include "utils/ucc_mpool.h"
#include <hip/hip_runtime_api.h>


static inline ucc_status_t hip_error_to_ucc_status(hipError_t hip_err)
{
    switch(hip_err) {
    case hipSuccess:
        return UCC_OK;
    case hipErrorNotReady:
        return UCC_INPROGRESS;
    default:
        break;
    }
    return UCC_ERR_NO_MESSAGE;
}

typedef ucc_status_t (*ucc_mc_rocm_task_post_fn) (uint32_t *dev_status,
                                                  int blocking_wait,
                                                  hipStream_t stream);

typedef struct ucc_mc_rocm_config {
    ucc_mc_config_t                super;
    unsigned long                  reduce_num_blocks;
    int                            reduce_num_threads;
    size_t                         mpool_elem_size;
    int                            mpool_max_elems;
} ucc_mc_rocm_config_t;

typedef struct ucc_mc_rocm {
    ucc_mc_base_t                  super;
    hipStream_t                    stream;
    ucc_mpool_t                    events;
    ucc_mpool_t                    strm_reqs;
    ucc_mpool_t                    mpool;
    int                            mpool_init_flag;
    ucc_spinlock_t                 init_spinlock;
    ucc_thread_mode_t              thread_mode;
} ucc_mc_rocm_t;


extern ucc_mc_rocm_t ucc_mc_rocm;
#define ROCMCHECK(cmd) do {                                                    \
        hipError_t e = cmd;                                                    \
        if(e != hipSuccess) {                                                  \
            mc_error(&ucc_mc_rocm.super, "ROCm failed with ret:%d(%s)", e,     \
                     hipGetErrorString(e));                                    \
            return UCC_ERR_NO_MESSAGE;                                         \
        }                                                                      \
} while(0)

#define ROCM_FUNC(_func)                                                       \
    ({                                                                         \
        ucc_status_t _status = UCC_OK;                                         \
        do {                                                                   \
            hipError_t _result = (_func);                                      \
            if (hipSuccess != _result) {                                       \
                mc_error(&ucc_mc_rocm.super, "%s() failed: %s",                \
                       #_func, hipGetErrorString(_result));                    \
                _status = UCC_ERR_INVALID_PARAM;                               \
            }                                                                  \
        } while (0);                                                           \
        _status;                                                               \
    })

#define MC_ROCM_CONFIG                                                         \
    (ucc_derived_of(ucc_mc_rocm.super.config, ucc_mc_rocm_config_t))

#define UCC_MC_ROCM_INIT_STREAM() do {                                         \
    if (ucc_mc_rocm.stream == NULL) {                                          \
        hipError_t hip_st = hipSuccess;                                        \
        ucc_spin_lock(&ucc_mc_rocm.init_spinlock);                             \
        if (ucc_mc_rocm.stream == NULL) {                                      \
            hip_st = hipStreamCreateWithFlags(&ucc_mc_rocm.stream,             \
                                                hipStreamNonBlocking);         \
        }                                                                      \
        ucc_spin_unlock(&ucc_mc_rocm.init_spinlock);                           \
        if(hip_st != hipSuccess) {                                             \
            mc_error(&ucc_mc_rocm.super, "rocm failed with ret:%d(%s)",        \
                     hip_st, hipGetErrorString(hip_st));                       \
            return UCC_ERR_NO_MESSAGE;                                         \
        }                                                                      \
    }                                                                          \
} while(0)

#endif
