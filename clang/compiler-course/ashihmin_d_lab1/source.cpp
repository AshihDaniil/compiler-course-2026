#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"

#include <map>
#include <set>

using namespace clang;

namespace {

class ResourceLeakVisitor : public RecursiveASTVisitor<ResourceLeakVisitor> {
public:
  explicit ResourceLeakVisitor(ASTContext *Context) : Context(Context) {}

  bool TraverseFunctionDecl(FunctionDecl *FD) {
    if (!FD || !FD->hasBody())
      return RecursiveASTVisitor::TraverseFunctionDecl(FD);

    Allocated.clear();
    Freed.clear();
    Reported.clear();

    RecursiveASTVisitor::TraverseStmt(FD->getBody());

    FinalReport();

    return true;
  }

  bool VisitVarDecl(VarDecl *D) {
    if (D->hasInit() && isAllocation(D->getInit())) {
      Allocated[D] = D->getLocation();
    }
    return true;
  }

  bool VisitBinaryOperator(BinaryOperator *BO) {
    if (BO->isAssignmentOp() && isAllocation(BO->getRHS())) {
      if (VarDecl *VD = getVarDecl(BO->getLHS())) {
        Allocated[VD] = BO->getOperatorLoc();
      }
    }
    return true;
  }

  bool VisitCallExpr(CallExpr *CE) {
    if (FunctionDecl *FD = CE->getDirectCallee()) {
      std::string Name = FD->getNameAsString();

      if (Name == "free" || Name == "fclose") {
        if (CE->getNumArgs() > 0) {
          if (VarDecl *VD = getVarDecl(CE->getArg(0))) {
            Freed.insert(VD);
          }
        }
      }
    }
    return true;
  }

  bool VisitCXXDeleteExpr(CXXDeleteExpr *DE) {
    if (VarDecl *VD = getVarDecl(DE->getArgument())) {
      Freed.insert(VD);
    }
    return true;
  }

  bool VisitReturnStmt(ReturnStmt *RS) {
    for (auto const &[VD, Loc] : Allocated) {
      if (Freed.count(VD) == 0 && Reported.count(VD) == 0) {
        Report(VD, RS->getReturnLoc(), true);
        Reported.insert(VD);
      }
    }
    return true;
  }

private:
  ASTContext *Context;

  std::map<const VarDecl *, SourceLocation> Allocated;
  std::set<const VarDecl *> Freed;
  std::set<const VarDecl *> Reported;

  bool isAllocation(Expr *E) {
    if (!E)
      return false;

    E = E->IgnoreParenCasts();

    if (isa<CXXNewExpr>(E))
      return true;

    if (auto *CE = dyn_cast<CallExpr>(E)) {
      if (auto *FD = CE->getDirectCallee()) {
        std::string Name = FD->getNameAsString();
        return Name == "malloc" || Name == "fopen";
      }
    }

    return false;
  }

  VarDecl *getVarDecl(Expr *E) {
    if (!E)
      return nullptr;

    E = E->IgnoreParenCasts();

    if (auto *DRE = dyn_cast<DeclRefExpr>(E))
      return dyn_cast<VarDecl>(DRE->getDecl());

    return nullptr;
  }

  void FinalReport() {
    for (auto const &[VD, Loc] : Allocated) {
      if (Freed.count(VD) == 0 && Reported.count(VD) == 0) {
        Report(VD, VD->getLocation(), false);
        Reported.insert(VD);
      }
    }
  }

  void Report(const VarDecl *VD, SourceLocation Loc, bool IsReturn) {
    DiagnosticsEngine &DE = Context->getDiagnostics();
    unsigned DiagID;

    if (IsReturn) {
      DiagID = DE.getCustomDiagID(
          DiagnosticsEngine::Warning,
          "Ресурс для переменной '%0' может быть не освобожден (не "
          "гарантированное освобождение при return)!");
    } else {
      DiagID = DE.getCustomDiagID(
          DiagnosticsEngine::Warning,
          "Память или ресурс для переменной '%0' не освобождены!");
    }

    DE.Report(Loc, DiagID) << VD->getNameAsString();
  }
};

class ResourceLeakConsumer : public ASTConsumer {
public:
  void HandleTranslationUnit(ASTContext &Context) override {
    ResourceLeakVisitor Visitor(&Context);
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }
};

class ResourceLeakAction : public PluginASTAction {
public:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef) override {
    return std::make_unique<ResourceLeakConsumer>();
  }

  bool ParseArgs(const CompilerInstance &,
                 const std::vector<std::string> &) override {
    return true;
  }
};

} // namespace

static FrontendPluginRegistry::Add<ResourceLeakAction>
    X("ashihmin_d_analizator", "Leak detector plugin");