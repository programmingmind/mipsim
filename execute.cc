#include "mipsim.hpp"
#include <stdint.h>

Stats stats;
Caches caches(0);

unsigned int signExtend16to32ui(short i) {
  return static_cast<unsigned int>(static_cast<int>(i));
}

unsigned int signExtend8to32ui(int i) {
  return static_cast<unsigned int>(static_cast<int>(i & 0xFF));
}

//check if about to go out of range
void determineLatchesUsed(unsigned int regLoaded) {  
  Data32 d = imem[pc];
  RType rrt(d);
  IType rri(d);

  if (d != Data32(0)) {
    switch(d.classifyType(d)) {
      case R_TYPE:
        if (rrt.rs == regLoaded || rrt.rt == regLoaded)
          stats.numExForwards++;
        if (rrt.rd == regLoaded)
          return;
      break;
      case I_TYPE:
        if (rri.rs == regLoaded || ((rri.op==4 || rri.op==5 || rri.op==43 || rri.op==40) && rri.rt==regLoaded))
          stats.numExForwards++;
        if (rri.rt == regLoaded)
          return;
      break;
    }
  }

  d = imem[pc + 4];
  RType rrtt(d);
  IType rrii(d);

  if (d != Data32(0) ) {
    switch(d.classifyType(d)) {
      case R_TYPE:
        if (rrtt.rs == regLoaded || rrtt.rt == regLoaded)
          stats.numMemForwards++;
        break;
      case I_TYPE:
        if (rrii.rs == regLoaded || ((rrii.op==4 || rrii.op==5 || rrii.op==43 || rrii.op==40) && rrii.rt==regLoaded))
          stats.numMemForwards++;
      break;
    } 
  }
}

void determineLoadUseHazard(unsigned int regLoaded) {
  Data32 d = imem[pc];
  RType rrt(d);
  IType rri(d);
  Type type = d.classifyType(d);

  if ((type==R_TYPE && (rrt.rs==regLoaded || rrt.rt==regLoaded)) || (type==I_TYPE && (rri.rs == regLoaded || ((rri.op==4 || rri.op==5 || rri.op==43 || rri.op==40) && rri.rt==regLoaded))))
    stats.loadHasLoadUseHazard++;
  else {
    stats.loadHasNoLoadUseHazard++;
    if (d == Data32(0))
      stats.loadHasLoadUseStall++;
  }
}


void execute() {
  Data32 instr = imem[pc];
  Data32 temp = NULL;
  GenericType rg(instr);
  RType rt(instr);
  IType ri(instr);
  JType rj(instr);
  unsigned int pctarget = pc + 4;
  unsigned int addr, tmp;
  /*if ((stats.instrs%1000000)==0)
    cout<<"stats.instrs: "<<stats.instrs<<endl;*/
  stats.instrs++;
  stats.cycles++;
  pc = pctarget;
  switch(rg.op) {
  case OP_SPECIAL:
    if (instr != Data32(0) && rg.func != SP_JR)
      determineLatchesUsed(rt.rd);
    switch(rg.func) {
    case SP_ADDU:
      rf.write(rt.rd, rf[rt.rs] + rf[rt.rt]);
      stats.numRegReads += 2;
      stats.numRegWrites++;
      stats.numRType++;
      break;
    case SP_ADD:
      rf.write(rt.rd, rf[rt.rs] + rf[rt.rt]);
      stats.numRegReads += 2;
      stats.numRegWrites++;
      stats.numRType++;
      break;
    case SP_SUB: //new
      rf.write(rt.rd, rf[rt.rs] - rf[rt.rt]);
      stats.numRType++;
      stats.numRegReads += 2;
      stats.numRegWrites++;
      break;
    /*case SP_SUBU: //new
      rf.write(rt.rd, (unsigned int)rf[rt.rs] - (unsigned int)rf[rt.rt]);
      stats.numRType++;
      stats.numRegReads += 2;
      stats.numRegWrites++;
      break;*/
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
      if (instr == Data32(0))
        stats.instrs--;
      else
         stats.numRType++;
      rf.write(rt.rd, rf[rt.rt] << rt.sa);
      stats.numRegReads++;
      stats.numRegWrites++;
      break;
    case SP_JALR:
      rf.write(31, pc + 4);

      if (imem[pc] == Data32(0)) {
       stats.hasUselessJumpDelaySlot++;
      }
      else {
        stats.hasUsefulJumpDelaySlot++;
      }
    
      execute(); 
      pc = (unsigned int)rf[rt.rs];
      stats.numRType++;
      stats.numRegReads++;
      stats.numRegWrites++;
      break;
    case SP_JR:
      if (imem[pc] == Data32(0)) {
        stats.hasUselessJumpDelaySlot++;
      }
      else {
        stats.hasUsefulJumpDelaySlot++;
      }

      execute();
      pc = (unsigned int)rf[rt.rs];
      stats.numRType++;
      stats.numRegReads++;
      break;
    case SP_SRL: //new, assumed register values are signed
      rf.write(rt.rd, rf[rt.rt] >> rt.sa);
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
    determineLatchesUsed(ri.rt);
    rf.write(ri.rt, rf[ri.rs] + signExtend16to32ui(ri.imm));
    stats.numIType++;
    stats.numRegReads++;
    stats.numRegWrites++;
    break;
  case OP_SW:
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
    temp.set_data_ubyte4(0, rf[ri.rt] & 0xFF);
    dmem.write(addr, temp);
    stats.numIType++;
    stats.numRegReads += 2;
    stats.numMemWrites++;
    break;
  case OP_LW:
    determineLatchesUsed(ri.rt);
    addr = rf[ri.rs] + signExtend16to32ui(ri.imm);
    caches.access(addr);
    rf.write(ri.rt, dmem[addr]);
    stats.numIType++;
    stats.numRegReads++;
    stats.numRegWrites++;
    stats.numMemReads++;
    determineLoadUseHazard(ri.rt);
    break;
  case OP_LB:
    determineLatchesUsed(ri.rt);
    addr = rf[ri.rs] + signExtend16to32ui(ri.imm);
    caches.access(addr);
    rf.write(ri.rt, signExtend8to32ui(dmem[addr].data_ubyte4(0)));
    stats.numIType++;
    stats.numRegReads++;
    stats.numRegWrites++;
    stats.numMemReads++;
    determineLoadUseHazard(ri.rt);
    break;
  case OP_LBU:
    determineLatchesUsed(ri.rt);
    addr = rf[ri.rs] + signExtend16to32ui(ri.imm);
    caches.access(addr);
    rf.write(ri.rt, signExtend8to32ui(dmem[addr].data_ubyte4(0)));
    stats.numIType++;
    stats.numRegReads++;
    stats.numRegWrites++;
    stats.numMemReads++;
    determineLoadUseHazard(ri.rt);
    break;
  case OP_LUI:
    determineLatchesUsed(ri.rt);
    rf.write(ri.rt, ri.imm << 16);
    stats.numIType++;
    stats.numRegWrites++;
    break;
  case OP_J:
     if (imem[pc] == Data32(0)) {
      stats.hasUselessJumpDelaySlot++;
    }
    else {
      stats.hasUsefulJumpDelaySlot++;
    }
    pctarget = (pc & 0xf0000000) | (rj.target << 2);
    execute();
    pc = pctarget;
    stats.numJType++;
    break;
  case OP_JAL:
    rf.write(31, pc + 4);
    if (imem[pc] == Data32(0)) {
      stats.hasUselessJumpDelaySlot++;
    }
    else {
      stats.hasUsefulJumpDelaySlot++;
    }
    pctarget = (pc & 0xf0000000) | (rj.target << 2);
    execute();
    pc = pctarget;
    stats.numJType++;
    stats.numRegWrites++;
    break;
  case OP_SLTI:
    determineLatchesUsed(ri.rt);
    rf.write(ri.rt, rf[ri.rs] < ri.imm ? 1 : 0);
    stats.numIType++;
    stats.numRegWrites++;
    stats.numRegReads++;
    break;
  case OP_BNE:
    if (imem[pc] == Data32(0))
      stats.hasUselessBranchDelaySlot++;
    else
      stats.hasUsefulBranchDelaySlot++;
    if (rf[ri.rs] != rf[ri.rt]) {
      pctarget = pc + (ri.imm<<2);
      execute();
      pc = pctarget;
      if (ri.imm < 0)
        stats.numBackwardBranchesTaken++;
      else
        stats.numForwardBranchesTaken++;
    }
    else {
      if (ri.imm < 0)
        stats.numBackwardBranchesNotTaken++;
      else
        stats.numForwardBranchesNotTaken++;
    }

    stats.numIType++;
    stats.numRegReads += 2;
    break;
  case OP_BEQ:
    if (imem[pc] == Data32(0))
      stats.hasUselessBranchDelaySlot++;
    else
      stats.hasUsefulBranchDelaySlot++;
    if (rf[ri.rs] == rf[ri.rt]) {
      pctarget = pc + (ri.imm<<2);
      execute(); 
      pc = pctarget;
      if (ri.imm < 0)
        stats.numBackwardBranchesTaken++;
      else
        stats.numForwardBranchesTaken++;
    }
    else {
      if (ri.imm < 0)
        stats.numBackwardBranchesNotTaken++;
      else
        stats.numForwardBranchesNotTaken++;
    }
    
    stats.numIType++;
    stats.numRegReads += 2;
    break;
  case OP_BLEZ:
    if (imem[pc] == Data32(0))
      stats.hasUselessBranchDelaySlot++;
    else
      stats.hasUsefulBranchDelaySlot++;
    if (rf[ri.rs] <= 0) {
      pctarget = pc + (ri.imm<<2);
      execute();
      pc = pctarget;
      if (ri.imm < 0)
        stats.numBackwardBranchesTaken++;
      else
        stats.numForwardBranchesTaken++;
    }
    else {
      if (ri.imm < 0)
        stats.numBackwardBranchesNotTaken++;
      else
        stats.numForwardBranchesNotTaken++;
    }

    stats.numIType++;
    stats.numRegReads++;
    break;
  case OP_ORI:
    determineLatchesUsed(ri.rt);
    rf.write(ri.rt, rf[ri.rs] | ri.imm);
    stats.numIType++;
    stats.numRegReads++;
    stats.numRegWrites++;
    break;
  case OP_SLTIU:
    determineLatchesUsed(ri.rt);
    rf.write(ri.rt, rf[ri.rs] < ri.imm ? 1 : 0);
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
