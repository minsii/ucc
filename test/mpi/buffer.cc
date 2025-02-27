/**
 * Copyright (C) Mellanox Technologies Ltd. 2021.  ALL RIGHTS RESERVED.
 *
 * See file LICENSE for terms.
 */
#include <iostream>
#include <mpi.h>
#include <ucc/api/ucc.h>
BEGIN_C_DECLS
#include "components/mc/ucc_mc.h"
#include "utils/ucc_math.h"
END_C_DECLS
#include "test_mpi.h"

#define TEST_MPI_FP_EPSILON 1e-5

template<typename T>
void init_buffer_host(void *buf, size_t count, int _value)
{
    T *ptr = (T *)buf;
    for (size_t i = 0; i < count; i++) {
        ptr[i] = (T)((_value + i + 1) % 128);
    }
}

void init_buffer(void *_buf, size_t count, ucc_datatype_t dt,
                 ucc_memory_type_t mt, int value)
{
    void *buf = NULL;
    if (mt == UCC_MEMORY_TYPE_CUDA || mt == UCC_MEMORY_TYPE_ROCM) {
        buf = ucc_malloc(count * ucc_dt_size(dt), "buf");
        UCC_MALLOC_CHECK(buf);
    } else if (mt == UCC_MEMORY_TYPE_HOST) {
        buf = _buf;
    } else {
        std::cerr << "Unsupported mt\n";
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
    switch(dt) {
    case UCC_DT_INT8:
        init_buffer_host<int8_t>(buf, count, value);
        break;
    case UCC_DT_UINT8:
        init_buffer_host<uint8_t>(buf, count, value);
        break;
    case UCC_DT_INT16:
        init_buffer_host<int16_t>(buf, count, value);
        break;
    case UCC_DT_UINT16:
        init_buffer_host<uint16_t>(buf, count, value);
        break;
    case UCC_DT_INT32:
        init_buffer_host<int32_t>(buf, count, value);
        break;
    case UCC_DT_UINT32:
        init_buffer_host<uint32_t>(buf, count, value);
        break;
    case UCC_DT_INT64:
        init_buffer_host<int64_t>(buf, count, value);
        break;
    case UCC_DT_UINT64:
        init_buffer_host<uint64_t>(buf, count, value);
        break;
    case UCC_DT_FLOAT32:
        init_buffer_host<float>(buf, count, value);
        break;
    case UCC_DT_FLOAT64:
        init_buffer_host<double>(buf, count, value);
        break;
    default:
        std::cerr << "Unsupported dt\n";
        MPI_Abort(MPI_COMM_WORLD, -1);
        break;
    }
    if (UCC_MEMORY_TYPE_HOST != mt) {
        UCC_CHECK(ucc_mc_memcpy(_buf, buf, count * ucc_dt_size(dt),
                                mt, UCC_MEMORY_TYPE_HOST));
        ucc_free(buf);
    }
}

template<typename T>
static inline bool is_equal(T a, T b, T epsilon)
{
    return fabs(a - b) <= ((fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * epsilon);
}

template<typename T>
ucc_status_t compare_buffers_fp(T *b1, T *b2, size_t count) {
    T epsilon = (T)TEST_MPI_FP_EPSILON;
    for (size_t i = 0; i < count; i++) {
        if (!is_equal(b1[i], b2[i], epsilon)) {
            return UCC_ERR_NO_MESSAGE;
        }
    }
    return UCC_OK;
}

ucc_status_t compare_buffers(void *_rst, void *expected, size_t count,
                             ucc_datatype_t dt, ucc_memory_type_t mt)
{
    ucc_status_t status = UCC_ERR_NO_MESSAGE;
    ucc_mc_buffer_header_t *rst_mc_header;
    void *rst = NULL;

    if (UCC_MEMORY_TYPE_HOST == mt) {
        rst = _rst;
    } else if (UCC_MEMORY_TYPE_CUDA == mt || UCC_MEMORY_TYPE_ROCM == mt) {
        UCC_ALLOC_COPY_BUF(rst_mc_header, UCC_MEMORY_TYPE_HOST, _rst, mt,
                           count * ucc_dt_size(dt));
        rst = rst_mc_header->addr;
    } else {
        std::cerr << "Unsupported mt\n";
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    if (dt == UCC_DT_FLOAT32) {
        status = compare_buffers_fp<float>((float*)rst, (float*)expected, count);
    } else if (dt == UCC_DT_FLOAT64) {
        status = compare_buffers_fp<double>((double*)rst, (double*)expected,
                                            count);
    } else {
        status = memcmp(rst, expected, count*ucc_dt_size(dt)) ?
            UCC_ERR_NO_MESSAGE : UCC_OK;
    }

    if (UCC_MEMORY_TYPE_HOST != mt) {
        UCC_CHECK(ucc_mc_free(rst_mc_header));
    }

    return status;
}

template <typename T> void divide_buffers_fp(T *b, size_t divider, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        b[i] = b[i] / (T)divider;
    }
}

ucc_status_t divide_buffer(void *expected, size_t divider, size_t count,
                           ucc_datatype_t dt)
{
    if (dt == UCC_DT_FLOAT32) {
        divide_buffers_fp<float>((float *)expected, divider, count);
    }
    else if (dt == UCC_DT_FLOAT64) {
        divide_buffers_fp<double>((double *)expected, divider, count);
    }
    else {
        std::cerr << "Unsupported dt for avg\n";
        return UCC_ERR_NO_MESSAGE;
    }
    return UCC_OK;
}
