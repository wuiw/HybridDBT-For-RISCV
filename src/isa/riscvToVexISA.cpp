/*
 * riscvToVexISA.cpp
 *
 *  Created on: 4 janv. 2018
 *      Author: simon
 */

#include <isa/riscvToVexISA.h>
#include <isa/vexISA.h>

#ifndef __HW
#ifndef __SW
ac_int<7, false> functBindingOP[8]   = {VEX_ADD, VEX_SLL, VEX_CMPLT, VEX_CMPLTU, VEX_XOR, 0, VEX_OR, VEX_AND};
ac_int<7, false> functBindingOPI[8]  = {VEX_ADDi, VEX_SLLi, VEX_CMPLTi, VEX_CMPLTUi, VEX_XORi, 0, VEX_ORi, VEX_ANDi};
ac_int<7, false> functBindingLD[8]   = {VEX_LDB, VEX_LDH, VEX_LDW, VEX_LDD, VEX_LDBU, VEX_LDHU, VEX_LDWU};
ac_int<7, false> functBindingST[8]   = {VEX_STB, VEX_STH, VEX_STW, VEX_STD};
ac_int<7, false> functBindingBR[8]   = {VEX_BR, VEX_BRF, 0, 0, VEX_BLT, VEX_BGE, VEX_BLTU, VEX_BGEU};
ac_int<7, false> functBindingMULT[8] = {VEX_MPY, VEX_MPYH, VEX_MPYHSU, VEX_MPYHU, VEX_DIV, VEX_DIVU, VEX_REM, VEX_REMU};
ac_int<7, false> functBindingMULTW[8] = {VEX_MPYW, 0, 0, 0, VEX_DIVW, VEX_DIVUW, VEX_REMW, VEX_REMUW};
#endif
#endif

char functBindingOP_sw[8]    = {VEX_ADD, VEX_SLL, VEX_CMPLT, VEX_CMPLTU, VEX_XOR, 0, VEX_OR, VEX_AND};
char functBindingOPI_sw[8]   = {VEX_ADDi, VEX_SLLi, VEX_CMPLTi, VEX_CMPLTUi, VEX_XORi, 0, VEX_ORi, VEX_ANDi};
char functBindingLD_sw[8]    = {VEX_LDB, VEX_LDH, VEX_LDW, VEX_LDD, VEX_LDBU, VEX_LDHU, VEX_LDWU};
char functBindingST_sw[8]    = {VEX_STB, VEX_STH, VEX_STW, VEX_STD};
char functBindingBR_sw[8]    = {VEX_BR, VEX_BRF, 0, 0, VEX_BLT, VEX_BGE, VEX_BLTU, VEX_BGEU};
char functBindingMULT_sw[8]  = {VEX_MPY, VEX_MPYH, VEX_MPYHSU, VEX_MPYHU, VEX_DIV, VEX_DIVU, VEX_REM, VEX_REMU};
char functBindingMULTW_sw[8] = {VEX_MPYW, 0, 0, 0, VEX_DIVW, VEX_DIVUW, VEX_REMW, VEX_REMUW};
