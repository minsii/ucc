/**
 * Copyright (C) Mellanox Technologies Ltd. 2021.  ALL RIGHTS RESERVED.
 *
 * See file LICENSE for terms.
 */

#ifndef UCC_TL_CUDA_H_
#define UCC_TL_CUDA_H_

#include "components/tl/ucc_tl.h"
#include "components/tl/ucc_tl_log.h"
#include "utils/ucc_mpool.h"
#include "tl_cuda_ep_hash.h"
#include "tl_cuda_topo.h"
#include "tl_cuda_team_topo.h"
#include <cuda_runtime.h>

#ifndef UCC_TL_CUDA_DEFAULT_SCORE
#define UCC_TL_CUDA_DEFAULT_SCORE 40
#endif

#define UCC_TL_CUDA_MAX_PEERS 8
#define UCC_TL_CUDA_SUPPORTED_COLLS                                            \
    (UCC_COLL_TYPE_ALLTOALL)

#define UCC_TL_CUDA_TEAM_LIB(_team)                                            \
    (ucc_derived_of((_team)->super.super.context->lib, ucc_tl_cuda_lib_t))

#define UCC_TL_CUDA_TEAM_CTX(_team)                                            \
    (ucc_derived_of((_team)->super.super.context, ucc_tl_cuda_context_t))

#define UCC_TL_CUDA_TEAM_SYNC(_team, _rank, _id)                               \
    ({                                                                         \
        size_t _ctrl_size_rank = (sizeof(ucc_tl_cuda_sync_t)  +                \
                                  sizeof(ucc_tl_cuda_sync_data_t) *            \
                                  (UCC_TL_TEAM_SIZE(_team) - 1)) ;             \
        size_t _ctrl_size = _ctrl_size_rank * UCC_TL_TEAM_SIZE(_team);         \
        void *_sync = PTR_OFFSET(_team->sync, _ctrl_size * (_id) +             \
                                 _ctrl_size_rank * (_rank));                   \
        (ucc_tl_cuda_sync_t*)_sync;                                            \
    })

#define UCC_TL_CUDA_TEAM_BARRIER(_team, _id)                                   \
    ({                                                                         \
        size_t _bar_size = sizeof(ucc_tl_cuda_shm_barrier_t);                  \
        void *_bar = PTR_OFFSET(_team->bar, _bar_size * (_id));                \
        (ucc_tl_cuda_shm_barrier_t*)_bar;                                      \
    })

#ifdef HAVE_PROFILING_TL_CUDA
#include "utils/profile/ucc_profile.h"
#else
#include "utils/profile/ucc_profile_off.h"
#endif

#define UCC_TL_CUDA_PROFILE_FUNC UCC_PROFILE_FUNC
#define UCC_TL_CUDA_PROFILE_REQUEST_NEW UCC_PROFILE_REQUEST_NEW
#define UCC_TL_CUDA_PROFILE_REQUEST_EVENT UCC_PROFILE_REQUEST_EVENT
#define UCC_TL_CUDA_PROFILE_REQUEST_FREE UCC_PROFILE_REQUEST_FREE

typedef struct ucc_tl_cuda_iface {
    ucc_tl_iface_t super;
} ucc_tl_cuda_iface_t;

extern ucc_tl_cuda_iface_t ucc_tl_cuda;

typedef struct ucc_tl_cuda_lib_config {
    ucc_tl_lib_config_t super;
    uint32_t            max_concurrent;
} ucc_tl_cuda_lib_config_t;

typedef struct ucc_tl_cuda_context_config {
    ucc_tl_context_config_t super;
} ucc_tl_cuda_context_config_t;

typedef struct ucc_tl_cuda_lib {
    ucc_tl_lib_t             super;
    ucc_tl_cuda_lib_config_t cfg;
} ucc_tl_cuda_lib_t;
UCC_CLASS_DECLARE(ucc_tl_cuda_lib_t, const ucc_base_lib_params_t *,
                  const ucc_base_config_t *);

typedef struct ucc_tl_cuda_context {
    ucc_tl_context_t              super;
    ucc_tl_cuda_context_config_t  cfg;
    int                           device;
    ucc_tl_cuda_device_pci_id_t   device_id;
    ucc_tl_cuda_topo_t           *topo;
    ucc_mpool_t                   req_mp;
    tl_cuda_ep_hash_t            *ipc_cache;
} ucc_tl_cuda_context_t;
UCC_CLASS_DECLARE(ucc_tl_cuda_context_t, const ucc_base_context_params_t *,
                  const ucc_base_config_t *);

typedef uint32_t ucc_tl_cuda_sync_state_t;

typedef struct ucc_tl_cuda_shm_barrier {
    ucc_rank_t   size;
    ucc_rank_t   count;
    int          sense;
    ucc_status_t state[UCC_TL_CUDA_MAX_PEERS];
    int          local_sense[UCC_TL_CUDA_MAX_PEERS];
} ucc_tl_cuda_shm_barrier_t;

typedef struct ucc_tl_cuda_rank_id {
    int                         device;
    ucc_tl_cuda_device_pci_id_t pci_id;
    int                         shm;
} ucc_tl_cuda_rank_id_t;

typedef struct ucc_tl_cuda_sync_data {
    cudaEvent_t ipc_event_remote;
} ucc_tl_cuda_sync_data_t;

typedef struct ucc_tl_cuda_mem_info {
    void               *ptr;
    size_t              length;
    size_t              offset;
    cudaIpcMemHandle_t  handle;
} ucc_tl_cuda_mem_info_t;

typedef struct ucc_tl_cuda_sync {
    uint32_t                 seq_num[2];
    ucc_tl_cuda_mem_info_t   mem_info_src;
    ucc_tl_cuda_mem_info_t   mem_info_dst;
    cudaEvent_t              ipc_event_local;
    cudaIpcEventHandle_t     ev_handle;
    ucc_tl_cuda_sync_data_t  data[1];
} ucc_tl_cuda_sync_t;

typedef struct ucc_tl_cuda_team {
    ucc_tl_team_t              super;
    uint32_t                   seq_num;
    ucc_tl_cuda_team_topo_t   *topo;
    ucc_tl_cuda_sync_t        *sync;
    ucc_tl_cuda_sync_state_t  *sync_state;
    ucc_tl_cuda_shm_barrier_t *bar;
    cudaStream_t               stream;
    ucc_tl_cuda_rank_id_t     *ids;
    ucc_team_oob_coll_t        oob;
    void                      *oob_req;
} ucc_tl_cuda_team_t;

UCC_CLASS_DECLARE(ucc_tl_cuda_team_t, ucc_base_context_t *,
                  const ucc_base_team_params_t *);

typedef struct ucc_tl_cuda_task {
    ucc_coll_task_t            super;
    uint32_t                   seq_num;
    uint32_t                   coll_id;
    ucc_tl_cuda_shm_barrier_t *bar;
    union {
        struct {
            int                     stage;
            ucc_tl_cuda_mem_info_t  mem_info_src;
            ucc_tl_cuda_mem_info_t  mem_info_dst;
            void                   *peer_map_addr_src[UCC_TL_CUDA_MAX_PEERS];
            void                   *peer_map_addr_dst[UCC_TL_CUDA_MAX_PEERS];
            void                   *copy_done;
        } alltoall_ce;
    };
} ucc_tl_cuda_task_t;

#endif
