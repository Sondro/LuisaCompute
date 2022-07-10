#include "visitor.h"
#include <spv/codegen/builder.h>
namespace toolhub::spv {
Visitor::Visitor(Builder* bd, Function* func)
	: Component(bd), func(func) {}
Visitor::~Visitor() {}
void Visitor::visit(const BreakStmt* stmt) {
	bd->Str() << "OpBranch "sv << loopBranch->MergeBlock().ToString() << '\n';
	bd->inBlock = false;
}
void Visitor::visit(const ContinueStmt* stmt) {
	bd->Str() << "OpBranch "sv << loopBranch->ContinueBlock().ToString() << '\n';
	bd->inBlock = false;
}
template<typename T>
requires(detail::IsExpression<T>)
	Id Visitor::Accept(T ptr, Usage usage) {
	lastUsage = usage;
	ptr->accept(*this);
	return lastReturnId;
}
void Visitor::visit(const ReturnStmt* stmt) {
	if (stmt->expression() == nullptr) {
		bd->Str() << "OpBranch "sv << func->GetReturnBranch().ToString() << '\n';
		bd->inBlock = false;
	}
}
void Visitor::visit(const ScopeStmt* stmt) {
	for (auto&& i : stmt->statements()) {
		if (!bd->inBlock) break;
		i->accept(*this);
	}
	block.Delete();
}
void Visitor::visit(const IfStmt* stmt) {
	auto cond = Accept(stmt->condition(), Usage::Read);
	IfBranch branch(bd, cond);
	{
		block.New(bd, branch.trueBranch, branch.mergeBlock);
		stmt->true_branch()->accept(*this);
	}
	{
		block.New(bd, branch.falseBranch, branch.mergeBlock);
		stmt->false_branch()->accept(*this);
	}
}
void Visitor::visit(const LoopStmt* stmt) {
	stmt->body()->accept(*this);
}
void Visitor::visit(const ExprStmt* stmt) {
	stmt->expression()->accept(*this);
}
void Visitor::visit(const SwitchStmt* stmt) {
	auto cond = Accept(stmt->expression(), Usage::Read);
	// should be switchcase stmt
	// kill Luisa if it's not
	switchIndices.clear();
	bool useDefault = false;
	auto cases = stmt->body()->statements();
	switchIndices.reserve(cases.size());
	for (auto&& i : cases) {
		if (i->tag() == Statement::Tag::SWITCH_CASE) {
			auto caseStmt = static_cast<SwitchCaseStmt const*>(i);
			auto literal = static_cast<LiteralExpr const*>(caseStmt->expression());
			luisa::visit(
				[&]<typename T>(T const& v) {
					if constexpr (std::is_constructible_v<int32, T>) {
						switchIndices.emplace_back(v);
					}
				},
				literal->value());
		} else {
			auto defaultStmt = static_cast<SwitchDefaultStmt const*>(i);
			useDefault = true;
		}
	}
	SwitchCase switcher(bd, cond, switchIndices, useDefault);
	Id const* caseIds = switcher.caseId.data();

	for (auto&& i : cases) {
		if (i->tag() == Statement::Tag::SWITCH_CASE) {
			block.New(bd, *caseIds, switcher.mergeBlock);
			i->accept(*this);
			++caseIds;
		} else {
			block.New(bd, switcher.defaultBlock, switcher.mergeBlock);
			i->accept(*this);
			++caseIds;
		}
	}
}
void Visitor::visit(const SwitchCaseStmt* stmt) {
	stmt->body()->accept(*this);
}
void Visitor::visit(const SwitchDefaultStmt* stmt) {
	stmt->body()->accept(*this);
}
void Visitor::visit(const ForStmt* stmt) {
	Accept(stmt->variable(), Usage::None);
	auto& var = *lastVar;
	ForLoop forLoop(bd, [&]{
		return Accept(stmt->condition(), Usage::Read);
	});
	this->loopBranch = &forLoop;
	block.New(bd, forLoop.LoopBlock(), forLoop.afterLoopBlock);
	stmt->body()->accept(*this);
	Block afterLoopBlock(bd, forLoop.afterLoopBlock, forLoop.MergeBlock());
	Accept(stmt->step(), Usage::None);
}
void Visitor::visit(const AssignStmt* stmt) {}
void Visitor::visit(const CommentStmt* stmt) {}
void Visitor::visit(const MetaStmt* stmt) {}

void Visitor::visit(const UnaryExpr* expr) {}
void Visitor::visit(const BinaryExpr* expr) {}
void Visitor::visit(const MemberExpr* expr) {}
void Visitor::visit(const AccessExpr* expr) {}
void Visitor::visit(const LiteralExpr* expr) {}
void Visitor::visit(const RefExpr* expr) {}
void Visitor::visit(const ConstantExpr* expr) {}
void Visitor::visit(const CallExpr* expr) {}
void Visitor::visit(const CastExpr* expr) {}
}// namespace toolhub::spv