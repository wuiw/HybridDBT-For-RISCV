/**************************************************************************
 *
                       PipeLined Four Issues Processor  CUSTOM MOV
 *
 **************************************************************************/

/*	This is a four ways pipelined (5 stages) prossecor simulator
 *		All the ways can perform common operations
 *		But some ways can perform further operations :
 *		->	Way 1 can perform Branch operations too
 *		->	Way 2 can perform Memory access operations too
 * 		->	Way 4 can perform Multiplication operations too
 *
 *		The different stages are :
 *		-> Fetch (F) 		: Access Instruction registers and store current instruction
 *		-> Decode (DC)		: Decode the instruction and select the needed operands for the next stage
 *(including accessing to registers)
 *		-> Execute (EX) 	: Do the calculating part
 *		-> Memory (M)		: Access memory if needed (Only for Way 2)
 *		-> Write Back (WB) 	: Update registers value if needed
 *
 *		For any questions, please contact yo.uguen@gmail.com
 */

#ifndef __NIOS

#ifndef __CATAPULT

#include <cmath>
#include <cstring>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

using namespace std;
#endif
#include <isa/vexISA.h>
#include <simulator/vexSimulator.h>
#include <types.h>

#define MAX_NUMBER_OF_INSTRUCTIONS 65536
#define REG_NUMBER 64
#define BRANCH_UNIT_NUMBER 8
#define DATA_SIZE 65536
#define SIZE_INSTRUCTION 128
#define SIZE_MEM_ADDR 16

#define __VERBOSE
// Declaration of different structs

char incrementInstrMem = 2;
// 64 bits registers

#ifdef __CATAPULT
ac_int<64, true> REG[64];
ac_int<64, false> PC, NEXT_PC, cycle = 0;

struct MemtoWB memtoWB1;
struct MemtoWB memtoWB2;
struct MemtoWB memtoWB3;
struct MemtoWB memtoWB4;
struct MemtoWB memtoWB5;
struct MemtoWB memtoWB6;
struct MemtoWB memtoWB7;
struct MemtoWB memtoWB8;
struct ExtoMem extoMem1;
struct ExtoMem extoMem2;
struct ExtoMem extoMem3;
struct ExtoMem extoMem4;
struct ExtoMem extoMem5;
struct ExtoMem extoMem6;
struct ExtoMem extoMem7;
struct ExtoMem extoMem8;
struct DCtoEx dctoEx1;
struct DCtoEx dctoEx2;
struct DCtoEx dctoEx3;
struct DCtoEx dctoEx4;
struct DCtoEx dctoEx5;
struct DCtoEx dctoEx6;
struct DCtoEx dctoEx7;
struct DCtoEx dctoEx8;
struct FtoDC ftoDC1;
struct FtoDC ftoDC2;
struct FtoDC ftoDC3;
struct FtoDC ftoDC4;
struct FtoDC ftoDC5;
struct FtoDC ftoDC6;
struct FtoDC ftoDC7;
struct FtoDC ftoDC8;
ac_int<1, false> stop;

#endif

// Cycle counter

typedef ac_int<32, false> acuint32;
typedef ac_int<8, false> acuint8;

#ifdef __CATAPULT
ac_int<64, false> ldd(unsigned int addr, ac_int<8, false> memory0[65536], ac_int<8, false> memory1[65536],
                      ac_int<8, false> memory2[65536], ac_int<8, false> memory3[65536], ac_int<8, false> memory4[65536],
                      ac_int<8, false> memory5[65536], ac_int<8, false> memory6[65536], ac_int<8, false> memory7[65536])
{
  ac_int<64, false> result = 0;
  result.set_slc(0, memory0[addr >> 2]);
  result.set_slc(8, memory1[addr >> 2]);
  result.set_slc(16, memory2[addr >> 2]);
  result.set_slc(24, memory3[addr >> 2]);
  result.set_slc(32, memory4[addr >> 2]);
  result.set_slc(40, memory5[addr >> 2]);
  result.set_slc(48, memory6[addr >> 2]);
  result.set_slc(56, memory7[addr >> 2]);

  return result;
}

#endif

#ifdef __CATAPULT
void doWB(struct MemtoWB memtoWB)
{
#else
void VexSimulator::doWB(struct MemtoWB memtoWB)
{
#endif

  if (memtoWB.isFloat && memtoWB.WBena) {
    regf[memtoWB.dest] = memtoWB.floatRes;
  } else if (memtoWB.WBena && memtoWB.dest != 0) {

    REG[memtoWB.dest] = memtoWB.result;
  }
}
#ifdef __CATAPULT
void doMemNoMem(struct ExtoMem extoMem, struct MemtoWB* memtoWB)
{
#else
void VexSimulator::doMemNoMem(struct ExtoMem extoMem, struct MemtoWB* memtoWB)
{
#endif

  memtoWB->WBena    = extoMem.WBena;
  memtoWB->dest     = extoMem.dest;
  memtoWB->result   = extoMem.result;
  memtoWB->isFloat  = extoMem.isFloat;
  memtoWB->floatRes = extoMem.floatRes;
  memtoWB->pc       = extoMem.pc;
}

#ifdef __CATAPULT
void doMem(struct ExtoMem extoMem, struct MemtoWB* memtoWB, ac_int<8, false> memory0[65536],
           ac_int<8, false> memory1[65536], ac_int<8, false> memory2[65536], ac_int<8, false> memory3[65536],
           ac_int<8, false> memory4[65536], ac_int<8, false> memory5[65536], ac_int<8, false> memory6[65536],
           ac_int<8, false> memory7[65536])
{
#else
void VexSimulator::doMem(struct ExtoMem extoMem, struct MemtoWB* memtoWB)
{
#endif

  memtoWB->WBena    = extoMem.WBena;
  memtoWB->dest     = extoMem.dest;
  memtoWB->isFloat  = extoMem.isFloat;
  memtoWB->floatRes = extoMem.floatRes;
  memtoWB->pc       = extoMem.pc;

  // IMPORTANT NOTE: In this function the value of exToMem.address is the address of the memory access to perform
  // 					and it is addressing byte. In current implementation DATA is a word array so we
  // need to
  //					divide the address by four to have actual place inside the array

  ac_int<64, false> address = extoMem.result;

  if (extoMem.opCode == VEX_FLW || extoMem.opCode == VEX_FLH || extoMem.opCode == VEX_FLB) {

    if (extoMem.opCode == VEX_FLW) {

      unsigned int value;
      if (extoMem.result[2])
        value = (unsigned int)extoMem.memValue.slc<32>(32);
      else
        value = (unsigned int)extoMem.memValue.slc<32>(0);

      memcpy(&memtoWB->floatRes, &value, 4);

      memtoWB->isFloat = 1;
    }

  } else if (extoMem.opCode == VEX_FSW || extoMem.opCode == VEX_FSH || extoMem.opCode == VEX_FSB) {
    memtoWB->WBena = 0; // TODO : this shouldn't be necessary : WB shouldn't be enabled before

    if (extoMem.opCode == VEX_FSW) {
      unsigned int value;
      memcpy(&value, &extoMem.floatRes, 4);

      stw(extoMem.result, value);
    } else if (extoMem.opCode == VEX_FSH) {
      unsigned int value;
      memcpy(&value, &extoMem.floatRes, 2);
      sth(address, value);
    } else if (extoMem.opCode == VEX_FSW) {
      unsigned int value;
      memcpy(&value, &extoMem.floatRes, 1);
      stb(address, value);
    }
  } else if (extoMem.opCode.slc<3>(4) == 1) {
    // The instruction is a memory access

    if (extoMem.opCode == VEX_PROFILE) {

      if (this->profileResult[extoMem.result] != 255)
        this->profileResult[extoMem.result]++;

      memtoWB->WBena = 0;
    } else if (!extoMem.opCode[3]) {
      // The operation is a load instruction

      ac_int<16, false> const0_16 = 0;
      ac_int<16, false> const1_16 = 0xffff;
      ac_int<24, false> const0_24 = 0;
      ac_int<24, false> const1_24 = 0xffffff;
      ac_int<16, true> signed16Value;
      ac_int<8, true> signed8Value;

      ac_int<32, false> unsignedWord = 0;
      ac_int<32, true> signedWord    = 0;
      ac_int<16, false> unsignedHalf = 0;
      ac_int<16, true> signedHalf    = 0;
      ac_int<8, false> unsignedByte  = 0;
      ac_int<8, true> signedByte     = 0;

      char offset = (address << 3).slc<6>(0);

      switch (extoMem.opCode) {
        case VEX_LDD:
          memtoWB->result = extoMem.memValue;
          break;
        case VEX_LDWU:
          // ldw
          unsignedWord.set_slc(0, extoMem.memValue.slc<32>(offset));
          memtoWB->result = unsignedWord;
          break;
        case VEX_LDW:
          // ldw
          signedWord.set_slc(0, extoMem.memValue.slc<32>(offset));
          memtoWB->result = signedWord;
          break;
        case VEX_LDHU:
          // ldhuextoMem.opcode
          unsignedHalf.set_slc(0, extoMem.memValue.slc<16>(offset));
          memtoWB->result = unsignedHalf;
          break;
        case VEX_LDH:
          // ldh
          signedHalf.set_slc(0, extoMem.memValue.slc<16>(offset));
          memtoWB->result = signedHalf;
          break;
        case VEX_LDBU:
          // ldbu
          unsignedByte.set_slc(0, extoMem.memValue.slc<8>(offset));
          memtoWB->result = unsignedByte;
          break;
        case VEX_LDB:
          // ldb
          signedByte.set_slc(0, extoMem.memValue.slc<8>(offset));
          memtoWB->result = signedByte;
          break;
        default:
          break;
      }

    } else {

      memtoWB->WBena = 0; // TODO : this shouldn't be necessary : WB shouldn't be enabled before

      // We are on a store instruction
      ac_int<1, false> byteEna0 = 0, byteEna1 = 0, byteEna2 = 0, byteEna3 = 0, byteEna4 = 0, byteEna5 = 0, byteEna6 = 0,
                       byteEna7 = 0;
      ac_int<8, false> byte0, byte1, byte2, byte3, byte4, byte5, byte6, byte7;
      ac_int<7, false> opcodeToSwitch = extoMem.opCode;
      ac_int<1, false> enableStore    = 1;

      if (opcodeToSwitch > VEX_STD) {
        opcodeToSwitch -= 4;
        enableStore = this->enable;
      }

      if (enableStore) {
        switch (extoMem.opCode) {
          case VEX_STD:
            this->std(address, extoMem.datac);
            break;
          case VEX_STW:

            this->stw(address, extoMem.datac.slc<32>(0));
            break;
          case VEX_STH:
            // STH

            this->sth(address, extoMem.datac.slc<16>(0));
            break;
          case VEX_STB:

            this->stb(address, extoMem.datac.slc<8>(0));

            break;
          default:
            break;
        }
      }

#ifdef __CATAPULT

      if (byteEna0)
        memory0[address >> 2] = byte0;
      if (byteEna1)
        memory1[address >> 2] = byte1;
      if (byteEna2)
        memory2[address >> 2] = byte2;
      if (byteEna3)
        memory3[address >> 2] = byte3;
      if (byteEna4)
        memory4[address >> 2] = byte4;
      if (byteEna5)
        memory5[address >> 2] = byte5;
      if (byteEna6)
        memory6[address >> 2] = byte6;
      if (byteEna7)
        memory7[address >> 2] = byte7;

#else
      /*
                              if (byteEna0)
                                      this->stb((address & 0xfffffffffffffff8), byte0);
                              if (byteEna1)
                                      this->stb((address & 0xfffffffffffffff8)+1, byte1);
                              if (byteEna2)
                                      this->stb((address & 0xfffffffffffffff8)+2, byte2);
                              if (byteEna3)
                                      this->stb((address & 0xfffffffffffffff8)+3, byte3);
                              if (byteEna4)
                                      this->stb((address & 0xfffffffffffffff8)+4, byte4);
                              if (byteEna5)
                                      this->stb((address & 0xfffffffffffffff8)+5, byte5);
                              if (byteEna6)
                                      this->stb((address & 0xfffffffffffffff8)+6, byte6);
                              if (byteEna7)
                                      this->stb((address & 0xfffffffffffffff8)+7, byte7);
      */
#endif
    }
  } else
    memtoWB->result = extoMem.result;
}

#ifdef __CATAPULT
void doEx(struct DCtoEx dctoEx, struct ExtoMem* extoMem)
{
#else
void VexSimulator::doEx(struct DCtoEx dctoEx, struct ExtoMem* extoMem)
{
#endif

  extoMem->isFloat             = 0;
  ac_int<6, false> shiftAmount = dctoEx.datab.slc<6>(0);

  ac_int<64, true> addDataa = (dctoEx.opCode == VEX_SH1ADD)
                                  ? (dctoEx.dataa << 1)
                                  : (dctoEx.opCode == VEX_SH2ADD)
                                        ? (dctoEx.dataa << 2)
                                        : (dctoEx.opCode == VEX_SH3ADD)
                                              ? (dctoEx.dataa << 3)
                                              : (dctoEx.opCode == VEX_SH4ADD) ? (dctoEx.dataa << 4) : dctoEx.dataa;

  ac_int<64, false> addDatab = dctoEx.datab;

  ac_int<1, false> selectAdd =
      (dctoEx.opCode.slc<3>(4) == 0x1) | (dctoEx.opCode == VEX_FLW) | (dctoEx.opCode == VEX_FSW) // Memory instructions
      | (dctoEx.opCode == VEX_ADD) | (dctoEx.opCode == VEX_SH1ADD) | (dctoEx.opCode == VEX_SH2ADD) |
      (dctoEx.opCode == VEX_SH3ADD) | (dctoEx.opCode == VEX_SH4ADD) | (dctoEx.opCode == VEX_ADDi) |
      (dctoEx.opCode == VEX_SH1ADDi) | (dctoEx.opCode == VEX_SH2ADDi) | (dctoEx.opCode == VEX_SH3ADDi) |
      (dctoEx.opCode == VEX_SH4ADDi);

  ac_int<1, false> selectSub = (dctoEx.opCode == VEX_SUB) | (dctoEx.opCode == VEX_SUBi);
  ac_int<1, false> selectSll = (dctoEx.opCode == VEX_SLL) | (dctoEx.opCode == VEX_SLLi);
  ac_int<1, false> selectSrl = (dctoEx.opCode == VEX_SRL) | (dctoEx.opCode == VEX_SRLi);
  ac_int<1, false> selectSra = (dctoEx.opCode == VEX_SRA) | (dctoEx.opCode == VEX_SRAi);
  ac_int<1, false> selectAnd = (dctoEx.opCode == VEX_AND) | (dctoEx.opCode == VEX_ANDi);
  ac_int<1, false> selectOr  = (dctoEx.opCode == VEX_OR) | (dctoEx.opCode == VEX_ORi);
  ac_int<1, false> selectNot = (dctoEx.opCode == VEX_NOT) | (dctoEx.opCode == VEX_NOTi);
  ac_int<1, false> selectXor = (dctoEx.opCode == VEX_XOR) | (dctoEx.opCode == VEX_XORi);
  ac_int<1, false> selectNor = (dctoEx.opCode == VEX_NOR) | (dctoEx.opCode == VEX_NORi);
  ac_int<1, false> selectCmp =
      (dctoEx.opCode == VEX_CMPLT) | (dctoEx.opCode == VEX_CMPLTi) | (dctoEx.opCode == VEX_CMPLTU) |
      (dctoEx.opCode == VEX_CMPLTUi) | (dctoEx.opCode == VEX_CMPNE) | (dctoEx.opCode == VEX_CMPNEi) |
      (dctoEx.opCode == VEX_CMPEQ) | (dctoEx.opCode == VEX_CMPEQi) | (dctoEx.opCode == VEX_CMPGE) |
      (dctoEx.opCode == VEX_CMPGEi) | (dctoEx.opCode == VEX_CMPGEU) | (dctoEx.opCode == VEX_CMPGEUi) |
      (dctoEx.opCode == VEX_CMPGT) | (dctoEx.opCode == VEX_CMPGTi) | (dctoEx.opCode == VEX_CMPGTU) |
      (dctoEx.opCode == VEX_CMPGTUi) | (dctoEx.opCode == VEX_CMPLE) | (dctoEx.opCode == VEX_CMPLEi) |
      (dctoEx.opCode == VEX_CMPLEU) | (dctoEx.opCode == VEX_CMPLEUi);

  ac_int<64, false> unsigned_dataa = dctoEx.dataa;
  ac_int<64, false> unsigned_datab = dctoEx.datab;

  ac_int<64, true> add_result = addDataa + addDatab;
  ac_int<64, true> sub_result = dctoEx.dataa - dctoEx.datab;
  ac_int<64, true> sl_result  = dctoEx.dataa << shiftAmount;
  ac_int<64, true> srl_result = unsigned_dataa >> shiftAmount;
  ac_int<64, true> sra_result = dctoEx.dataa >> shiftAmount;

  ac_int<64, true> and_result = dctoEx.dataa & dctoEx.datab;
  ac_int<64, true> or_result  = dctoEx.dataa | dctoEx.datab;
  ac_int<64, true> not_result = ~dctoEx.dataa;
  ac_int<64, true> xor_result = dctoEx.dataa ^ dctoEx.datab;
  ac_int<64, true> nor_result = ~(dctoEx.dataa | dctoEx.datab);

  ac_int<64, true> unsigned_sub_result = unsigned_dataa - unsigned_datab;

  ac_int<1, false> cmpResult_1 =
      ((dctoEx.opCode == VEX_CMPLT) | (dctoEx.opCode == VEX_CMPLTi))
          ? sub_result < 0
          : ((dctoEx.opCode == VEX_CMPLTU) | (dctoEx.opCode == VEX_CMPLTUi))
                ? unsigned_dataa < unsigned_datab
                : ((dctoEx.opCode == VEX_CMPNE) | (dctoEx.opCode == VEX_CMPNEi))
                      ? sub_result != 0
                      : ((dctoEx.opCode == VEX_CMPEQ) | (dctoEx.opCode == VEX_CMPEQi))
                            ? sub_result == 0
                            : ((dctoEx.opCode == VEX_CMPGE) | (dctoEx.opCode == VEX_CMPGEi))
                                  ? dctoEx.dataa >= dctoEx.datab
                                  : ((dctoEx.opCode == VEX_CMPGEU) | (dctoEx.opCode == VEX_CMPGEUi))
                                        ? unsigned_dataa >= unsigned_datab
                                        : ((dctoEx.opCode == VEX_CMPGT) | (dctoEx.opCode == VEX_CMPGTi))
                                              ? sub_result > 0
                                              : ((dctoEx.opCode == VEX_CMPGTU) | (dctoEx.opCode == VEX_CMPGTUi))
                                                    ? unsigned_sub_result > 0
                                                    : ((dctoEx.opCode == VEX_CMPLE) | (dctoEx.opCode == VEX_CMPLEi))
                                                          ? sub_result <= 0
                                                          : sub_result <= 0;

  ac_int<64, true> cmpResult = cmpResult_1;

  ac_int<1, false> selectAdd32 = (dctoEx.opCode == VEX_ADDW) | (dctoEx.opCode == VEX_ADDWi);
  ac_int<1, false> selectSub32 = (dctoEx.opCode == VEX_SUBW);
  ac_int<1, false> selectSll32 = (dctoEx.opCode == VEX_SLLW) | (dctoEx.opCode == VEX_SLLWi);
  ac_int<1, false> selectSrl32 = (dctoEx.opCode == VEX_SRLW) | (dctoEx.opCode == VEX_SRLWi);
  ac_int<1, false> selectSra32 = (dctoEx.opCode == VEX_SRAW) | (dctoEx.opCode == VEX_SRAWi);
  ac_int<1, false> select32    = selectSub32 || selectAdd32 || selectSll32 || selectSrl32 || selectSra32;

  ac_int<32, true> dataa32           = dctoEx.dataa.slc<32>(0);
  ac_int<32, false> unsigned_dataa32 = dctoEx.dataa.slc<32>(0);
  ac_int<32, true> datab32           = dctoEx.datab.slc<32>(0);

  ac_int<5, false> shiftAmount32 = datab32.slc<5>(0);
  ac_int<32, true> add_result_32 = dataa32 + datab32;
  ac_int<32, true> sub_result_32 = dataa32 - datab32;
  ac_int<32, true> sl_result_32  = dataa32 << shiftAmount32;
  ac_int<32, true> srl_result_32 = unsigned_dataa32 >> shiftAmount32;
  ac_int<32, true> sra_result_32 = dataa32 >> shiftAmount32;

  ac_int<1, false> selectMovui = (dctoEx.opCode == VEX_MOVUI);

  ac_int<64, true> result32 =
      selectAdd32
          ? add_result_32
          : selectSub32 ? sub_result_32 : selectSll32 ? sl_result_32 : selectSra32 ? sra_result_32 : srl_result_32;

  extoMem->result =
      selectAdd
          ? add_result
          : selectSub
                ? sub_result
                : selectSll
                      ? sl_result
                      : selectSra
                            ? sra_result
                            : selectSrl
                                  ? srl_result
                                  : selectAnd
                                        ? and_result
                                        : selectOr ? or_result
                                                   : selectNot
                                                         ? not_result
                                                         : selectXor
                                                               ? xor_result
                                                               : selectNor
                                                                     ? nor_result
                                                                     : selectCmp
                                                                           ? cmpResult
                                                                           : select32 ? result32
                                                                                      : selectMovui ? dctoEx.dataa << 12
                                                                                                    : dctoEx.dataa;

  extoMem->WBena = !(dctoEx.opCode == VEX_NOP); // TODO
  if ((dctoEx.opCode == VEX_SETc & !dctoEx.datab) || (dctoEx.opCode == VEX_SETFc & dctoEx.datab))
    extoMem->WBena = 0;

  extoMem->datac      = dctoEx.datac;
  extoMem->dest       = dctoEx.dest;
  extoMem->opCode     = dctoEx.opCode;
  extoMem->memValue   = dctoEx.memValue;
  extoMem->isSpec     = dctoEx.isSpec;
  extoMem->funct      = dctoEx.funct;
  extoMem->pc         = dctoEx.pc;
  extoMem->isRollback = dctoEx.isRollback;

  if (dctoEx.opCode == VEX_FLB || dctoEx.opCode == VEX_FLH || dctoEx.opCode == VEX_FLW) {
    extoMem->isFloat = 1;
  } else if (dctoEx.opCode == VEX_FSB || dctoEx.opCode == VEX_FSH || dctoEx.opCode == VEX_FSW) {
    extoMem->floatRes = dctoEx.floatValueB;
  }
}

#ifdef __CATAPULT
void doExMult(struct DCtoEx dctoEx, struct ExtoMem* extoMem)
{
#else
void VexSimulator::doExMult(struct DCtoEx dctoEx, struct ExtoMem* extoMem)
{
#endif
  extoMem->isFloat             = 0;
  ac_int<6, false> shiftAmount = dctoEx.datab.slc<6>(0);

  ac_int<64, true> addDataa = (dctoEx.opCode == VEX_SH1ADD)
                                  ? (dctoEx.dataa << 1)
                                  : (dctoEx.opCode == VEX_SH2ADD)
                                        ? (dctoEx.dataa << 2)
                                        : (dctoEx.opCode == VEX_SH3ADD)
                                              ? (dctoEx.dataa << 3)
                                              : (dctoEx.opCode == VEX_SH4ADD) ? (dctoEx.dataa << 4) : dctoEx.dataa;

  ac_int<64, false> addDatab = dctoEx.datab;

  ac_int<1, false> selectAdd =
      (dctoEx.opCode.slc<3>(4) == 0x1) | (dctoEx.opCode == VEX_FLW) | (dctoEx.opCode == VEX_FSW) // Memory instructions
      | (dctoEx.opCode == VEX_ADD) | (dctoEx.opCode == VEX_SH1ADD) | (dctoEx.opCode == VEX_SH2ADD) |
      (dctoEx.opCode == VEX_SH3ADD) | (dctoEx.opCode == VEX_SH4ADD) | (dctoEx.opCode == VEX_ADDi) |
      (dctoEx.opCode == VEX_SH1ADDi) | (dctoEx.opCode == VEX_SH2ADDi) | (dctoEx.opCode == VEX_SH3ADDi) |
      (dctoEx.opCode == VEX_SH4ADDi);

  ac_int<1, false> selectSub = (dctoEx.opCode == VEX_SUB) | (dctoEx.opCode == VEX_SUBi);
  ac_int<1, false> selectSll = (dctoEx.opCode == VEX_SLL) | (dctoEx.opCode == VEX_SLLi);
  ac_int<1, false> selectSrl = (dctoEx.opCode == VEX_SRL) | (dctoEx.opCode == VEX_SRLi);
  ac_int<1, false> selectSra = (dctoEx.opCode == VEX_SRA) | (dctoEx.opCode == VEX_SRAi);
  ac_int<1, false> selectAnd = (dctoEx.opCode == VEX_AND) | (dctoEx.opCode == VEX_ANDi);
  ac_int<1, false> selectOr  = (dctoEx.opCode == VEX_OR) | (dctoEx.opCode == VEX_ORi);
  ac_int<1, false> selectNot = (dctoEx.opCode == VEX_NOT) | (dctoEx.opCode == VEX_NOTi);
  ac_int<1, false> selectXor = (dctoEx.opCode == VEX_XOR) | (dctoEx.opCode == VEX_XORi);
  ac_int<1, false> selectNor = (dctoEx.opCode == VEX_NOR) | (dctoEx.opCode == VEX_NORi);
  ac_int<1, false> selectCmp =
      (dctoEx.opCode == VEX_CMPLT) | (dctoEx.opCode == VEX_CMPLTi) | (dctoEx.opCode == VEX_CMPLTU) |
      (dctoEx.opCode == VEX_CMPLTUi) | (dctoEx.opCode == VEX_CMPNE) | (dctoEx.opCode == VEX_CMPNEi) |
      (dctoEx.opCode == VEX_CMPEQ) | (dctoEx.opCode == VEX_CMPEQi) | (dctoEx.opCode == VEX_CMPGE) |
      (dctoEx.opCode == VEX_CMPGEi) | (dctoEx.opCode == VEX_CMPGEU) | (dctoEx.opCode == VEX_CMPGEUi) |
      (dctoEx.opCode == VEX_CMPGT) | (dctoEx.opCode == VEX_CMPGTi) | (dctoEx.opCode == VEX_CMPGTU) |
      (dctoEx.opCode == VEX_CMPGTUi) | (dctoEx.opCode == VEX_CMPLE) | (dctoEx.opCode == VEX_CMPLEi) |
      (dctoEx.opCode == VEX_CMPLEU) | (dctoEx.opCode == VEX_CMPLEUi);

  ac_int<64, false> unsigned_dataa = dctoEx.dataa;
  ac_int<64, false> unsigned_datab = dctoEx.datab;

  ac_int<64, true> add_result = addDataa + addDatab;
  ac_int<64, true> sub_result = dctoEx.dataa - dctoEx.datab;
  ac_int<64, true> sl_result  = dctoEx.dataa << shiftAmount;
  ac_int<64, true> srl_result = unsigned_dataa >> shiftAmount;
  ac_int<64, true> sra_result = dctoEx.dataa >> shiftAmount;

  ac_int<64, true> and_result = dctoEx.dataa & dctoEx.datab;
  ac_int<64, true> or_result  = dctoEx.dataa | dctoEx.datab;
  ac_int<64, true> not_result = ~dctoEx.dataa;
  ac_int<64, true> xor_result = dctoEx.dataa ^ dctoEx.datab;
  ac_int<64, true> nor_result = ~(dctoEx.dataa | dctoEx.datab);

  ac_int<128, true> mul_result  = dctoEx.dataa * dctoEx.datab;
  ac_int<64, true> mullo_result = mul_result.slc<64>(0);
  ac_int<64, true> mulhi_result = mul_result.slc<64>(64);

  ac_int<128, false> mulu_result = unsigned_dataa * unsigned_datab;
  ac_int<64, true> mulhiu_result = mulu_result.slc<64>(64);

  ac_int<128, false> mulsu_result = dctoEx.dataa * unsigned_datab;
  ac_int<64, true> mulhisu_result = mulsu_result.slc<64>(64);

  ac_int<65, true> const0   = 0;
  ac_int<64, false> constu0 = 0;

#ifdef __CATAPULT
  ac_int<64, true> div_result  = 0; // Currently catapult version do not do division
  ac_int<64, true> remu_result = 0;
#else
  ac_int<64, true> div_result = !dctoEx.datab ? const0 : dctoEx.dataa / dctoEx.datab;
  ac_int<64, true> rem_result =
      !dctoEx.datab ? dctoEx.datab : dctoEx.datab < 0 ? dctoEx.dataa % -dctoEx.datab : dctoEx.dataa % dctoEx.datab;
  ac_int<64, true> divu_result    = !unsigned_datab ? constu0 : unsigned_dataa / unsigned_datab;
  ac_int<64, true> remu_result    = !unsigned_datab ? unsigned_datab : unsigned_dataa % unsigned_datab;
#endif
  ac_int<64, true> unsigned_sub_result = unsigned_dataa - unsigned_datab;

  ac_int<1, false> cmpResult_1 =
      ((dctoEx.opCode == VEX_CMPLT) | (dctoEx.opCode == VEX_CMPLTi))
          ? sub_result < 0
          : ((dctoEx.opCode == VEX_CMPLTU) | (dctoEx.opCode == VEX_CMPLTUi))
                ? unsigned_dataa < unsigned_datab
                : ((dctoEx.opCode == VEX_CMPNE) | (dctoEx.opCode == VEX_CMPNEi))
                      ? sub_result != 0
                      : ((dctoEx.opCode == VEX_CMPEQ) | (dctoEx.opCode == VEX_CMPEQi))
                            ? sub_result == 0
                            : ((dctoEx.opCode == VEX_CMPGE) | (dctoEx.opCode == VEX_CMPGEi))
                                  ? sub_result >= 0
                                  : ((dctoEx.opCode == VEX_CMPGEU) | (dctoEx.opCode == VEX_CMPGEUi))
                                        ? unsigned_dataa >= unsigned_datab
                                        : ((dctoEx.opCode == VEX_CMPGT) | (dctoEx.opCode == VEX_CMPGTi))
                                              ? sub_result > 0
                                              : ((dctoEx.opCode == VEX_CMPGTU) | (dctoEx.opCode == VEX_CMPGTUi))
                                                    ? unsigned_sub_result > 0
                                                    : ((dctoEx.opCode == VEX_CMPLE) | (dctoEx.opCode == VEX_CMPLEi))
                                                          ? sub_result <= 0
                                                          : sub_result <= 0;

  ac_int<64, true> cmpResult = cmpResult_1;

  ac_int<1, false> selectAdd32  = (dctoEx.opCode == VEX_ADDW) | (dctoEx.opCode == VEX_ADDWi);
  ac_int<1, false> selectSub32  = (dctoEx.opCode == VEX_SUBW);
  ac_int<1, false> selectSll32  = (dctoEx.opCode == VEX_SLLW) | (dctoEx.opCode == VEX_SLLWi);
  ac_int<1, false> selectSrl32  = (dctoEx.opCode == VEX_SRLW) | (dctoEx.opCode == VEX_SRLWi);
  ac_int<1, false> selectSra32  = (dctoEx.opCode == VEX_SRAW) | (dctoEx.opCode == VEX_SRAWi);
  ac_int<1, false> selectMult32 = (dctoEx.opCode == VEX_MPYW);
  ac_int<1, false> selectDiv32  = (dctoEx.opCode == VEX_DIVW);
  ac_int<1, false> selectRem32  = (dctoEx.opCode == VEX_REMW);
  ac_int<1, false> selectDivu32 = (dctoEx.opCode == VEX_DIVUW);
  ac_int<1, false> selectRemu32 = (dctoEx.opCode == VEX_REMUW);

  ac_int<1, false> select32 = selectSub32 || selectAdd32 || selectSll32 || selectSrl32 || selectSra32 || selectMult32 ||
                              selectDiv32 || selectDivu32 || selectRem32 || selectRemu32;

  ac_int<32, true> dataa32           = dctoEx.dataa.slc<32>(0);
  ac_int<32, false> unsigned_dataa32 = dctoEx.dataa.slc<32>(0);
  ac_int<32, false> unsigned_datab32 = dctoEx.datab.slc<32>(0);
  ac_int<32, true> datab32           = dctoEx.datab.slc<32>(0);
  ac_int<33, true> const0_32         = 0;
  ac_int<32, false> constu0_32       = 0;

  ac_int<5, false> shiftAmount32  = datab32.slc<5>(0);
  ac_int<32, true> add_result_32  = dataa32 + datab32;
  ac_int<32, true> sub_result_32  = dataa32 - datab32;
  ac_int<32, true> sl_result_32   = dataa32 << shiftAmount32;
  ac_int<32, true> srl_result_32  = unsigned_dataa32 >> shiftAmount32;
  ac_int<32, true> sra_result_32  = dataa32 >> shiftAmount32;
  ac_int<32, true> mult_result_32 = dataa32 * datab32;
  ac_int<32, true> const1         = 1;
#ifdef __CATAPULT
  ac_int<32, true> div_result_32  = 0; // Currently catapult version do not do division
  ac_int<32, true> remu_result_32 = 0;
#else
  ac_int<1, false> overflow       = dataa32 == 0x80000000 & datab32 == -1;
  ac_int<1, false> divideByZero   = datab32 == 0;
  ac_int<33, true> minValue       = 0x80000000;
  ac_int<32, false> minValueU     = 0x80000000;
  ac_int<33, true> minusOne       = -1;
  ac_int<32, true> zero           = 0;
  ac_int<32, true> div_result_32  = 0;
  ac_int<32, true> rem_result_32  = 0;
  ac_int<32, true> divu_result_32 = 0;
  ac_int<32, true> remu_result_32 = 0;

  if (selectDiv32 || selectDivu32 || selectRem32 || selectRemu32) {

    div_result_32  = divideByZero ? minusOne : overflow ? minValue : dataa32 / datab32;
    rem_result_32  = divideByZero ? (dataa32 % 1) : overflow ? zero : dataa32 % datab32;
    divu_result_32 = divideByZero ? minValueU : overflow ? minValueU : unsigned_dataa32 / unsigned_datab32;
    remu_result_32 = divideByZero ? unsigned_dataa32 : overflow ? minValueU : unsigned_dataa32 % unsigned_datab32;
  }
#endif

  ac_int<64, true> result32 =
      selectAdd32
          ? add_result_32
          : selectSub32
                ? sub_result_32
                : selectSll32
                      ? sl_result_32
                      : selectSra32
                            ? sra_result_32
                            : selectMult32 ? mult_result_32
                                           : selectDiv32 ? div_result_32
                                                         : selectRem32 ? rem_result_32
                                                                       : selectDivu32 ? divu_result_32
                                                                                      : selectRemu32 ? remu_result_32
                                                                                                     : srl_result_32;

  ac_int<1, false> selectMovui = (dctoEx.opCode == VEX_MOVUI);

  extoMem->result =
      selectAdd
          ? add_result
          : selectSub
                ? sub_result
                : selectSll
                      ? sl_result
                      : selectSra
                            ? sra_result
                            : selectSrl
                                  ? srl_result
                                  : selectAnd
                                        ? and_result
                                        : selectOr
                                              ? or_result
                                              : selectNot
                                                    ? not_result
                                                    : selectXor
                                                          ? xor_result
                                                          : selectNor
                                                                ? nor_result
                                                                : selectCmp
                                                                      ? cmpResult
                                                                      : (dctoEx.opCode == VEX_MPY)
                                                                            ? mullo_result
                                                                            : (dctoEx.opCode == VEX_MPYH)
                                                                                  ? mulhi_result
                                                                                  : (dctoEx.opCode == VEX_MPYHSU)
                                                                                        ? mulhisu_result
                                                                                        : (dctoEx.opCode == VEX_MPYHU)
                                                                                              ? mulhiu_result
                                                                                              : (dctoEx.opCode ==
                                                                                                 VEX_REM)
                                                                                                    ? rem_result
                                                                                                    : (dctoEx.opCode ==
                                                                                                       VEX_DIV)
                                                                                                          ? div_result
                                                                                                          : (dctoEx
                                                                                                                 .opCode ==
                                                                                                             VEX_REMU)
                                                                                                                ? remu_result
                                                                                                                : (dctoEx
                                                                                                                       .opCode ==
                                                                                                                   VEX_DIVU)
                                                                                                                      ? divu_result
                                                                                                                      : select32
                                                                                                                            ? result32
                                                                                                                            : selectMovui
                                                                                                                                  ? dctoEx.dataa
                                                                                                                                        << 12
                                                                                                                                  : dctoEx
                                                                                                                                        .dataa;

  extoMem->WBena = !(dctoEx.opCode == VEX_NOP); // TODO
  if ((dctoEx.opCode == VEX_SETc & !dctoEx.datab) || (dctoEx.opCode == VEX_SETFc & dctoEx.datab))
    extoMem->WBena = 0;

  extoMem->datac      = dctoEx.datac;
  extoMem->dest       = dctoEx.dest;
  extoMem->opCode     = dctoEx.opCode;
  extoMem->memValue   = dctoEx.memValue;
  extoMem->isSpec     = dctoEx.isSpec;
  extoMem->funct      = dctoEx.funct;
  extoMem->pc         = dctoEx.pc;
  extoMem->isRollback = dctoEx.isRollback;

  extoMem->isFloat = 0;
  float localFloat;

  if (dctoEx.opCode == VEX_FLB || dctoEx.opCode == VEX_FLH || dctoEx.opCode == VEX_FLW) {
    extoMem->isFloat = 1;
  } else if (dctoEx.opCode == VEX_FSB || dctoEx.opCode == VEX_FSH || dctoEx.opCode == VEX_FSW) {
    extoMem->floatRes = dctoEx.floatValueB;
  } else if (dctoEx.opCode == VEX_FMADD) {
    extoMem->isFloat  = 1;
    extoMem->floatRes = dctoEx.floatValueA * dctoEx.floatValueB + dctoEx.floatValueC;
  } else if (dctoEx.opCode == VEX_FNMADD) {
    extoMem->isFloat  = 1;
    extoMem->floatRes = -dctoEx.floatValueA * dctoEx.floatValueB - dctoEx.floatValueC;
  } else if (dctoEx.opCode == VEX_FMSUB) {
    extoMem->isFloat  = 1;
    extoMem->floatRes = dctoEx.floatValueA * dctoEx.floatValueB - dctoEx.floatValueC;
  } else if (dctoEx.opCode == VEX_FNMSUB) {
    extoMem->isFloat  = 1;
    extoMem->floatRes = -dctoEx.floatValueA * dctoEx.floatValueB + dctoEx.floatValueC;
  } else if (dctoEx.opCode == VEX_FP) {
    extoMem->isFloat = 1;
    unsigned int localint;
    switch (dctoEx.funct) {
      case VEX_FP_FADD:
        extoMem->floatRes = dctoEx.floatValueA + dctoEx.floatValueB;
        break;
      case VEX_FP_FSUB:
        extoMem->floatRes = dctoEx.floatValueA - dctoEx.floatValueB;
        break;
      case VEX_FP_FMUL:
        extoMem->floatRes = dctoEx.floatValueA * dctoEx.floatValueB;
        break;
      case VEX_FP_FDIV:
        extoMem->floatRes = dctoEx.floatValueA / dctoEx.floatValueB;
        break;
      case VEX_FP_FSQRT:
        extoMem->floatRes = sqrt(dctoEx.floatValueA);
        break;
      case VEX_FP_FSGNJ:
        localFloat = std::fabs(dctoEx.floatValueA);

        if (dctoEx.floatValueB < 0)
          extoMem->floatRes = -localFloat;
        else
          extoMem->floatRes = localFloat;
        break;
      case VEX_FP_FSGNJN:
        localFloat = std::fabs(dctoEx.floatValueA);
        if (dctoEx.floatValueB >= 0)
          extoMem->floatRes = -localFloat;
        else
          extoMem->floatRes = localFloat;
        break;
      case VEX_FP_FSGNJNX:
        localFloat = std::fabs(dctoEx.floatValueA);
        if ((dctoEx.floatValueB < 0 && dctoEx.floatValueA >= 0) || (dctoEx.floatValueB >= 0 && dctoEx.floatValueA < 0))
          extoMem->floatRes = -localFloat;
        else
          extoMem->floatRes = localFloat;
        break;
      case VEX_FP_FMAX:
        if (dctoEx.floatValueA > dctoEx.floatValueB)
          extoMem->floatRes = dctoEx.floatValueA;
        else
          extoMem->floatRes = dctoEx.floatValueB;
        break;
      case VEX_FP_FMIN:
        if (dctoEx.floatValueA < dctoEx.floatValueB)
          extoMem->floatRes = dctoEx.floatValueA;
        else
          extoMem->floatRes = dctoEx.floatValueB;
        break;
      case VEX_FP_FCVTWS:
        extoMem->result  = dctoEx.floatValueA;
        extoMem->isFloat = 0;

        break;
      case VEX_FP_FCVTWUS:
        extoMem->result  = (unsigned int)dctoEx.floatValueA;
        extoMem->isFloat = 0;

        break;
      case VEX_FP_FMVXW:

        memcpy(&localint, &(dctoEx.floatValueA), 4);
        extoMem->result  = localint;
        extoMem->isFloat = 0;

        break;
      case VEX_FP_FCLASS:
        exit(-1);
        break;
      case VEX_FP_FEQ:
        extoMem->isFloat = 0;
        extoMem->result  = dctoEx.floatValueA == dctoEx.floatValueB;
        break;

      case VEX_FP_FLE:
        extoMem->isFloat = 0;
        extoMem->result  = dctoEx.floatValueA <= dctoEx.floatValueB;
        break;
      case VEX_FP_FLT:
        extoMem->isFloat = 0;
        extoMem->result  = dctoEx.floatValueA < dctoEx.floatValueB;
        break;
      case VEX_FP_FCVTSW:
        extoMem->floatRes = dctoEx.dataa;
        break;
      case VEX_FP_FCVTSWU:
        extoMem->floatRes = (unsigned int)dctoEx.dataa;
        break;
      case VEX_FP_FMVWX:
        localint = dctoEx.dataa;
        memcpy(&(extoMem->floatRes), &localint, 4);
        break;
    }
  }
}

#ifdef __CATAPULT
void doDC(struct FtoDC ftoDC, struct DCtoEx* dctoEx)
{
#else
void VexSimulator::doDC(struct FtoDC ftoDC, struct DCtoEx* dctoEx)
{
#endif

  ac_int<6, false> RA    = ftoDC.instruction.slc<6>(26);
  ac_int<6, false> RB    = ftoDC.instruction.slc<6>(20);
  ac_int<6, false> RC    = ftoDC.instruction.slc<6>(14);
  ac_int<6, false> RD    = ftoDC.instruction.slc<6>(8);
  ac_int<5, false> funct = ftoDC.instruction.slc<5>(7);

  ac_int<19, false> IMM19_u = ftoDC.instruction.slc<19>(7);
  ac_int<19, true> IMM19_s  = ftoDC.instruction.slc<19>(7);
  ac_int<13, false> IMM13_u = ftoDC.instruction.slc<13>(7);
  ac_int<13, true> IMM13_s  = ftoDC.instruction.slc<13>(7);
  ac_int<9, false> IMM9_u   = ftoDC.instruction.slc<9>(11);
  ac_int<9, true> IMM9_s    = ftoDC.instruction.slc<9>(11);

  ac_int<7, false> OP   = ftoDC.instruction.slc<7>(0);
  ac_int<3, false> BEXT = ftoDC.instruction.slc<3>(8);

  ac_int<1, false> isIType = (OP.slc<3>(4) == 2);
  ac_int<1, false> isImm   = OP.slc<3>(4) == 1 || OP.slc<3>(4) == 6 || OP.slc<3>(4) == 7 || OP == VEX_FLW ||
                           OP == VEX_FLH || OP == VEX_FLB || OP == VEX_FSW || OP == VEX_FSH || OP == VEX_FSB;

  ac_int<1, false> isUnsigned = (OP == VEX_CMPLTU) | (OP == VEX_CMPLTUi) | (OP == VEX_CMPGEU) | (OP == VEX_CMPGEUi) |
                                (OP == VEX_CMPGTU) | (OP == VEX_CMPGTUi) | (OP == VEX_CMPLEU) | (OP == VEX_CMPLEUi);

  ac_int<23, false> const0_23 = 0;
  ac_int<23, false> const1_23 = 0x7fffff;
  ac_int<19, false> const0_19 = 0;
  ac_int<19, false> const1_19 = 0x7ffff;
  ac_int<13, false> const0_13 = 0;
  ac_int<13, false> const1_13 = 0x1fff;

  ac_int<6, false> secondRegAccess = RB;

  ac_int<64, true> regValueA = REG[RA];
  ac_int<64, true> regValueB = REG[secondRegAccess];

  dctoEx->floatValueA = regf[RA];
  dctoEx->floatValueB = regf[RB];
  dctoEx->floatValueC = regf[RC];
  dctoEx->funct       = funct;
  dctoEx->pc          = ftoDC.pc;
  dctoEx->isRollback  = ftoDC.isRollback;

  dctoEx->opCode = OP;
  dctoEx->datac  = regValueB; // For store instructions

  if (isIType) {
    // The instruction is I type
    dctoEx->dest = RA;

    dctoEx->dataa = IMM19_s;

  } else {
    // The instruction is R type
    dctoEx->dataa = regValueA;

    if (isImm) {
      dctoEx->dest = RB;
      if (isUnsigned)
        dctoEx->datab = IMM13_u;
      else
        dctoEx->datab = IMM13_s;

    } else {
      dctoEx->dest  = RC;
      dctoEx->datab = regValueB;
    }
  }

  if (OP == VEX_FNMADD || OP == VEX_FMADD || OP == VEX_FNMSUB || OP == VEX_FMSUB) {
    dctoEx->dest = RD;
  }

  if (OP == VEX_SYSTEM && (IMM19_s & 0xf) == VEX_SYSTEM_CSRRS) {
    dctoEx->opCode = VEX_MOVI;
    dctoEx->dataa  = cycle;
  }
}

#ifdef __CATAPULT
void doDCMem(struct FtoDC ftoDC, struct DCtoEx* dctoEx, ac_int<8, false> memory0[65536],
             ac_int<8, false> memory1[65536], ac_int<8, false> memory2[65536], ac_int<8, false> memory3[65536],
             ac_int<8, false> memory4[65536], ac_int<8, false> memory5[65536], ac_int<8, false> memory6[65536],
             ac_int<8, false> memory7[65536])
{
#else
void VexSimulator::doDCMem(struct FtoDC ftoDC, struct DCtoEx* dctoEx)
{
#endif

  ac_int<6, false> RA    = ftoDC.instruction.slc<6>(26);
  ac_int<6, false> RB    = ftoDC.instruction.slc<6>(20);
  ac_int<6, false> RC    = ftoDC.instruction.slc<6>(14);
  ac_int<6, false> RD    = ftoDC.instruction.slc<6>(8);
  ac_int<5, false> funct = ftoDC.instruction.slc<5>(7);

  ac_int<19, false> IMM19_u = ftoDC.instruction.slc<19>(7);
  ac_int<19, true> IMM19_s  = ftoDC.instruction.slc<19>(7);
  ac_int<13, false> IMM13_u = ftoDC.instruction.slc<13>(7);
  ac_int<13, true> IMM13_s  = ftoDC.instruction.slc<13>(7);
  ac_int<12, false> IMM12_u = ftoDC.instruction.slc<12>(8);
  ac_int<12, true> IMM12_s  = ftoDC.instruction.slc<12>(8);
  ac_int<7, true> IMM7_s    = ftoDC.instruction.slc<7>(13);

  ac_int<9, false> IMM9_u = ftoDC.instruction.slc<9>(11);
  ac_int<9, true> IMM9_s  = ftoDC.instruction.slc<9>(11);

  ac_int<7, false> OP   = ftoDC.instruction.slc<7>(0);
  ac_int<3, false> BEXT = ftoDC.instruction.slc<3>(8);

  ac_int<1, false> isIType = (OP.slc<3>(4) == 2);
  ac_int<1, false> isImm   = OP.slc<3>(4) == 1 || OP.slc<3>(4) == 6 || OP.slc<3>(4) == 7 || OP == VEX_FLW ||
                           OP == VEX_FLH || OP == VEX_FLB || OP == VEX_FSW || OP == VEX_FSH || OP == VEX_FSB;

  ac_int<1, false> isUnsigned = (OP == VEX_CMPLTU) | (OP == VEX_CMPLTUi) | (OP == VEX_CMPGEU) | (OP == VEX_CMPGEUi) |
                                (OP == VEX_CMPGTU) | (OP == VEX_CMPGTUi) | (OP == VEX_CMPLEU) | (OP == VEX_CMPLEUi);

  ac_int<23, false> const0_23 = 0;
  ac_int<23, false> const1_23 = 0x7fffff;
  ac_int<19, false> const0_19 = 0;
  ac_int<19, false> const1_19 = 0x7ffff;
  ac_int<13, false> const0_13 = 0;
  ac_int<13, false> const1_13 = 0x1fff;

  ac_int<6, false> secondRegAccess = RB;

  ac_int<64, true> regValueA = REG[RA];
  ac_int<64, true> regValueB = REG[secondRegAccess];

  dctoEx->floatValueA = regf[RA];
  dctoEx->floatValueB = regf[RB];
  dctoEx->floatValueC = regf[RC];
  dctoEx->funct       = funct;
  dctoEx->pc          = ftoDC.pc;
  dctoEx->isRollback  = ftoDC.isRollback;

  dctoEx->floatRes = regf[secondRegAccess];

  dctoEx->opCode = OP;
  dctoEx->datac  = regValueB; // For store instructions

  if (isIType) {
    // The instruction is I type
    dctoEx->dest = RA;

    dctoEx->dataa = IMM19_s;

  } else {
    // The instruction is R type
    dctoEx->dataa = regValueA;

    if (isImm) {
      dctoEx->dest = RB;
      if ((OP >> 4) == 0x1) {
        dctoEx->isSpec = ftoDC.instruction[7];
        if (!ftoDC.instruction[7]) {
          dctoEx->datab = IMM12_s;
        } else {
          dctoEx->funct = IMM12_u.slc<5>(0);
          dctoEx->datab = IMM7_s;
        }
      } else {
        if (isUnsigned)
          dctoEx->datab = IMM13_u;
        else
          dctoEx->datab = IMM13_s;
      }
    } else {
      dctoEx->dest  = RC;
      dctoEx->datab = regValueB;
    }
  }

  if (OP == VEX_FNMADD || OP == VEX_FMADD || OP == VEX_FNMSUB || OP == VEX_FMSUB) {
    dctoEx->dest = RD;
  }
  ac_int<64, false> address;
  if (ftoDC.instruction[7]) {
    address = IMM7_s + regValueA;
  } else
    address = IMM12_s + regValueA;

  if ((OP.slc<4>(3) == (VEX_LDD >> 3) || OP == VEX_FLW || OP == VEX_FLH || OP == VEX_FLB) && OP != VEX_PROFILE &&
      (address != 0x10009000 || !OP[3])) {
// If we are in a memory access, the access is initiated here
#ifdef __CATAPULT
    dctoEx->memValue =
        ldd(address & 0xfffffffffffffff8, memory0, memory1, memory2, memory3, memory4, memory5, memory6, memory7);
#else
    dctoEx->memValue = this->ldd(address & 0xfffffffffffffff8);

#endif
  }

  if (OP == VEX_SYSTEM && (IMM19_s & 0xf) == VEX_SYSTEM_CSRRS) {
    dctoEx->opCode = VEX_MOVI;
    dctoEx->dataa  = cycle;
  }
}

#ifdef __CATAPULT
void doDCBr(struct FtoDC ftoDC, struct DCtoEx* dctoEx)
{
#else
void VexSimulator::doDCBr(struct FtoDC ftoDC, struct DCtoEx* dctoEx)
{
#endif
  ac_int<6, false> RA = ftoDC.instruction.slc<6>(26);
  ac_int<6, false> RB = ftoDC.instruction.slc<6>(20);
  ac_int<6, false> RC = ftoDC.instruction.slc<6>(14);
  ac_int<6, false> RD = ftoDC.instruction.slc<6>(8);

  ac_int<5, false> funct    = ftoDC.instruction.slc<5>(7);
  ac_int<19, false> IMM19_u = ftoDC.instruction.slc<19>(7);
  ac_int<19, true> IMM19_s  = ftoDC.instruction.slc<19>(7);
  ac_int<13, false> IMM13_u = ftoDC.instruction.slc<13>(7);
  ac_int<13, true> IMM13_s  = ftoDC.instruction.slc<13>(7);
  ac_int<9, false> IMM9_u   = ftoDC.instruction.slc<9>(11);
  ac_int<9, true> IMM9_s    = ftoDC.instruction.slc<9>(11);

  ac_int<7, false> OP   = ftoDC.instruction.slc<7>(0);
  ac_int<3, false> BEXT = ftoDC.instruction.slc<3>(8);

  ac_int<1, false> isIType = (OP.slc<3>(4) == 2);
  ac_int<1, false> isImm   = OP.slc<3>(4) == 1 || OP.slc<3>(4) == 6 || OP.slc<3>(4) == 7 || OP == VEX_FLW ||
                           OP == VEX_FLH || OP == VEX_FLB || OP == VEX_FSW || OP == VEX_FSH || OP == VEX_FSB;

  ac_int<1, false> isUnsigned = (OP == VEX_CMPLTU) | (OP == VEX_CMPLTUi) | (OP == VEX_CMPGEU) | (OP == VEX_CMPGEUi) |
                                (OP == VEX_CMPGTU) | (OP == VEX_CMPGTUi) | (OP == VEX_CMPLEU) | (OP == VEX_CMPLEUi);

  ac_int<23, false> const0_23 = 0;
  ac_int<23, false> const1_23 = 0x7fffff;
  ac_int<19, false> const0_19 = 0;
  ac_int<19, false> const1_19 = 0x7ffff;
  ac_int<13, false> const0_13 = 0;
  ac_int<13, false> const1_13 = 0x1fff;

  ac_int<6, false> secondRegAccess = RB;

  char newIssueWidth;
  ac_int<64, true> regValueA = REG[RA];
  ac_int<64, true> regValueB = REG[secondRegAccess];

  ac_int<64, false> regValueA_u;
  ac_int<64, false> regValueB_u;

  regValueA_u.set_slc(0, regValueA);
  regValueB_u.set_slc(0, regValueB);

  dctoEx->floatValueA = regf[RA];
  dctoEx->floatValueB = regf[RB];
  dctoEx->floatValueC = regf[RC];
  dctoEx->funct       = funct;
  dctoEx->opCode      = OP;
  dctoEx->datac       = regValueB;
  dctoEx->pc          = ftoDC.pc;
  dctoEx->isRollback  = ftoDC.isRollback;

  if (isIType) {
    // The instruction is I type
    dctoEx->dest  = RA;
    dctoEx->dataa = IMM19_s;

  } else {
    // The instruction is R type
    dctoEx->dataa = regValueA;

    if (isImm) {
      dctoEx->dest = RB;
      if (isUnsigned)
        dctoEx->datab = IMM13_u;
      else
        dctoEx->datab = IMM13_s;
    } else {
      dctoEx->dest  = RC;
      dctoEx->datab = regValueB;
    }
  }
  switch (OP) { // Select the right operation and place the right values into the right operands
    case VEX_GOTO:
      dctoEx->opCode = 0;
      NEXT_PC        = 4 * IMM19_u;
      break; // GOTO1

    case VEX_CALL:

      dctoEx->dataa = PC + 4 * incrementInstrMem;
      NEXT_PC       = IMM19_u;
      NEXT_PC       = NEXT_PC << 2;
      NEXT_PC       = 4 * IMM19_u;
      break; // CALL

    case VEX_CALLR:
      dctoEx->dataa = PC + 4 * incrementInstrMem;
      NEXT_PC       = regValueA + IMM19_s;
      break; // ICALL

    case VEX_GOTOR:
      dctoEx->opCode = 0;
      NEXT_PC        = regValueA + IMM19_s;
      break; // IGOTO

    case VEX_BR:
      dctoEx->opCode = 0;
      if (regValueA == regValueB)
        NEXT_PC = PC + 4 * IMM13_s - 4 * incrementInstrMem;
      break; // BR

    case VEX_BRF:
      dctoEx->opCode = 0;
      if (regValueA != regValueB)
        NEXT_PC = PC + 4 * IMM13_s - 4 * incrementInstrMem;
      break; // BRF
    case VEX_BGE:
      dctoEx->opCode = 0;
      if (regValueA >= regValueB)
        NEXT_PC = PC + 4 * IMM13_s - 4 * incrementInstrMem;
      break; // BR

    case VEX_BLT:
      dctoEx->opCode = 0;
      if (regValueA < regValueB)
        NEXT_PC = PC + 4 * IMM13_s - 4 * incrementInstrMem;
      break; // BRF
    case VEX_BGEU:
      dctoEx->opCode = 0;
      if (regValueA_u >= regValueB_u)
        NEXT_PC = PC + 4 * IMM13_s - 4 * incrementInstrMem;
      break; // BR

    case VEX_BLTU:
      dctoEx->opCode = 0;
      if (regValueA_u < regValueB_u)
        NEXT_PC = PC + 4 * IMM13_s - 4 * incrementInstrMem;
      break; // BRF

#ifndef __CATAPULT
    case VEX_RECONFFS:
      dctoEx->opCode = 0;

      newIssueWidth = IMM19_u.slc<4>(11);
      // If the issue width change, we may have to correct the next PC value
      if (this->issueWidth <= 4 && newIssueWidth > 4)
        NEXT_PC += 4;
      else if (this->issueWidth > 4 && newIssueWidth <= 4)
        NEXT_PC -= 4;

      if (newIssueWidth > 4)
        incrementInstrMem = 2;
      else
        incrementInstrMem = 1;

      this->issueWidth        = newIssueWidth;
      this->unitActivation[0] = IMM19_u[0];
      this->unitActivation[1] = IMM19_u[1];
      this->unitActivation[2] = IMM19_u[2];
      this->unitActivation[3] = IMM19_u[3];
      this->unitActivation[4] = IMM19_u[4];
      this->unitActivation[5] = IMM19_u[5];
      this->unitActivation[6] = IMM19_u[6];
      this->unitActivation[7] = IMM19_u[7];

      this->muxValues[0] = IMM19_u[8];
      this->muxValues[1] = IMM19_u[9];
      this->muxValues[2] = IMM19_u[10];

      timeInConfig[currentConfig] += (cycle - lastReconf);
      lastReconf    = cycle;
      currentConfig = RA;
      // TODO: handle reg file
      // TODO: add some code to check/wait if an execution unit is disabled

      break;

    case VEX_SYSTEM:

      if ((IMM19_s & 0xf) == VEX_SYSTEM_ECALL) {
        dctoEx->dataa  = this->solveSyscall(REG[17], REG[10], REG[11], REG[12], REG[13]);
        dctoEx->dest   = 10;
        dctoEx->opCode = VEX_MOVI;
      } else if ((IMM19_s & 0xf) == VEX_SYSTEM_CSRRS) {
        dctoEx->opCode = VEX_MOVI;
        dctoEx->dataa  = cycle;
      }
      break;

#endif
    default:
      break;
  }

  if (OP == VEX_FNMADD || OP == VEX_FMADD || OP == VEX_FNMSUB || OP == VEX_FMSUB) {
    dctoEx->dest = RD;
  }
}

#ifdef __CATAPULT
int doStep(ac_int<8, false> memory0[65536], ac_int<8, false> memory1[65536], ac_int<8, false> memory2[65536],
           ac_int<8, false> memory3[65536], ac_int<8, false> memory4[65536], ac_int<8, false> memory5[65536],
           ac_int<8, false> memory6[65536], ac_int<8, false> memory7[65536], ac_int<128, false> RI[65536])
{
#else
int VexSimulator::doStep()
{
#endif

  doWB(memtoWB2);
  doWB(memtoWB3);
  doWB(memtoWB7);
  doWB(memtoWB8);

  ///////////////////////////////////////////////////////
  //													 //
  //                         EX                        //
  //													 //
  ///////////////////////////////////////////////////////

  doEx(dctoEx1, &extoMem1);
  doExMult(dctoEx2, &extoMem2);
  doExMult(dctoEx3, &extoMem3);
  doEx(dctoEx4, &extoMem4);
  doEx(dctoEx5, &extoMem5);
  doEx(dctoEx6, &extoMem6);
  doEx(dctoEx7, &extoMem7);
  doExMult(dctoEx8, &extoMem8);

  ///////////////////////////////////////////////////////
  //													 //
  //                       M                           //
  //													 //
  ///////////////////////////////////////////////////////

  doMemNoMem(extoMem1, &memtoWB1);
  doMemNoMem(extoMem3, &memtoWB3);
  doMemNoMem(extoMem4, &memtoWB4);
  doMemNoMem(extoMem5, &memtoWB5);
  doMemNoMem(extoMem6, &memtoWB6);
  doMemNoMem(extoMem8, &memtoWB8);

#ifdef __CATAPULT
  doMem(extoMem2, &memtoWB2, memory0, memory1, memory2, memory3, memory4, memory5, memory6, memory7);
  doMem(extoMem7, &memtoWB7, memory0, memory1, memory2, memory3, memory4, memory5, memory6, memory7);

#else
  doMem(extoMem2, &memtoWB2);
  doMem(extoMem7, &memtoWB7);
#endif

  //		doMem(extoMem6, &memtoWB6, DATA0, DATA1, DATA2, DATA3);

  ///////////////////////////////////////////////////////
  //													 //
  //                       WB                          //
  //  												 //
  ///////////////////////////////////////////////////////

  doWB(memtoWB1);
  doWB(memtoWB4);
  doWB(memtoWB5);
  doWB(memtoWB6);

  ///////////////////////////////////////////////////////
  //													 //
  //                       DC                          //
  //													 //
  ///////////////////////////////////////////////////////

  NEXT_PC = PC + 4;
  if (this->issueWidth > 4)
    NEXT_PC += 4;

  doDCBr(ftoDC1, &dctoEx1);

#ifdef __CATAPULT
  doDCMem(ftoDC2, &dctoEx2, memory0, memory1, memory2, memory3, memory4, memory5, memory6, memory7);
#else
  doDCMem(ftoDC2, &dctoEx2);
#endif
  doDC(ftoDC3, &dctoEx3);
  doDC(ftoDC4, &dctoEx4);

  doDC(ftoDC5, &dctoEx5);
  doDC(ftoDC6, &dctoEx6);

#ifdef __CATAPULT
  doDCMem(ftoDC7, &dctoEx7, memory0, memory1, memory2, memory3, memory4, memory5, memory6, memory7);
#else
  doDCMem(ftoDC7, &dctoEx7);
#endif
  doDC(ftoDC8, &dctoEx8);

  ac_int<7, false> OP1 = ftoDC1.instruction.slc<7>(0);

  // If the operation code is 0x2f then the processor stops
  if (OP1 == 0x2F) {
    stop    = 1;
    NEXT_PC = PC;
  }
  // If the operation code is 0x2f then the processor stops
  if (stop == 1) {

    return PC;
  }

  ///////////////////////////////////////////////////////
  //                       F                           //
  ///////////////////////////////////////////////////////

  // Retrieving new instruction

  ac_int<64, false> secondLoadAddress = (PC >> 2) + 1;

  ac_int<32, false> instructions[8];
  instructions[0] = RI[(int)PC + 0];
  instructions[1] = RI[(int)PC + 1];
  instructions[2] = RI[(int)PC + 2];
  instructions[3] = RI[(int)PC + 3];
  instructions[4] = RI[(int)PC + 4];
  instructions[5] = RI[(int)PC + 5];
  instructions[6] = RI[(int)PC + 6];
  instructions[7] = RI[(int)PC + 7];

  ac_int<32, false> nopInstr = 0;

// Redirect instructions to thier own ways
#ifndef __CATAPULT

  ftoDC1.instruction = instructions[0];
  ftoDC2.instruction = this->unitActivation[1] ? instructions[1] : nopInstr;
  ftoDC3.instruction = this->unitActivation[2] ? instructions[2] : nopInstr;
  ftoDC4.instruction = this->unitActivation[3] ? instructions[3] : nopInstr;

  ftoDC5.instruction = this->unitActivation[4] ? instructions[4] : nopInstr;
  ftoDC6.instruction = this->unitActivation[5] ? (this->muxValues[0] ? instructions[1] : instructions[5]) : nopInstr;
  ftoDC7.instruction = this->unitActivation[6] ? (this->muxValues[1] ? instructions[2] : instructions[6]) : nopInstr;
  ftoDC8.instruction = this->unitActivation[7] ? (this->muxValues[2] ? instructions[3] : instructions[7]) : nopInstr;

  ftoDC1.pc = PC;
  ftoDC2.pc = PC;
  ftoDC3.pc = PC;
  ftoDC4.pc = PC;
  ftoDC5.pc = PC;
  ftoDC6.pc = PC;
  ftoDC7.pc = PC;
  ftoDC8.pc = PC;

  // We increment IPc counters
  if (ftoDC1.instruction != 0)
    nbInstr++;
  if (ftoDC2.instruction != 0)
    nbInstr++;
  if (ftoDC3.instruction != 0)
    nbInstr++;
  if (ftoDC4.instruction != 0)
    nbInstr++;
  if (ftoDC5.instruction != 0)
    nbInstr++;
  if (ftoDC6.instruction != 0)
    nbInstr++;
  if (ftoDC7.instruction != 0)
    nbInstr++;
  if (ftoDC8.instruction != 0)
    nbInstr++;

#else
  ftoDC1.instruction = instructions[0];
  ftoDC2.instruction = instructions[1];
  ftoDC3.instruction = instructions[2];
  ftoDC4.instruction = instructions[3];

#endif

  nbCycleType[typeInstr[(int)PC / 4]]++;

  int pcValueForDebug = PC;
  // Next instruction

  PC = NEXT_PC;
  cycle++;

  // DISPLAY

#ifndef __CATAPULT

  if (debugLevel >= 1 || 1 /* || (PC >= 4*22090 && PC < 4*22125) || (PC >= 4*458 && PC < 4*507)*/) {

    std::cerr << std::to_string(cycle) + ";" + std::to_string(pcValueForDebug) + ";";
    //		if (this->unitActivation[0])
    //			std::cerr << "\033[1;31m" << printDecodedInstr(ftoDC1.instruction) << "\033[0m;";
    //		if (this->unitActivation[1])
    //			std::cerr << "\033[1;35m" << printDecodedInstr(ftoDC2.instruction) << "\033[0m;";
    //		if (this->unitActivation[2])
    //			std::cerr << "\033[1;34m" << printDecodedInstr(ftoDC3.instruction) << "\033[0m;";
    //		if (this->unitActivation[3])
    //			std::cerr << "\033[1;33m" << printDecodedInstr(ftoDC4.instruction) << "\033[0m;";
    //		if (this->unitActivation[4])
    //			std::cerr << "\033[1;33m" << printDecodedInstr(ftoDC5.instruction) << "\033[0m;";
    //		if (this->unitActivation[5])
    //			std::cerr << "\033[1;33m" << printDecodedInstr(ftoDC6.instruction) << "\033[0m;";
    //		if (this->unitActivation[6])
    //			std::cerr << "\033[1;32m" << printDecodedInstr(ftoDC7.instruction) << "\033[0m;";
    //		if (this->unitActivation[7])
    //			std::cerr << "\033[1;34m" << printDecodedInstr(ftoDC8.instruction) << "\033[0m;";

    if (this->unitActivation[0])
      std::cerr << printDecodedInstr(ftoDC1.instruction);
    if (this->unitActivation[1])
      std::cerr << printDecodedInstr(ftoDC2.instruction);
    if (this->unitActivation[2])
      std::cerr << printDecodedInstr(ftoDC3.instruction);
    if (this->unitActivation[3])
      std::cerr << printDecodedInstr(ftoDC4.instruction);
    if (this->unitActivation[4])
      std::cerr << printDecodedInstr(ftoDC5.instruction);
    if (this->unitActivation[5])
      std::cerr << printDecodedInstr(ftoDC6.instruction);
    if (this->unitActivation[6])
      std::cerr << printDecodedInstr(ftoDC7.instruction);
    if (this->unitActivation[7])
      std::cerr << printDecodedInstr(ftoDC8.instruction);

    fprintf(stderr, ";");

    for (int oneRegister = 0; oneRegister < 38; oneRegister++) {
      fprintf(stderr, "%lx;", (long)REG[oneRegister]);
    }
    fprintf(stderr, ";;%lx;", (long)REG[63]);

    fprintf(stderr, "\n");
  }

#endif

  return 0;
}

#ifdef __CATAPULT
int run(int mainPc, ac_int<8, false> memory0[65536], ac_int<8, false> memory1[65536], ac_int<8, false> memory2[65536],
        ac_int<8, false> memory3[65536], ac_int<8, false> memory4[65536], ac_int<8, false> memory5[65536],
        ac_int<8, false> memory6[65536], ac_int<8, false> memory7[65536], ac_int<128, false> RI[65536])
{
#else
int VexSimulator::initializeRun(int mainPc, int argc, char* argv[])
{
#endif

  if (this->issueWidth > 4)
    incrementInstrMem = 2;
  else
    incrementInstrMem = 1;

#ifndef __CATAPULT
  // We clear IPC counter
  this->nbInstr     = 0;
  this->lastNbCycle = 0;
  this->lastNbInstr = 0;
  this->cycle       = 0;
  for (int oneConfig = 0; oneConfig < 32; oneConfig++) {
    this->timeInConfig[oneConfig] = 0;
  }
  this->lastReconf = 0;
#endif

  // Initialise program counter
  PC     = mainPc;
  REG[2] = 0x70000;
  stop   = 0;

  /*********************************************************************
   * Definition and initialization of all pipeline registers
   *********************************************************************/
  memtoWB1.WBena = 0;
  memtoWB2.WBena = 0;
  memtoWB3.WBena = 0;
  memtoWB4.WBena = 0;
  memtoWB5.WBena = 0;
  memtoWB6.WBena = 0;
  memtoWB7.WBena = 0;
  memtoWB8.WBena = 0;

  extoMem1.WBena  = 0;
  extoMem2.WBena  = 0;
  extoMem3.WBena  = 0;
  extoMem4.WBena  = 0;
  extoMem5.WBena  = 0;
  extoMem6.WBena  = 0;
  extoMem7.WBena  = 0;
  extoMem8.WBena  = 0;
  extoMem1.opCode = 0;
  extoMem2.opCode = 0;
  extoMem3.opCode = 0;
  extoMem4.opCode = 0;
  extoMem5.opCode = 0;
  extoMem6.opCode = 0;
  extoMem7.opCode = 0;
  extoMem8.opCode = 0;

  dctoEx1.opCode = 0;
  dctoEx2.opCode = 0;
  dctoEx3.opCode = 0;
  dctoEx4.opCode = 0;
  dctoEx5.opCode = 0;
  dctoEx6.opCode = 0;
  dctoEx7.opCode = 0;
  dctoEx8.opCode = 0;

  ftoDC1.instruction = 0;
  ftoDC2.instruction = 0;
  ftoDC3.instruction = 0;
  ftoDC4.instruction = 0;
  ftoDC5.instruction = 0;
  ftoDC6.instruction = 0;
  ftoDC7.instruction = 0;
  ftoDC8.instruction = 0;

  /*	Description of the VLIW pipeline.
   * Above we defined :
   * 	-> Four groups of function which performs a pipeline step :
   * 		* doDC performs instruction decoding, register file access and intiate memory access (if needed)
   * 		* doEx performs ALU operations and select the correct result
   * 		* doMem performs the memory write and read the memory operation initiated at dc
   * 		* doWB write the result back in the result
   *
   * 	-> data structures to store values from one step to the next
   *
   * 	The pipeline description is done as follow:
   *
   * 	To describe a five step pipeline, we call each pipeline step from the last to the first:
   * 		WB, Mem, Ex, DC and Fetch
   * 	Then, after five loop iterations, an instruction fetched at iteration i will be decoded at iteration i+1,
   * 	executed at iteration i+2, performs memory access at i+3 and the result will be wrote back at i+4.
   *
   *
   * 	In the following we describe a three step pipeline (with zero latency eg. an instruction can use the result of *
   * 	instructions from previous cycle.  One issue is organized as a four step pipeline to perform multiplication.
   *
   * 	Pipeline way 1 : branch and common
   * 	Pipeline way 2 : Memory and common
   * 	Pipeline way 3 : Mult and common (one more step)
   *  Pipeline way 4 : Common only
   *
   */
#ifdef __CATAPULT
  while (1) {
    int returnedValue = doStep(memory0, memory1, memory2, memory3, memory4, memory5, memory6, memory7, RI);

    if (returnedValue != 0)
      return returnedValue;
  }
#endif

  this->initialize(argc, argv);

  return 0;
}

#ifndef __CATAPULT

void VexSimulator::initializeDataMemory(unsigned char* content, unsigned int size, unsigned int start)
{
  for (int i = 0; i < size; i++) {
    ac_int<8, false> oneByte = content[i];
    ac_int<64, false> index  = start + i;

    this->stb(index, oneByte);
  }
}

int VexSimulator::doStep(int numberCycles)
{
  this->stop              = 0;
  int initialCycleCounter = cycle;
  while (cycle < initialCycleCounter + numberCycles) {
    int returnedValue = doStep();

    if (returnedValue != 0) {
      ftoDC1.instruction = 0;
      ftoDC2.instruction = 0;
      ftoDC3.instruction = 0;
      ftoDC4.instruction = 0;

      return returnedValue;
    }
  }
  return 0;
}

float VexSimulator::getAverageIPC()
{
  double localNBInstr = this->nbInstr - this->lastNbInstr;
  double nbCycle      = this->cycle - this->lastNbCycle;
  float result        = localNBInstr / nbCycle;

  this->lastNbInstr = this->nbInstr;
  this->lastNbCycle = this->cycle;

  return result;
}

#endif
#endif
