#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/Builders.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Tools/Plugins/PassPlugin.h"

using namespace mlir;

namespace {
class ConditionTracingPass
    : public PassWrapper<ConditionTracingPass, OperationPass<ModuleOp>> {
public:
  StringRef getArgument() const final { return "trace-conditions"; }
  StringRef getDescription() const final {
    return "Inserts trace calls into if-conditions";
  }

  void ensureDeclaration(ModuleOp module, StringRef name) {
    if (module.lookupSymbol<func::FuncOp>(name))
      return;
    OpBuilder builder(module.getBodyRegion());
    builder
        .create<func::FuncOp>(module.getLoc(), name,
                              builder.getFunctionType({}, {}))
        .setPrivate();
  }

  void instrumentRegion(Region &region, StringRef beginFunc, StringRef endFunc,
                        OpBuilder &builder) {
    if (region.empty())
      return;
    Block &block = region.front();

    builder.setInsertionPointToStart(&block);
    builder.create<func::CallOp>(builder.getUnknownLoc(), beginFunc,
                                 TypeRange{});

    Operation *terminator = block.getTerminator();
    if (terminator) {
      builder.setInsertionPoint(terminator);
    } else {
      builder.setInsertionPointToEnd(&block);
    }
    builder.create<func::CallOp>(builder.getUnknownLoc(), endFunc, TypeRange{});
  }

  void runOnOperation() override {
    ModuleOp module = getOperation();
    OpBuilder builder(module.getContext());

    ensureDeclaration(module, "trace_condition_then_begin");
    ensureDeclaration(module, "trace_condition_then_end");
    ensureDeclaration(module, "trace_condition_else_begin");
    ensureDeclaration(module, "trace_condition_else_end");

    module.walk([&](Operation *op) {
      if (auto scfIf = dyn_cast<scf::IfOp>(op)) {
        instrumentRegion(scfIf.getThenRegion(), "trace_condition_then_begin",
                         "trace_condition_then_end", builder);
        if (!scfIf.getElseRegion().empty())
          instrumentRegion(scfIf.getElseRegion(), "trace_condition_else_begin",
                           "trace_condition_else_end", builder);
      } else if (auto affineIf = dyn_cast<affine::AffineIfOp>(op)) {
        instrumentRegion(affineIf.getThenRegion(), "trace_condition_then_begin",
                         "trace_condition_then_end", builder);
        if (!affineIf.getElseRegion().empty())
          instrumentRegion(affineIf.getElseRegion(),
                           "trace_condition_else_begin",
                           "trace_condition_else_end", builder);
      }
    });
  }
};
} // namespace

MLIR_DECLARE_EXPLICIT_TYPE_ID(ConditionTracingPass)
MLIR_DEFINE_EXPLICIT_TYPE_ID(ConditionTracingPass)

mlir::PassPluginLibraryInfo getConditionTracingPassPluginInfo() {
  return {MLIR_PLUGIN_API_VERSION, "ConditionTracingPass", "1.0",
          []() { mlir::PassRegistration<ConditionTracingPass>(); }};
}

extern "C" LLVM_ATTRIBUTE_WEAK mlir::PassPluginLibraryInfo
mlirGetPassPluginInfo() {
  return getConditionTracingPassPluginInfo();
}