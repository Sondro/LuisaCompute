#pragma once
#include <spv/codegen/types.h>
#include <ast/statement.h>
#include <ast/expression.h>
#include <spv/codegen/if_branch.h>
#include <spv/codegen/for_loop.h>
#include <spv/codegen/switch_case.h>
#include <spv/codegen/function.h>
#include <spv/codegen/variable.h>
#include <spv/codegen/internal_type.h>
#include <spv/codegen/builtin_func.h>
namespace toolhub::spv {
struct ExprValue {
	Id valueId;
	PointerUsage usage;
};
class Visitor : public Component, public StmtVisitor {
public:
	ExprValue Accept(Expression const* ptr);
	Id ReadAccept(Expression const* ptr);
	Id ReadAccept(Type const* type, Id valueId);
	/////////////////// function cache
	vstd::HashMap<uint, vstd::variant<Variable, Id>> varId;
	/////////////////// statement cache
	WhileLoop* loopBranch;
	Function* func;
	luisa::compute::Function kernel;
	vstd::optional<Block> block;
	vstd::small_vector<int32> switchIndices;
	Id threadId;
	Id groupId;
	Id dispatchedThreadId;
	Id dispatchedCountId;
	/////////////////// return values
	uint3 kernelGroupSize;
	BuiltinFunc builtinFunc;

	Visitor(Builder* bd);
	void Reset(
		luisa::compute::Function kernel);
	~Visitor();
	void visit(const BreakStmt* stmt) override;
	void visit(const ContinueStmt* stmt) override;
	void visit(const ReturnStmt* stmt) override;
	void visit(const ScopeStmt* stmt) override;
	void visit(const IfStmt* stmt) override;
	void visit(const LoopStmt* stmt) override;
	void visit(const ExprStmt* stmt) override;
	void visit(const SwitchStmt* stmt) override;
	void visit(const SwitchCaseStmt* stmt) override;
	void visit(const SwitchDefaultStmt* stmt) override;
	void visit(const AssignStmt* stmt) override;
	void visit(const ForStmt* stmt) override;
	void visit(const CommentStmt* stmt) override;
	void visit(const MetaStmt* stmt) override;
	void RegistVariables(
		vstd::span<luisa::compute::Variable const> variables,
		vstd::span<luisa::compute::Variable const> builtinVariables);
	void CullRedundentThread(ScopeStmt const* scope);

	ExprValue visit(const UnaryExpr* expr);
	ExprValue visit(const BinaryExpr* expr);
	ExprValue visit(const MemberExpr* expr);
	ExprValue visit(const AccessExpr* expr);
	ExprValue visit(const LiteralExpr* expr);
	ExprValue visit(const RefExpr* expr);
	ExprValue visit(const ConstantExpr* expr);
	ExprValue visit(const CallExpr* expr);
	ExprValue visit(const CastExpr* expr);
};
class ExprVisitorWrapper : public ExprVisitor {
public:
	Visitor* visitor;
	ExprValue* retId;
#define IMPL_VISITOR(Type) \
	void visit(const Type* expr) override { *retId = visitor->visit(expr); }
	IMPL_VISITOR(UnaryExpr)
	IMPL_VISITOR(BinaryExpr)
	IMPL_VISITOR(MemberExpr)
	IMPL_VISITOR(AccessExpr)
	IMPL_VISITOR(LiteralExpr)
	IMPL_VISITOR(RefExpr)
	IMPL_VISITOR(ConstantExpr)
	IMPL_VISITOR(CallExpr)
	IMPL_VISITOR(CastExpr)
#undef IMPL_VISITOR
};
}// namespace toolhub::spv