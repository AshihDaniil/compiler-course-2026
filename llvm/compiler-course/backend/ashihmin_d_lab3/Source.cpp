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

  StringRef getPassName() const override {
    return "Ashihmin D. Lab3: INC/DEC to ADD/SUB Merger";
  }

  bool runOnMachineFunction(MachineFunction &MF) override {
    const X86InstrInfo *X86TII = MF.getSubtarget<X86Subtarget>().getInstrInfo();
    bool IsModified = false;

    for (auto &MBB : MF) {
      auto MII = MBB.begin();
      while (MII != MBB.end()) {
        int DeltaAccum = 0;
        unsigned AddOp, SubOp, RegWidth;

        // Проверяем, является ли инструкция таргетом для замены
        if (!lookupOpcodeInfo(MII->getOpcode(), DeltaAccum, AddOp, SubOp,
                              RegWidth)) {
          ++MII;
          continue;
        }

        Register CurrentReg = MII->getOperand(0).getReg();
        DebugLoc DL = MII->getDebugLoc();
        auto SequenceHead = MII;

        SmallVector<MachineInstr *, 8> SequenceToProcess;
        int TotalSum = 0;

        // Собираем все идущие подряд INC/DEC для этого же регистра
        while (MII != MBB.end()) {
          int LocalVal = 0;
          unsigned TmpA, TmpS, TmpW;
          if (lookupOpcodeInfo(MII->getOpcode(), LocalVal, TmpA, TmpS, TmpW) &&
              MII->getOperand(0).getReg() == CurrentReg && TmpW == RegWidth) {

            TotalSum += LocalVal;
            SequenceToProcess.push_back(&*MII);
            ++MII;
          } else {
            break;
          }
        }

        if (!SequenceToProcess.empty()) {
          // Если после сложения (например, INC и DEC) получили не ноль
          if (TotalSum != 0) {
            unsigned FinalOpcode = (TotalSum > 0) ? AddOp : SubOp;
            uint64_t Immediate = std::abs(TotalSum);

            BuildMI(MBB, SequenceHead, DL, X86TII->get(FinalOpcode), CurrentReg)
                .addReg(CurrentReg)
                .addImm(Immediate)
                .addReg(X86::EFLAGS, RegState::Define | RegState::Implicit);
          }

          // Удаляем старые инструкции в любом случае (даже если TotalSum == 0)
          for (auto *Inst : SequenceToProcess) {
            Inst->eraseFromParent();
          }
          IsModified = true;
        }
      }
    }
    return IsModified;
  }

private:
  // Маппинг инструкций X86
  bool lookupOpcodeInfo(unsigned Op, int &Val, unsigned &A, unsigned &S,
                        unsigned &W) const {
    switch (Op) {
    case X86::INC8r:
      Val = 1;
      A = X86::ADD8ri;
      S = X86::SUB8ri;
      W = 8;
      return true;
    case X86::DEC8r:
      Val = -1;
      A = X86::ADD8ri;
      S = X86::SUB8ri;
      W = 8;
      return true;
    case X86::INC16r:
      Val = 1;
      A = X86::ADD16ri;
      S = X86::SUB16ri;
      W = 16;
      return true;
    case X86::DEC16r:
      Val = -1;
      A = X86::ADD16ri;
      S = X86::SUB16ri;
      W = 16;
      return true;
    case X86::INC32r:
      Val = 1;
      A = X86::ADD32ri;
      S = X86::SUB32ri;
      W = 32;
      return true;
    case X86::DEC32r:
      Val = -1;
      A = X86::ADD32ri;
      S = X86::SUB32ri;
      W = 32;
      return true;
    case X86::INC64r:
      Val = 1;
      A = X86::ADD64ri32;
      S = X86::SUB64ri32;
      W = 64;
      return true;
    case X86::DEC64r:
      Val = -1;
      A = X86::ADD64ri32;
      S = X86::SUB64ri32;
      W = 64;
      return true;
    default:
      return false;
    }
  }
};

char ashihmin_d_lab3::ID = 0;
} // namespace

static RegisterPass<ashihmin_d_lab3> X("ashihmin_d_lab3",
                                       "ashihmin_d_lab3 pass", false, false);