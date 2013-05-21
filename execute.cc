#include "mipsim.hpp"
#include <stdint.h>

Stats stats;
Caches caches(0);

unsigned int signExtend16to32ui(short i) {
  return static_cast<unsigned int>(static_cast<int>(i));
}

unsigned int signExtend8to32ui(int i) {
  //cout<<20<<"\t"<<32<<endl;
  //cout<<i<<"\t"<<(i & 0xFF)<<endl;
  return static_cast<unsigned int>(static_cast<int>(i & 0xFF));
}

void execute() {
//cout<<"Data[40022c]: " << dmem[0x40022c]<<endl;
//  int16_t signedShort;
//  int32_t signedInt;
  Data32 instr = imem[pc];
  Data32 temp = NULL;
  GenericType rg(instr);
  RType rt(instr);
  IType ri(instr);
  JType rj(instr);
  unsigned int pctarget = pc + 4;
  unsigned int addr, tmp;
  //cout<<pc<<"  ";
  //instr.printI(instr);
  stats.instrs++;
  stats.cycles++;
  pc = pctarget;
  switch(rg.op) {
  case OP_SPECIAL:
    switch(rg.func) {
    case SP_ADDU:
//cout<<rf[rt.rs]<<"\t"<<rf[rt.rt]<<endl;
      rf.write(rt.rd, rf[rt.rs] + rf[rt.rt]);
      stats.numRegReads += 2;
      stats.numRegWrites++;
      stats.numRType++;
      break;
//    case SP_ADD:
//      break;
    case SP_SUB: //new
      rf.write(rt.rd, rf[rt.rs] - rf[rt.rt]);
      stats.numRType++;
      stats.numRegReads += 2;
      stats.numRegWrites++;
      break;
    case SP_SUBU: //new
      rf.write(rt.rd, (unsigned int)rf[rt.rs] - (unsigned int)rf[rt.rt]);
      stats.numRType++;
      stats.numRegReads += 2;
      stats.numRegWrites++;
      break;
    case SP_AND: //new
      rf.write(rt.rd, rf[rt.rs] & rf[rt.rt]);
      stats.numRType++;
      stats.numRegReads += 2;
      stats.numRegWrites++;
      break;
    case SP_OR: //new
      rf.write(rt.rd, rf[rt.rs] | rf[rt.rt]);
      stats.numRType++;
      stats.numRegReads += 2;
      stats.numRegWrites++; 
      break;
    case SP_XOR: //new
      rf.write(rt.rd, rf[rt.rs] ^ rf[rt.rt]);
      stats.numRType++;
      stats.numRegReads += 2;
      stats.numRegWrites++; 
      break;
    case SP_NOR: //new
      rf.write(rt.rd, ~(rf[rt.rs] | rf[rt.rt]));
      stats.numRType++;
      stats.numRegReads += 2;
      stats.numRegWrites++;
      break;
    case SP_SLL:
      if (instr != Data32(0)) { // Not a NOP
         rf.write(rt.rd, rf[rt.rt] << rt.sa);
         stats.numRType++;
         stats.numRegReads++;
         stats.numRegWrites++;
      }
      else {
         stats.instrs--;
      }
      break;
    case SP_JALR:
      if (imem[pc] == Data32(0)) {
       stats.hasUselessJumpDelaySlot++;
       stats.cycles++;
      }
      else {
        stats.hasUsefulJumpDelaySlot++;
        execute();
      }
      rf.write(rt.rd, pc + 4);
      pc = (unsigned int)(rf[rt.rs]);
      stats.numRType++;
      stats.numRegReads++;
      stats.numRegWrites++;
      break;
    case SP_JR:
      if (imem[pc] == Data32(0)) {
        stats.hasUselessJumpDelaySlot++;
        stats.cycles++;
      }
      else {
        stats.hasUsefulJumpDelaySlot++;
        execute();
      }
      pc = (unsigned int)(rf[rt.rs]);
      stats.numRType++;
      stats.numRegReads++;
      break;
    case SP_SRL: //new, assumed register values are signed
      rf.write(rt.rd, (unsigned int)rf[rt.rt] >> rt.sa);
      stats.numRType++;
      stats.numRegReads++;
      stats.numRegWrites++;
      break;
    case SP_SRA: //new assumed register values are signed
      rf.write(rt.rd, rf[rt.rt] >> rt.sa);
      stats.numRType++;
      stats.numRegReads++;
      stats.numRegWrites++;
      break;
    case SP_SLT:
      rf.write(rt.rd, rf[rt.rs] < rf[rt.rt] ? 1 : 0);
      stats.numRType++;
      stats.numRegReads += 2;
      stats.numRegWrites++;
      break;
    default:
      cout << "Unsupported special instruction: ";
      instr.printI(instr);
      exit(1);
      break;
    }
    break;
  case OP_ADDIU:
    rf.write(ri.rt, rf[ri.rs] + signExtend16to32ui(ri.imm));
    stats.numIType++;
    stats.numRegReads++;
    stats.numRegWrites++;
    break;
  case OP_SW:
//cout<<dmem[addr]<<endl;
    addr = rf[ri.rs] + signExtend16to32ui(ri.imm);
    caches.access(addr);
    dmem.write(addr, rf[ri.rt]);
    stats.numIType++;
    stats.numRegReads += 2;
    stats.numMemWrites++;
    break;
  case OP_SB:
    addr = rf[ri.rs] + signExtend16to32ui(ri.imm);
    caches.access(addr);
    temp = dmem[addr];
    temp.set_data_ubyte4(3, rf[ri.rt] & 0xFF);
    dmem.write(addr, temp);
    stats.numIType++;
    stats.numRegReads += 2;
    stats.numMemWrites++;
    stats.numMemReads++;
    break;
  case OP_LW:
    addr = rf[ri.rs] + signExtend16to32ui(ri.imm);
//cout<<rf[ri.rs]<<"\t"<<ri.imm<<"\t"<<addr<<endl;
//cout<<dmem[addr]<<endl;
    caches.access(addr);
    rf.write(ri.rt, dmem[addr]);
    stats.numIType++;
    stats.numRegReads++;
    stats.numRegWrites++;
    stats.numMemReads++;

    if (0) // check load use Hazard
      stats.loadHasLoadUseHazard++;
    else {
      stats.loadHasNoLoadUseHazard++;
      if (imem[pc] == Data32(0))
        stats.loadHasLoadUseStall++;
    }

    break;
  case OP_LB:
//cout<<rf[ri.rs]<<"\t"<<rf[ri.rt]<<"\t"<<ri.imm<<"\t"<<signExtend16to32ui(ri.imm)<<endl;
    addr = rf[ri.rs] + signExtend16to32ui(ri.imm);
//cout<<dmem[rf[ri.rs]]<<endl;
//cout<<addr<<endl; 
    caches.access(addr);
//cout<<dmem[addr]<<endl;
    rf.write(ri.rt, signExtend8to32ui(dmem[addr].data_ubyte4(3)));
   //cout<<rf[ri.rt]<<endl;
    stats.numIType++;
    stats.numRegReads++;
    stats.numRegWrites++;
    stats.numMemReads++;
    break;
  case OP_LUI:
//cout<<ri.imm<<"\t"<<(ri.imm << 16)<<endl;
    rf.write(ri.rt, ri.imm << 16);
    stats.numIType++;
    stats.numRegWrites++;
//cout<<rf[ri.rt]<<endl;
    break;
  case OP_J:
     if (imem[pc] == Data32(0)) {
      stats.hasUselessJumpDelaySlot++;
      stats.cycles++;
    }
    else {
      stats.hasUsefulJumpDelaySlot++;   //cout <<(pc&0xf0000000)<<'\t'<<(rj.target<<2)<<endl;
      execute();
    }
    pc = (pc & 0xf0000000) | (rj.target << 2);
    stats.numJType++;
    break;
  case OP_JAL:
    if (imem[pc] == Data32(0)) {
      stats.hasUselessJumpDelaySlot++;
      stats.cycles++;
    }
    else {
      stats.hasUsefulJumpDelaySlot++;
      execute();
    }
    rf.write(31, pc + 4);
    pc = (pc & 0xf0000000) | (rj.target << 2);
    stats.numJType++;
    stats.numRegWrites++;
    break;
  case OP_SLTI:
    rf.write(ri.rt, rf[ri.rs] < ri.imm ? 1 : 0);
    stats.numIType++;
    stats.numRegWrites++;
    stats.numRegReads++;
    break;
  case OP_BNE:
    if (imem[pc] == Data32(0))
      stats.hasUselessBranchDelaySlot++;
    if (rf[ri.rs] != rf[ri.rt]) {
      execute();
      pc = pc + (ri.imm<<2) - 8;
      if (ri.imm < 0)
        stats.numBackwardBranchesTaken++;
      else
        stats.numForwardBranchesTaken++;
    }
    else {
      if (ri.imm < 0)
        stats.numBackwardBranchesNotTaken++;
      else
        stats.numForwardBranchesTaken++;
    }

    stats.numIType++;
    stats.numRegReads += 2;
    break;
  case OP_BEQ:
    if (rf[ri.rs] == rf[ri.rt]) 
      pc = pc + (ri.imm<<2) - 4;
    stats.numIType++;
    stats.numRegReads += 2;
    break;
//new vvvv
  case OP_BLEZ:
    if (rf[ri.rs] <= 0)
      pc = pc + (ri.imm<<2) - 4;
    stats.numIType++;
    stats.numRegReads++;
    break;
  case OP_ORI:
    rf.write(ri.rt, rf[ri.rs] | ri.imm);
    stats.numIType++;
    stats.numRegReads++;
    stats.numRegWrites++;
    break;
  case OP_SLTIU:
    rf.write(ri.rt, rf[ri.rs] < (unsigned short)ri.imm ? 1 : 0);
    stats.numIType++;
    stats.numRegReads++;
    stats.numRegWrites++;
    break;
  default:
    cout << "Unsupported instruction: ";
    instr.printI(instr);
    exit(1);
    break;
  }
}
