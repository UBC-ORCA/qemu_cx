// qemu/target/riscv/
#include "qemu/osdep.h"
#include "exec/exec-all.h"
#include "exec/helper-proto.h"
#include "cpu_bits.h"

#include <assert.h>
#include <stdio.h>

#include "../../../zoo/exports.h"
#include "../../../include/utils.h"

target_ulong HELPER(cx_reg)(CPURISCVState *env, target_ulong cf_id, 
                             target_ulong rs1, target_ulong rs2)
{
    // not sure if these are the right error values to set it to
    // if (env->ucx_sel == CX_INVALID_SELECTOR) {
    //     cx_status_t cx_status = {.idx = env->cx_status};
    //     cx_status.sel.IV = 1;
    //     cx_status.sel.IC = 1;
    //     cx_status.sel.IS = 1;
    //     env->cx_status = cx_status.idx;
    //     return (target_ulong)-1;
    // }

    /* Need to check the correct enable bit in the correct CSR */

    if (env->ucx_sel == CX_LEGACY) {
        return -1;
    }

    if (env->ucx_sel == CX_INVALID_SELECTOR) {
        return -1;
    }

    int cxu_id = CX_GET_CXU_ID(env->ucx_sel);
    int state_id = CX_GET_STATE_ID(env->ucx_sel);

    int mcx_enable_csr = cxu_id / 2;
    int mcx_enable = 0xFFFFFFFF;

    switch (mcx_enable_csr) {
        case 0:
            mcx_enable = env->mcx_enable0;
            break;
        case 1:
            mcx_enable = env->mcx_enable1;
            break;
        case 2:
            mcx_enable = env->mcx_enable2;
            break;
        case 3:
            mcx_enable = env->mcx_enable3;
            break;
        default:
            printf("Further CSRs not defined (4-7); I should do that eventually\n");
            break;
    }

    int32_t enable = GET_BITS(mcx_enable, (state_id + (cxu_id % 2) * (MAX_NUM_CXUS)), 1);
    
    // No exceptions in S mode
    if (enable == 0 && env->priv == PRV_U) {
        riscv_raise_exception(env, RISCV_EXCP_ILLEGAL_INST, GETPC());
    }

    uint32_t OPCODE_ID = cf_id;
    int32_t OPA = rs1;
    int32_t OPB = rs2;
    uint32_t CXU_ID = CX_GET_CXU_ID(env->ucx_sel);
    uint32_t STATE_ID = CX_GET_STATE_ID(env->ucx_sel);

    // stateless 
    if (STATE_ID > 0 && num_states[CXU_ID] == 0) {
        cx_status_t cx_status = {.idx = env->cx_status};
        cx_status.sel.IS = 1;
        env->cx_status = cx_status.idx;
    }
    // stateful
    else if (STATE_ID > num_states[CXU_ID] - 1) {
        cx_status_t cx_status = {.idx = env->cx_status};
        cx_status.sel.IS = 1;
        env->cx_status = cx_status.idx;
    }

    if (OPCODE_ID > num_cfs[CXU_ID] - 1) {
        cx_status_t cx_status = {.idx = env->cx_status};
        cx_status.sel.IF = 1;
        env->cx_status = cx_status.idx;
    }
    
    assert( CXU_ID < NUM_CXUS ); // Possibly redundant
    int32_t out = cx_funcs[CXU_ID][OPCODE_ID](OPA, OPB, STATE_ID);

    return (target_ulong)out;
} 