#pragma once
#include <spv/codegen/types.h>
#include <ast/statement.h>
#include <ast/expression.h>
#include <spv/codegen/if_branch.h>
#include <spv/codegen/for_loop.h>
#include <spv/codegen/switch_case.h>
#include <spv/codegen/function.h>
namespace toolhub::spv {
namespace detail {
template<typename T>
static constexpr bool IsExpression = std::is_base_of_v<Expression, std::remove_cvref_t<std::remove_pointer_t<T>>>;
}
class Visitor : public Component, public StmtVisitor, public ExprVisitor {
	enum class Usage : uint {
		None,
		Read,
		Write
	};
	template<typename T>
	requires(detail::IsExpression<T>)
		Id Accept(T ptr, Usage usage);
	/////////////////// function cache
	vstd::HashMap<uint, Variable> varId;
	/////////////////// statement cache
	WhileLoop* loopBranch;
	Function* func;
	vstd::optional<Block> block;
	vstd::small_vector<int32> switchIndices;
	/////////////////// return values
	Usage lastUsage;
	Id lastReturnId;
	Variable* lastVar;

public:
	Visitor(Builder* bd, Function* func);
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

	void visit(const UnaryExpr* expr) override;
	void visit(const BinaryExpr* expr) override;
	void visit(const MemberExpr* expr) override;
	void visit(const AccessExpr* expr) override;
	void visit(const LiteralExpr* expr) override;
	void visit(const RefExpr* expr) override;
	void visit(const ConstantExpr* expr) override;
	void visit(const CallExpr* expr) override;
	void visit(const CastExpr* expr) override;
};
}// namespace toolhub::spv