/**
 * Copyright (C) Mellanox Technologies Ltd. 2021.  ALL RIGHTS RESERVED.
 * Copyright (C) Advanced Micro Devices, Inc. 2022. ALL RIGHTS RESERVED.
 * See file LICENSE for terms.
 */

extern "C" {
#include <components/mc/ucc_mc.h>
#include <pthread.h>
}
#include <common/test.h>
#include <hip/hip_runtime.h>
#include <vector>

void *mt_ucc_mc_rocm_allocs(void *args)
{
    // Final size will be:
    // size * (quantifier^(num_of_allocs/2))
    // and should be larger than mpool buffer size which is 1MB by default,
    // to assure testing both fast and slow ucc_mc_alloc path.
    // if num_of_allocs is changed, change quantifier accordingly.
    int                                   quantifier    = 2;
    size_t                                size          = 4;
    int                                   num_of_allocs = 40;
    std::vector<ucc_mc_buffer_header_t *> headers;
    std::vector<void *>                   pointers;
    headers.resize(num_of_allocs);
    pointers.resize(num_of_allocs);

    for (int i = 0; i < num_of_allocs; i++) {
        EXPECT_EQ(UCC_OK,
                  ucc_mc_alloc(&headers[i], size, UCC_MEMORY_TYPE_ROCM));
        pointers[i] = headers[i]->addr;
        EXPECT_EQ(hipSuccess, hipMemset(pointers[i], 0, size));
        EXPECT_EQ(UCC_OK, ucc_mc_free(headers[i]));
        if (i % 2) {
            size *= quantifier;
        }
    }
    return 0;
}

class test_mc_rocm : public ucc::test {
  protected:
    const int               TEST_ALLOC_SIZE = 1024;
    ucc_mc_buffer_header_t *mc_header;
    void *                  test_ptr;
    ucc_memory_type_t       test_mtype;
    void TestMCRocmSetUp(ucc_mc_params_t mc_params)
    {
        ucc::test::SetUp();
        ucc_constructor();
        ucc_mc_init(&mc_params);
        test_ptr   = NULL;
        test_mtype = UCC_MEMORY_TYPE_UNKNOWN;
    }
    virtual void SetUp() override
    {
        ucc_mc_params_t mc_params = {
            .thread_mode = UCC_THREAD_SINGLE,
        };
        TestMCRocmSetUp(mc_params);
    }
    virtual void TearDown() override
    {
        ucc_mc_finalize();
        ucc::test::TearDown();
    }
};

class test_mc_rocm_mt : public test_mc_rocm {
  protected:
    virtual void SetUp() override
    {
        ucc_mc_params_t mc_params = {
            .thread_mode = UCC_THREAD_MULTIPLE,
        };
        TestMCRocmSetUp(mc_params);
    }
};

UCC_TEST_F(test_mc_rocm, mc_rocm_load)
{
    EXPECT_EQ(UCC_OK, ucc_mc_available(UCC_MEMORY_TYPE_ROCM));
}

UCC_TEST_F(test_mc_rocm, can_alloc_and_free_mem)
{
    // mpool will be used only if size is smaller than UCC_MC_CPU_ELEM_SIZE, which by default set to 1MB and is configurable at runtime.
    EXPECT_EQ(UCC_OK,
              ucc_mc_alloc(&mc_header, TEST_ALLOC_SIZE, UCC_MEMORY_TYPE_ROCM));
    test_ptr = mc_header->addr;
    EXPECT_EQ(hipSuccess, hipMemset(test_ptr, 0, TEST_ALLOC_SIZE));
    EXPECT_EQ(UCC_OK, ucc_mc_free(mc_header));
}

// Disabled because can't reinit mc with different thread mode
UCC_TEST_F(test_mc_rocm_mt, DISABLED_can_alloc_and_free_mem_mt)
{
    // mpool will be used only if size is smaller than UCC_MC_CPU_ELEM_SIZE, which by default set to 1MB and is configurable at runtime.
    int                    num_of_threads = 10;
    std::vector<pthread_t> threads;
    threads.resize(num_of_threads);

    for (int i = 0; i < num_of_threads; i++) {
        pthread_create(&threads[i], NULL, &mt_ucc_mc_rocm_allocs, NULL);
    }
    for (int i = 0; i < num_of_threads; i++) {
        pthread_join(threads[i], NULL);
    }
}

UCC_TEST_F(test_mc_rocm, can_detect_host_mem)
{
    ucc_mem_attr_t mem_attr;

    mem_attr.field_mask = UCC_MEM_ATTR_FIELD_MEM_TYPE;
    test_ptr = malloc(TEST_ALLOC_SIZE);
    if (test_ptr == NULL) {
        ADD_FAILURE() << "failed to allocate host memory";
    }
    EXPECT_EQ(UCC_OK, ucc_mc_get_mem_attr(test_ptr, &mem_attr));
    EXPECT_EQ(UCC_MEMORY_TYPE_HOST, mem_attr.mem_type);
    free(test_ptr);
}

UCC_TEST_F(test_mc_rocm, can_detect_rocm_mem)
{
    ucc_mem_attr_t mem_attr;
    hipError_t st;

    mem_attr.field_mask = UCC_MEM_ATTR_FIELD_MEM_TYPE;
    st = hipMalloc(&test_ptr, TEST_ALLOC_SIZE);
    if (st != hipSuccess) {
        ADD_FAILURE() << "failed to allocate device memory";
    }
    EXPECT_EQ(UCC_OK, ucc_mc_get_mem_attr(test_ptr, &mem_attr));
    EXPECT_EQ(UCC_MEMORY_TYPE_ROCM, mem_attr.mem_type);
    hipFree(test_ptr);
    ucc_mc_finalize();
}

UCC_TEST_F(test_mc_rocm, can_query_rocm_mem)
{
    ucc_mem_attr_t mem_attr;
    hipError_t st;
    void *ptr;

    st = hipMalloc(&test_ptr, TEST_ALLOC_SIZE);
    if (st != hipSuccess) {
        ADD_FAILURE() << "failed to allocate device memory";
    }

    mem_attr.field_mask = UCC_MEM_ATTR_FIELD_MEM_TYPE;
    EXPECT_EQ(UCC_OK, ucc_mc_get_mem_attr(test_ptr, &mem_attr));
    EXPECT_EQ(UCC_MEMORY_TYPE_ROCM, mem_attr.mem_type);

    /* query base addr and length */
    mem_attr.field_mask   = UCC_MEM_ATTR_FIELD_MEM_TYPE |
                            UCC_MEM_ATTR_FIELD_BASE_ADDRESS |
                            UCC_MEM_ATTR_FIELD_ALLOC_LENGTH;
    mem_attr.alloc_length = 1;
    ptr = (char *)test_ptr + TEST_ALLOC_SIZE/2;
    EXPECT_EQ(UCC_OK, ucc_mc_get_mem_attr(ptr, &mem_attr));
    EXPECT_EQ(UCC_MEMORY_TYPE_ROCM, mem_attr.mem_type);
    EXPECT_EQ(test_ptr, mem_attr.base_address);
    EXPECT_EQ(TEST_ALLOC_SIZE, mem_attr.alloc_length);

    hipFree(test_ptr);
    ucc_mc_finalize();
}

UCC_TEST_F(test_mc_rocm, can_detect_managed_mem)
{
    hipError_t st;
    ucc_mem_attr_t mem_attr;

    st = hipMallocManaged(&test_ptr, TEST_ALLOC_SIZE);
    if (st != hipSuccess) {
        ADD_FAILURE() << "failed to allocate managed memory";
    }
    mem_attr.field_mask = UCC_MEM_ATTR_FIELD_MEM_TYPE;
    EXPECT_EQ(UCC_OK, ucc_mc_get_mem_attr(test_ptr, &mem_attr));
    EXPECT_EQ(UCC_MEMORY_TYPE_ROCM_MANAGED, mem_attr.mem_type);
    hipFree(test_ptr);
}

UCC_TEST_F(test_mc_rocm, can_detect_host_alloc_mem)
{
    hipError_t st;
    ucc_mem_attr_t mem_attr;

    st = hipHostMalloc(&test_ptr, TEST_ALLOC_SIZE, hipHostMallocDefault);
    if (st != hipSuccess) {
        ADD_FAILURE() << "failed to allocate host mapped memory";
    }
    mem_attr.field_mask = UCC_MEM_ATTR_FIELD_MEM_TYPE;
    EXPECT_EQ(UCC_OK, ucc_mc_get_mem_attr(test_ptr, &mem_attr));
    EXPECT_EQ(UCC_MEMORY_TYPE_HOST, mem_attr.mem_type);
    hipFreeHost(test_ptr);
}
