#include "X86.h"
#include "X86InstrInfo.h"
#include "X86Subtarget.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include <vector>

using namespace llvm;

namespace {

struct X86OpInfo {
  int Change;
  unsigned AddVariant;
  unsigned SubVariant;
  unsigned Width;
};

class ashihmin_d_lab3 : public MachineFunctionPass {
public:
  static char ID;
  ashihmin_d_lab3() : MachineFunctionPass(ID) {}

  StringRef getPassName() const override { return "Ashihmin X86 Arith Folder"; }

  bool runOnMachineFunction(MachineFunction &MF) override {
    const TargetInstrInfo *TII = MF.getSubtarget().getInstrInfo();
    bool Modified = false;

    for (auto &MBB : MF) {
      auto MII = MBB.begin();
      while (MII != MBB.end()) {
        X86OpInfo Info;
        if (!collectInfo(MII->getOpcode(), Info)) {
          ++MII;
          continue;
        }

        Register RegToTrack = MII->getOperand(0).getReg();
        DebugLoc DL = MII->getDebugLoc();

        std::vector<MachineInstr *> Sequence;
        int AccumulatedDelta = 0;

        auto Scanner = MII;
        while (Scanner != MBB.end()) {
          X86OpInfo Current;
          if (collectInfo(Scanner->getOpcode(), Current) &&
              Scanner->getOperand(0).getReg() == RegToTrack &&
              Current.Width == Info.Width) {

            AccumulatedDelta += Current.Change;
            Sequence.push_back(&*Scanner);
            ++Scanner;
          } else {
            break;
          }
        }

        if (!Sequence.empty()) {
          if (AccumulatedDelta != 0) {
            unsigned OpcodeToUse =
                (AccumulatedDelta > 0) ? Info.AddVariant : Info.SubVariant;
            uint64_t FinalImm = std::abs(AccumulatedDelta);

            // Вставляем новую инструкцию перед первым элементом пачки
            BuildMI(MBB, MII, DL, TII->get(OpcodeToUse), RegToTrack)
                .addReg(RegToTrack)
                .addImm(FinalImm)
                .addReg(X86::EFLAGS, RegState::Define | RegState::Implicit);
          }

          // Удаляем старые инструкции
          for (auto *MI : Sequence) {
            MI->eraseFromParent();
          }
          Modified = true;
          MII = Scanner;
        }
      }
    }
    return Modified;
  }

private:
  bool collectInfo(unsigned Opc, X86OpInfo &P) const {
    switch (Opc) {
    case X86::INC8r:
      P = {1, X86::ADD8ri, X86::SUB8ri, 8};
      return true;
    case X86::DEC8r:
      P = {-1, X86::ADD8ri, X86::SUB8ri, 8};
      return true;
    case X86::INC16r:
      P = {1, X86::ADD16ri, X86::SUB16ri, 16};
      return true;
    case X86::DEC16r:
      P = {-1, X86::ADD16ri, X86::SUB16ri, 16};
      return true;
    case X86::INC32r:
      P = {1, X86::ADD32ri, X86::SUB32ri, 32};
      return true;
    case X86::DEC32r:
      P = {-1, X86::ADD32ri, X86::SUB32ri, 32};
      return true;
    case X86::INC64r:
      P = {1, X86::ADD64ri32, X86::SUB64ri32, 64};
      return true;
    case X86::DEC64r:
      P = {-1, X86::ADD64ri32, X86::SUB64ri32, 64};
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