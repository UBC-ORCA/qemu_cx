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
        printf("QEMU Error - Legacy set.\n");
        return -1;
    }

    if (GET_BITS(env->ucx_sel, 31, 1) == 1) {
        printf("QEMU Error - Invalid bit set in Selector.\n");
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
        case 4:
            mcx_enable = env->mcx_enable4;
            break;
        case 5:
            mcx_enable = env->mcx_enable5;
            break;
        case 6:
            mcx_enable = env->mcx_enable6;
            break;
        case 7:
            mcx_enable = env->mcx_enable7;
            break;
        default:
            printf("Enable CSR (%d) not defined. Max CSR: 16\n", mcx_enable_csr);
            riscv_raise_exception(env, RISCV_EXCP_ILLEGAL_INST, GETPC());
            break;
    }

    if (mcx_enable == 0xFFFFFFFF) {
        printf("illegal instruction\n");
        riscv_raise_exception(env, RISCV_EXCP_ILLEGAL_INST, GETPC());
    }

    int32_t enable = GET_BITS(mcx_enable, (state_id + (cxu_id % 2) * (MAX_NUM_CXUS)), 1);

    // No exceptions in S mode
    if (enable == 0 && env->priv == PRV_U) {
        riscv_raise_exception(env, RISCV_EXCP_ILLEGAL_INST, GETPC());
    }

    if (env->ucx_prev_sel == CX_INVALID_SELECTOR) {
        riscv_raise_exception(env, RISCV_EXCP_ILLEGAL_INST, GETPC());
    }

    if ((env->ucx_sel != env->ucx_prev_sel) && 
         env->priv == PRV_U) {
        riscv_raise_exception(env, RISCV_EXCP_ILLEGAL_INST, GETPC());
    }

    uint32_t OPCODE_ID = cf_id;
    int32_t OPA = rs1;
    int32_t OPB = rs2;
    uint32_t CXU_ID = CX_GET_CXU_ID(env->ucx_sel);
    uint32_t STATE_ID = CX_GET_STATE_ID(env->ucx_sel);

    // stateless 
    if (num_states[CXU_ID] == 0 && STATE_ID > 0) {
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

    cx_idx_t sys_sel = {.idx = env->ucx_sel};
    int32_t out = cx_funcs[CXU_ID][OPCODE_ID](OPA, OPB, sys_sel);

    return (target_ulong)out;
}
