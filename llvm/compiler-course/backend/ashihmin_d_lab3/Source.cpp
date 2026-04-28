#include "X86.h"
#include "X86InstrInfo.h"
#include "X86Subtarget.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"

using namespace llvm;

namespace {

class ashihmin_d_lab3 : public MachineFunctionPass {
public:
  static char ID;
  ashihmin_d_lab3() : MachineFunctionPass(ID) {}

  bool runOnMachineFunction(MachineFunction &MF) override;

  StringRef getPassName() const override {
    return "ashihmin_d_lab3: INC/DEC to ADD/SUB merger";
  }

private:
  bool getIncDecInfo(unsigned Opcode, int &Delta, unsigned &AddOp,
                     unsigned &SubOp, unsigned &RegSize) const {
    switch (Opcode) {
    case X86::INC8r:
      Delta = 1;
      AddOp = X86::ADD8ri;
      SubOp = X86::SUB8ri;
      RegSize = 8;
      return true;
    case X86::INC16r:
      Delta = 1;
      AddOp = X86::ADD16ri;
      SubOp = X86::SUB16ri;
      RegSize = 16;
      return true;
    case X86::INC32r:
      Delta = 1;
      AddOp = X86::ADD32ri;
      SubOp = X86::SUB32ri;
      RegSize = 32;
      return true;
    case X86::INC64r:
      Delta = 1;
      AddOp = X86::ADD64ri32;
      SubOp = X86::SUB64ri32;
      RegSize = 64;
      return true;
    case X86::DEC8r:
      Delta = -1;
      AddOp = X86::ADD8ri;
      SubOp = X86::SUB8ri;
      RegSize = 8;
      return true;
    case X86::DEC16r:
      Delta = -1;
      AddOp = X86::ADD16ri;
      SubOp = X86::SUB16ri;
      RegSize = 16;
      return true;
    case X86::DEC32r:
      Delta = -1;
      AddOp = X86::ADD32ri;
      SubOp = X86::SUB32ri;
      RegSize = 32;
      return true;
    case X86::DEC64r:
      Delta = -1;
      AddOp = X86::ADD64ri32;
      SubOp = X86::SUB64ri32;
      RegSize = 64;
      return true;
    default:
      return false;
    }
  }
};

char ashihmin_d_lab3::ID = 0;

bool ashihmin_d_lab3::runOnMachineFunction(MachineFunction &MF) {
  const X86InstrInfo *TII = MF.getSubtarget<X86Subtarget>().getInstrInfo();
  bool Changed = false;

  for (MachineBasicBlock &MBB : MF) {
    auto MI = MBB.begin();
    while (MI != MBB.end()) {
      int Delta = 0;
      unsigned AddOp, SubOp, RegSize;

      if (!getIncDecInfo(MI->getOpcode(), Delta, AddOp, SubOp, RegSize)) {
        ++MI;
        continue;
      }

      Register Reg = MI->getOperand(0).getReg();
      DebugLoc DL = MI->getDebugLoc();
      auto FirstMI = MI;

      int TotalDelta = 0;
      SmallVector<MachineInstr *, 4> ToErase;

      while (MI != MBB.end()) {
        int CurDelta = 0;
        unsigned CurAdd, CurSub, CurSize;
        if (getIncDecInfo(MI->getOpcode(), CurDelta, CurAdd, CurSub, CurSize) &&
            MI->getOperand(0).getReg() == Reg && CurSize == RegSize) {
          TotalDelta += CurDelta;
          ToErase.push_back(&*MI);
          ++MI;
        } else {
          break;
        }
      }

      if (!ToErase.empty()) {
        if (TotalDelta != 0) {
          unsigned FinalOp = (TotalDelta > 0) ? AddOp : SubOp;
          int FinalImm = std::abs(TotalDelta);

          BuildMI(MBB, FirstMI, DL, TII->get(FinalOp), Reg)
              .addReg(Reg)
              .addImm(FinalImm);
        }

        for (auto *Inst : ToErase) {
          Inst->eraseFromParent();
        }
        Changed = true;
      }
    }
  }
  return Changed;
}

} // namespace

static RegisterPass<ashihmin_d_lab3> X("ashihmin_d_lab3",
                                       "ashihmin_d_lab3 pass", false, false);