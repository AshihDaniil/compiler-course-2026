#include "clang/AST/ASTConsumer.h"
#include "clang/AST/Expr.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "llvm/Support/raw_ostream.h"

#include <map>

using namespace clang;

namespace {

class ResourceLeakVisitor : public RecursiveASTVisitor<ResourceLeakVisitor> {
public:
  explicit ResourceLeakVisitor(ASTContext *Context) : Context(Context) {}

  // Обрабатываем каждую функцию отдельно
  bool VisitFunctionDecl(FunctionDecl *D) {
    if (D->hasBody()) {
      AllocatedResources.clear();
      TraverseStmt(D->getBody());
      ReportFinalLeaks();
    }
    return false;
  }

  // int* p = malloc(10);
  bool VisitVarDecl(VarDecl *D) {
    if (D->hasInit()) {
      CheckAllocation(D, D->getInit());
    }
    return true;
  }

  // p = new int;
  bool VisitBinaryOperator(BinaryOperator *BO) {
    if (BO->isAssignmentOp()) {
      if (const VarDecl *VD = GetVarDeclFromExpr(BO->getLHS())) {
        CheckAllocation(VD, BO->getRHS());
      }
    }
    return true;
  }

  // Поиск освобождения и обработки RETURN
  bool VisitStmt(Stmt *S) {
    if (!S)
      return true;

    // Проверка free/fclose
    if (auto *CE = dyn_cast<CallExpr>(S)) {
      if (auto *FD = CE->getDirectCallee()) {
        std::string Name = FD->getNameAsString();
        if ((Name == "free" || Name == "fclose") && CE->getNumArgs() > 0) {
          if (const VarDecl *VD = GetVarDeclFromExpr(CE->getArg(0)))
            AllocatedResources.erase(VD);
        }
      }
    }
    // Проверка delete
    else if (auto *DE = dyn_cast<CXXDeleteExpr>(S)) {
      if (const VarDecl *VD = GetVarDeclFromExpr(DE->getArgument()))
        AllocatedResources.erase(VD);
    }
    // Проверка return
    else if (auto *RS = dyn_cast<ReturnStmt>(S)) {
      for (auto const &[VD, Loc] : AllocatedResources) {
        ReportWarning(VD, true);
      }
      AllocatedResources.clear();
    }
    return true;
  }

private:
  ASTContext *Context;
  // Карта: какая переменная владеет ресурсом -> где этот ресурс был выделен
  std::map<const VarDecl *, SourceLocation> AllocatedResources;

  // Проверяем, является ли выражение выделением ресурса malloc/fopen/new
  void CheckAllocation(const VarDecl *VD, const Expr *E) {
    E = E->IgnoreParenImpCasts();

    if (isa<CXXNewExpr>(E)) {
      AllocatedResources[VD] = VD->getLocation();
    } else if (auto *CE = dyn_cast<CallExpr>(E)) {
      if (auto *FD = CE->getDirectCallee()) {
        std::string Name = FD->getNameAsString();
        if (Name == "malloc" || Name == "fopen")
          AllocatedResources[VD] = VD->getLocation();
      }
    }
  }

  // Для получения переменной из выражения (p, (p), *&p и т.д.)
  const VarDecl *GetVarDeclFromExpr(const Expr *E) {
    E = E->IgnoreParenImpCasts();
    if (auto *DRE = dyn_cast<DeclRefExpr>(E))
      return dyn_cast<VarDecl>(DRE->getDecl());
    return nullptr;
  }

  // Формируем и выводим предупреждение
  void ReportFinalLeaks() {
    for (auto const &[VD, Loc] : AllocatedResources) {
      ReportWarning(VD, false);
    }
    AllocatedResources.clear();
  }

  void ReportWarning(const VarDecl *VD, bool IsEarlyReturn) {
    DiagnosticsEngine &DE = Context->getDiagnostics();
    unsigned DiagID;

    if (IsEarlyReturn) {
      DiagID = DE.getCustomDiagID(
          DiagnosticsEngine::Warning,
          "Ресурс для переменной '%0' может быть не освобожден (не "
          "гарантированное освобождение при return)!");
    } else {
      DiagID = DE.getCustomDiagID(
          DiagnosticsEngine::Warning,
          "Память или ресурс для переменной '%0' не освобождены!");
    }
    DE.Report(VD->getLocation(), DiagID) << VD->getNameAsString();
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
    X("ashihmin_d_analizator", "Leak detector");