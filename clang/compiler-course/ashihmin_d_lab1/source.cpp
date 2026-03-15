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

  // 1. int* p = malloc(10);
  bool VisitVarDecl(VarDecl *D) {
    if (D->hasInit()) {
      CheckAllocation(D, D->getInit());
    }
    return true;
  }

  // 2. p = new int;
  bool VisitBinaryOperator(BinaryOperator *BO) {
    if (BO->isAssignmentOp()) {
      if (const VarDecl *VD = GetVarDeclFromExpr(BO->getLHS())) {
        CheckAllocation(VD, BO->getRHS());
      }
    }
    return true;
  }

  // 3. free(p), fclose(f)
  bool VisitCallExpr(CallExpr *CE) {
    if (FunctionDecl *FD = CE->getDirectCallee()) {
      std::string Name = FD->getNameAsString();
      if (Name == "free" || Name == "fclose") {
        if (CE->getNumArgs() > 0) {
          if (const VarDecl *VD = GetVarDeclFromExpr(CE->getArg(0))) {
            AllocatedResources.erase(VD);
          }
        }
      }
    }
    return true;
  }

  // 4. delete p;
  bool VisitCXXDeleteExpr(CXXDeleteExpr *DE) {
    if (const VarDecl *VD = GetVarDeclFromExpr(DE->getArgument())) {
      AllocatedResources.erase(VD);
    }
    return true;
  }

  // В конце работы вызывается этот метод для вывода ошибок
  void ReportLeaks() {
    DiagnosticsEngine &DE = Context->getDiagnostics();
    unsigned DiagID = DE.getCustomDiagID(
        DiagnosticsEngine::Warning,
        "Ресурс для переменной '%0' выделен, но не гарантированно освобожден!");

    for (auto const &[VD, Loc] : AllocatedResources) {
      DE.Report(VD->getLocation(), DiagID) << VD->getNameAsString();
      unsigned NoteID = DE.getCustomDiagID(DiagnosticsEngine::Note,
                                           "Место выделения ресурса:");
      DE.Report(Loc, NoteID);
    }
  }

private:
  ASTContext *Context;
  // Карта: какая переменная владеет ресурсом -> где этот ресурс был выделен
  // (Loc)
  std::map<const VarDecl *, SourceLocation> AllocatedResources;

  // Проверяем, является ли выражение выделением ресурса
  void CheckAllocation(const VarDecl *VD, const Expr *Init) {
    Init = Init->IgnoreParenImpCasts();

    // Проверка на new
    if (isa<CXXNewExpr>(Init)) {
      AllocatedResources[VD] = Init->getBeginLoc();
    }
    // Проверка на malloc/fopen
    else if (const CallExpr *CE = dyn_cast<CallExpr>(Init)) {
      if (const FunctionDecl *FD = CE->getDirectCallee()) {
        std::string Name = FD->getNameAsString();
        if (Name == "malloc" || Name == "fopen") {
          AllocatedResources[VD] = Init->getBeginLoc();
        }
      }
    }
  }

  // Для получения переменной из выражения (p, (p), *&p и т.д.)
  const VarDecl *GetVarDeclFromExpr(const Expr *E) {
    E = E->IgnoreParenImpCasts();
    if (const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(E)) {
      return dyn_cast<VarDecl>(DRE->getDecl());
    }
    return nullptr;
  }
};

class ResourceLeakConsumer : public ASTConsumer {
public:
  explicit ResourceLeakConsumer(ASTContext *Context) : Visitor(Context) {}

  void HandleTranslationUnit(ASTContext &Context) override {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    Visitor.ReportLeaks();
  }

private:
  ResourceLeakVisitor Visitor;
};

class ResourceLeakAction : public PluginASTAction {
public:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef) override {
    return std::make_unique<ResourceLeakConsumer>(&CI.getASTContext());
  }
  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &args) override {
    return true;
  }
};

} // namespace

static FrontendPluginRegistry::Add<ResourceLeakAction>
    X("ashihmin_d_analizator",
      "Detect missing deallocation for new/malloc/fopen");