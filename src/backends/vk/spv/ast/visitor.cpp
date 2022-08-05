#include "visitor.h"
#include <spv/codegen/builder.h>
#include <spv/codegen/type_caster.h>
#include <vstl/ranges.h>
#include <spv/codegen/variable.h>
namespace toolhub::spv {

Visitor::Visitor(Builder* bd)
	: Component(bd), builtinFunc(bd) {}
void Visitor::Reset(
	luisa::compute::Function kernel) {
	this->kernel = kernel;
	loopBranch = nullptr;
	block.Delete();
	switchIndices.clear();
	varId.Clear();
}
Visitor::~Visitor() {}
void Visitor::visit(const BreakStmt* stmt) {
	bd->Str() << "OpBranch "sv << loopBranch->MergeBlock().ToString() << '\n';
	bd->inBlock = false;
	block.Delete();
}
void Visitor::visit(const ContinueStmt* stmt) {
	bd->Str() << "OpBranch "sv << loopBranch->ContinueBlock().ToString() << '\n';
	bd->inBlock = false;
	block.Delete();
}
ExprValue Visitor::Accept(Expression const* ptr) {
	ExprVisitorWrapper wrapper;
	wrapper.visitor = this;
	ExprValue retId;
	wrapper.retId = &retId;
	ptr->accept(wrapper);
	return retId;
}
Id Visitor::ReadAccept(Type const* type, Id valueId) {
	auto valueType = bd->GetTypeId(type, PointerUsage::NotPointer);
	Id newId(bd->NewId());
	bd->Str() << newId.ToString() << " = OpLoad "sv << valueType.ToString() << ' ' << valueId.ToString() << '\n';
	return newId;
}

Id Visitor::ReadAccept(Expression const* ptr) {
	ExprValue expr = Accept(ptr);
	if (expr.usage == PointerUsage::NotPointer) return expr.valueId;
	return ReadAccept(ptr->type(), expr.valueId);
}
void Visitor::visit(const ReturnStmt* stmt) {
	if (stmt->expression() == nullptr) {
		bd->Str() << "OpBranch "sv << func->GetReturnBranch().ToString() << '\n';
		bd->inBlock = false;
		block.Delete();
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
	auto cond = ReadAccept(stmt->condition());
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
	Accept(stmt->expression());
}
void Visitor::visit(const SwitchStmt* stmt) {
	auto cond = ReadAccept(stmt->expression());
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
	Accept(stmt->variable());
	ForLoop forLoop(bd, [&] {
		return ReadAccept(stmt->condition());
	});
	this->loopBranch = &forLoop;
	block.New(bd, forLoop.LoopBlock(), forLoop.afterLoopBlock);
	stmt->body()->accept(*this);
	Block afterLoopBlock(bd, forLoop.afterLoopBlock, forLoop.MergeBlock());
	Accept(stmt->step());
}
void Visitor::visit(const AssignStmt* stmt) {
	auto varValue = Accept(stmt->lhs());
	auto readValue = ReadAccept(stmt->rhs());
	bd->Str() << "OpStore "sv << varValue.valueId.ToString() << ' ' << readValue.ToString() << '\n';
}
void Visitor::visit(const CommentStmt* stmt) {}
void Visitor::RegistVariables(
	vstd::span<luisa::compute::Variable const> variables,
	vstd::span<luisa::compute::Variable const> builtinVariables) {
	using Tag = luisa::compute::Variable::Tag;
	auto LoadOrdinaryArgs = [&] {
		for (auto&& i : variables) {
			PointerUsage usage;
			switch (i.tag()) {
				case Tag::LOCAL:
				case Tag::BUFFER:
				case Tag::TEXTURE:
				case Tag::BINDLESS_ARRAY:
				case Tag::ACCEL:
				case Tag::REFERENCE:
					usage = PointerUsage::Function;
					break;
				case Tag::SHARED:
					usage = PointerUsage::Workgroup;
					break;
				default:
					return;
			}
			varId.Emplace(
				i.uid(),
				vstd::LazyEval([&] {
					return Variable(bd, i.type(), usage);
				}));
		}
		for (auto&& i : builtinVariables) {
			switch (i.tag()) {
				case Tag::DISPATCH_ID:
					varId.Emplace(i.uid(), dispatchedThreadId);
					return;
				case Tag::THREAD_ID:
					varId.Emplace(i.uid(), threadId);
					return;
				case Tag::BLOCK_ID:
					varId.Emplace(i.uid(), groupId);
					return;
				case Tag::DISPATCH_SIZE:
					varId.Emplace(i.uid(), dispatchedCountId);
					return;
			}
		}
	};
	if (kernel.tag() == luisa::compute::Function::Tag::KERNEL) {
		for (auto&& i : kernel.arguments()) {
			switch (i.tag()) {
				case Tag::LOCAL:
				case Tag::REFERENCE: {
					if ((static_cast<uint>(kernel.variable_usage(i.uid())) & static_cast<uint>(Usage::WRITE)) == 0) {
						Id newVarValue = bd->NewId();
						varId.Emplace(i.uid(), newVarValue);
					} else {
						varId.Emplace(
							i.uid(),
							vstd::LazyEval([&] {
								return Variable(bd, i.type(), PointerUsage::Function);
							}));
					}
					break;
				}
			}
		}
		threadId = bd->NewId();
		groupId = bd->NewId();
		dispatchedThreadId = bd->NewId();
		dispatchedCountId = bd->NewId();
		LoadOrdinaryArgs();
		Id dispatchCountVar = bd->NewId();
		auto uint3Id = Id::VecId(Id::UIntId(), 3).ToString();
		auto uint3UniformPtr = bd->GetTypeId(InternalType(InternalType::Tag::UINT, 3), PointerUsage::Uniform).ToString();
		bd->Str()
			<< threadId.ToString() << " = OpLoad "sv << vstd::string_view(uint3Id) << ' ' << Id::GroupThreadId().ToString() << '\n'
			<< groupId.ToString() << " = OpLoad "sv << vstd::string_view(uint3Id) << ' ' << Id::GroupId().ToString() << '\n'
			<< dispatchedThreadId.ToString() << " = OpLoad "sv << vstd::string_view(uint3Id) << ' ' << Id::DispatchThreadId().ToString() << '\n'
			<< dispatchCountVar.ToString()
			<< " = OpAccessChain "sv
			<< uint3UniformPtr << ' '			 // arg type
			<< bd->CBufferVar().ToString() << ' '// buffer
			<< Id::ZeroId().ToString() << ' '	 // first buffer
			<< Id::ZeroId().ToString() << ' '	 // first element
			<< Id::ZeroId().ToString() << '\n'	 // first member
			<< dispatchedCountId.ToString()
			<< " = OpLoad "sv
			<< vstd::string_view(uint3Id) << ' '
			<< dispatchCountVar.ToString() << '\n';

		uint cbufferIndex = 1;
		for (auto&& i : kernel.arguments()) {
			switch (i.tag()) {
				case Tag::LOCAL:
				case Tag::REFERENCE: {
					Id newVar = bd->NewId();
					auto ite = varId.Find(i.uid());
					if (!ite) break;
					auto LoadVar = [&](Id newVarValue) {
						bd->Str()
							<< newVar.ToString()
							<< " = OpAccessChain "sv
							<< bd->GetTypeId(i.type(), PointerUsage::Uniform).ToString() << ' '// arg type
							<< bd->CBufferVar().ToString() << ' '							   // buffer
							<< Id::ZeroId().ToString() << ' '								   // first buffer
							<< Id::ZeroId().ToString() << ' '								   // first element
							<< bd->GetConstId(cbufferIndex).ToString() << '\n'
							<< newVarValue.ToString()
							<< " = OpLoad "sv
							<< bd->GetTypeId(i.type(), PointerUsage::NotPointer).ToString() << ' '// arg type
							<< newVar.ToString() << '\n';
						++cbufferIndex;
					};
					ite.Value().multi_visit(
						[&](Variable const& var) {
							Id loadId(bd->NewId());
							LoadVar(loadId);
							var.Store(loadId);
						},
						[&](Id id) {
							LoadVar(id);
						});
				} break;
			}
		}
	} else {
		size_t argIndex = 0;
		for (auto&& i : kernel.arguments()) {
			switch (i.tag()) {
				case Tag::LOCAL:
				case Tag::REFERENCE: {
					if ((static_cast<uint>(kernel.variable_usage(i.uid())) & static_cast<uint>(Usage::WRITE)) == 0) {
						varId.Emplace(i.uid(), func->argValues[argIndex]);
					} else {
						varId.Emplace(
							i.uid(),
							vstd::LazyEval([&] {
								auto var = Variable(bd, i.type(), PointerUsage::Function);
								var.Store(func->argValues[argIndex]);
								return var;
							}));
					}
					break;
				}
			}
			++argIndex;
		}
		LoadOrdinaryArgs();
	}
}
void Visitor::visit(const MetaStmt* stmt) {
	block.New(bd, func->funcBlockId);
	RegistVariables(stmt->variables(), kernel.builtin_variables());
	CullRedundentThread(stmt->scope());
}

ExprValue Visitor::visit(const UnaryExpr* expr) {
	if (expr->op() == UnaryOp::PLUS) return Accept(expr->operand());
	auto dstNewId = bd->NewId();
	auto dstType = bd->GetTypeId(expr->type(), PointerUsage::NotPointer);
	auto operandValue = ReadAccept(expr->operand());
	auto PrintOperator = [&](auto&& getDstTypeAndOp) -> void {
		auto dstTypeAndOp = getDstTypeAndOp();
		bd->Str() << dstNewId.ToString() << dstTypeAndOp.first << dstTypeAndOp.second.ToString() << ' ' << operandValue.ToString() << '\n';
	};
	auto TypeId = [&](InternalType const* operandType, Id scalarId) {
		return (operandType->Size() > 1) ? Id::VecId(scalarId, operandType->Size()) : scalarId;
	};
	switch (expr->op()) {
		case UnaryOp::BIT_NOT:
			PrintOperator(
				[&] -> std::pair<vstd::string_view, Id> {
					return {" = OpNot "sv, operandValue};
				});
			break;
		case UnaryOp::NOT:
			PrintOperator(
				[&] -> std::pair<vstd::string_view, Id> {
					return {" = OpLogicalNot "sv, operandValue};
				});
			break;
		case UnaryOp::MINUS: {
			auto operandType = InternalType::GetType(expr->operand()->type());
			assert(operandType);
			PrintOperator(
				[&] -> std::pair<vstd::string_view, Id> {
					switch (operandType->tag) {
						case InternalType::Tag::FLOAT:
							return {" = OpFNegate "sv, TypeId(operandType, Id::FloatId())};
						case InternalType::Tag::INT:
							return {" = OpSNegate "sv, TypeId(operandType, Id::IntId())};
						default:
							assert(false);
					}
				});
		} break;
	}
	return {dstNewId, PointerUsage::NotPointer};
}
ExprValue Visitor::visit(const BinaryExpr* expr) {
	auto dstNewId = bd->NewId();
	auto dstType = bd->GetTypeId(expr->type(), PointerUsage::NotPointer);
	auto tarType = InternalType::GetType(expr->type());
	auto printOp = [&](InternalType dstType, Id leftValue, Id rightValue, vstd::string_view opName) {
		bd->Str() << dstNewId.ToString() << " = "sv << opName << ' ' << bd->GetTypeId(dstType, PointerUsage::NotPointer).ToString() << ' ' << leftValue.ToString() << ' ' << rightValue.ToString() << '\n';
	};
	auto PrintSameTypeOp = [&](vstd::string_view op) {
		auto leftType = InternalType::GetType(expr->lhs()->type());
		auto rightType = InternalType::GetType(expr->rhs()->type());
		assert(leftType && rightType && tarType);
		auto rightTransform = *rightType != *tarType;

		auto TryCast = [&](InternalType type, Expression const* expr) {
			return TypeCaster::TryCast(bd, type, *tarType, ReadAccept(expr));
		};
		printOp(*tarType, TryCast(*leftType, expr->lhs()), TryCast(*rightType, expr->rhs()), op);
	};
	auto FloatOrInt = [&](InternalType type, vstd::string_view floatOp, vstd::string_view intOp) -> vstd::string_view {
		switch (type.tag) {
			case InternalType::Tag::FLOAT:
			case InternalType::Tag::MATRIX:
				return floatOp;
			case InternalType::Tag::INT:
			case InternalType::Tag::UINT:
				return intOp;
			default:
				assert(false);
				break;
		}
	};
	switch (expr->op()) {
		case BinaryOp::ADD: {
			PrintSameTypeOp(FloatOrInt(*tarType, "OpFAdd"sv, "OpIAdd"sv));
		} break;
		case BinaryOp::SUB: {
			PrintSameTypeOp(FloatOrInt(*tarType, "OpFSub"sv, "OpISub"sv));
		} break;
		case BinaryOp::DIV:
			PrintSameTypeOp([&] {
				switch (tarType->tag) {
					case InternalType::Tag::FLOAT:
					case InternalType::Tag::MATRIX:
						return "OpFDiv"sv;
					case InternalType::Tag::INT:
						return "OpSDiv"sv;
					case InternalType::Tag::UINT:
						return "OpUDiv"sv;
				}
			}());
			break;
		case BinaryOp::MOD: {
			PrintSameTypeOp([&] {
				switch (tarType->tag) {
					case InternalType::Tag::FLOAT:
					case InternalType::Tag::MATRIX:
						return "OpFMod"sv;
					case InternalType::Tag::INT:
						return "OpSMod"sv;
					case InternalType::Tag::UINT:
						return "OpUMod"sv;
				}
			}());
		} break;
		case BinaryOp::BIT_AND: {
			PrintSameTypeOp("OpBitwiseAnd"sv);
		} break;
		case BinaryOp::BIT_OR: {
			PrintSameTypeOp("OpBitwiseOr"sv);
		} break;
		case BinaryOp::BIT_XOR: {
			PrintSameTypeOp("OpBitwiseXor"sv);
		} break;
		case BinaryOp::SHL:
		case BinaryOp::SHR: {
			auto rightType = InternalType::GetType(expr->rhs()->type());
			assert(rightType);
			auto unsignedType = *tarType;
			unsignedType.tag == InternalType::Tag::UINT;
			printOp(*tarType, ReadAccept(expr->lhs()), TypeCaster::TryCast(bd, *rightType, unsignedType, ReadAccept(expr->rhs())), expr->op() == BinaryOp::SHR ? "OpShiftRightLogical"sv : "OpShiftLeftLogical"sv);
		} break;
		case BinaryOp::AND: {
			PrintSameTypeOp("OpLogicalAnd"sv);
		} break;
		case BinaryOp::OR: {
			PrintSameTypeOp("OpLogicalOr"sv);
		} break;
		case BinaryOp::LESS: {
			PrintSameTypeOp([&] {
				switch (tarType->tag) {
					case InternalType::Tag::FLOAT:
					case InternalType::Tag::MATRIX:
						return "OpFUnordLessThan"sv;
					case InternalType::Tag::INT:
						return "OpSLessThan"sv;
					case InternalType::Tag::UINT:
						return "OpULessThan"sv;
				}
			}());
			//TODO
		} break;
		case BinaryOp::GREATER: {
			PrintSameTypeOp([&] {
				switch (tarType->tag) {
					case InternalType::Tag::FLOAT:
					case InternalType::Tag::MATRIX:
						return "OpFUnordGreaterThan"sv;
					case InternalType::Tag::INT:
						return "OpSGreaterThan"sv;
					case InternalType::Tag::UINT:
						return "OpUGreaterThan"sv;
				}
			}());
		} break;
		case BinaryOp::LESS_EQUAL: {
			PrintSameTypeOp([&] {
				switch (tarType->tag) {
					case InternalType::Tag::FLOAT:
					case InternalType::Tag::MATRIX:
						return "OpFUnordLessThanEqual"sv;
					case InternalType::Tag::INT:
						return "OpSLessThanEqual"sv;
					case InternalType::Tag::UINT:
						return "OpULessThanEqual"sv;
				}
			}());
			//TODO
		} break;
		case BinaryOp::GREATER_EQUAL: {
			PrintSameTypeOp([&] {
				switch (tarType->tag) {
					case InternalType::Tag::FLOAT:
					case InternalType::Tag::MATRIX:
						return "OpFUnordGreaterThanEqual"sv;
					case InternalType::Tag::INT:
						return "OpSGreaterThanEqual"sv;
					case InternalType::Tag::UINT:
						return "OpUGreaterThanEqual"sv;
				}
			}());
		} break;
		case BinaryOp::EQUAL: {
			PrintSameTypeOp([&] {
				switch (tarType->tag) {
					case InternalType::Tag::FLOAT:
					case InternalType::Tag::MATRIX:
						return "OpFUnordEqual"sv;
					case InternalType::Tag::INT:
					case InternalType::Tag::UINT:
						return "OpIEqual"sv;
				}
			}());
		} break;
		case BinaryOp::NOT_EQUAL: {
			PrintSameTypeOp([&] {
				switch (tarType->tag) {
					case InternalType::Tag::FLOAT:
					case InternalType::Tag::MATRIX:
						return "OpFUnordNotEqual"sv;
					case InternalType::Tag::INT:
					case InternalType::Tag::UINT:
						return "OpINotEqual"sv;
				}
			}());
		} break;
		case BinaryOp::MUL: {
			// OpVectorTimesScalar
			auto lhsType = expr->lhs()->type();
			auto rhsType = expr->rhs()->type();
			if (lhsType->is_vector() && rhsType->is_scalar()) {
				auto lhsInternalType = InternalType::GetType(lhsType);
				bd->Str()
					<< dstNewId.ToString()
					<< " = OpVectorTimesScalar "sv
					<< lhsInternalType->TypeId().ToString()
					<< ' '
					<< ReadAccept(expr->lhs()).ToString()
					<< ' ' << ReadAccept(expr->rhs()).ToString()
					<< '\n';

			}
			//OpMatrixTimesScalar
			else if (lhsType->is_matrix() && rhsType->is_scalar()) {
				auto lhsInternalType = InternalType::GetType(lhsType);
				bd->Str()
					<< dstNewId.ToString()
					<< " = OpMatrixTimesScalar "sv
					<< lhsInternalType->TypeId().ToString()
					<< ' '
					<< ReadAccept(expr->lhs()).ToString()
					<< ' ' << ReadAccept(expr->rhs()).ToString()
					<< '\n';

			}
			//OpVectorTimesMatrix
			else if (lhsType->is_vector() && rhsType->is_matrix()) {
				auto rhsInternalType = InternalType::GetType(rhsType);
				bd->Str()
					<< dstNewId.ToString()
					<< " = OpVectorTimesMatrix "sv
					<< rhsInternalType->TypeId().ToString()
					<< ' ' << ReadAccept(expr->lhs()).ToString()
					<< ' ' << ReadAccept(expr->rhs()).ToString()
					<< '\n';
			}
			//OpMatrixTimesVector
			else if (lhsType->is_matrix() && rhsType->is_vector()) {
				auto lhsInternalType = InternalType::GetType(lhsType);
				bd->Str()
					<< dstNewId.ToString()
					<< " = OpMatrixTimesVector "sv
					<< lhsInternalType->TypeId().ToString()
					<< ' ' << ReadAccept(expr->lhs()).ToString()
					<< ' ' << ReadAccept(expr->rhs()).ToString()
					<< '\n';
			}
			//OpMatrixTimesMatrix
			else if (lhsType->is_matrix() && rhsType->is_matrix()) {
				auto lhsInternalType = InternalType::GetType(lhsType);
				bd->Str()
					<< dstNewId.ToString()
					<< " = OpMatrixTimesMatrix "sv
					<< lhsInternalType->TypeId().ToString()
					<< ' ' << ReadAccept(expr->lhs()).ToString()
					<< ' ' << ReadAccept(expr->rhs()).ToString()
					<< '\n';
			} else {
				PrintSameTypeOp(FloatOrInt(*tarType, "OpFMul"sv, "OpIMul"sv));
			}
		} break;
	}
	return {dstNewId, PointerUsage::NotPointer};
}
ExprValue Visitor::visit(const MemberExpr* expr) {
	using TypeTag = luisa::compute::Type::Tag;
	auto selfType = expr->self()->type();
	auto value = Accept(expr->self());
	Variable var(bd, selfType, value.valueId, value.usage);
	if (expr->is_swizzle()) {
		if (expr->swizzle_size() > 1) {
			auto swizzleRange = vstd::IRangeImpl(
				vstd::CacheEndRange(vstd::range(expr->swizzle_size())) |
				vstd::TransformRange([&](size_t idx) -> uint { return expr->swizzle_index(idx); }));
			return {var.Swizzle(&swizzleRange), PointerUsage::NotPointer};
		} else {
			auto id = var.AccessVectorElement(expr->swizzle_index(0));
			return {id, var.usage};
		}
	} else {
		switch (selfType->tag()) {
			case TypeTag::MATRIX:
				return {var.AccessMatrixCol(bd->GetConstId((uint)expr->member_index())), value.usage};
			case TypeTag::STRUCTURE:
				return {var.AccessMember(expr->member_index()), value.usage};
			case TypeTag::VECTOR:
				return {var.AccessVectorElement(expr->member_index()), value.usage};
			default:
				assert(false);
		}
	}
}
ExprValue Visitor::visit(const AccessExpr* expr) {
	auto value = Accept(expr->range());
	auto num = ReadAccept(expr->index());
	Variable var(bd, expr->type(), value.valueId, value.usage);
	auto newId = [&] {
		switch (expr->type()->tag()) {
			case Type::Tag::ARRAY:
				return var.AccessArrayEle(num);
			case Type::Tag::BUFFER:
				return var.AccessBufferEle(Id::ZeroId(), num);
		}
	}();
	return {newId, value.usage};
}
ExprValue Visitor::visit(const LiteralExpr* expr) {
	return {bd->GetConstId(expr->value()), PointerUsage::NotPointer};
}
ExprValue Visitor::visit(const RefExpr* expr) {
	using VarTag = luisa::compute::Variable::Tag;
	auto var = expr->variable();
	auto ite = varId.Find(var.uid());
	assert(ite);
	auto& v = ite.Value();
	return v.multi_visit_or(
		vstd::UndefEval<ExprValue>{},
		[&](Variable const& v) -> ExprValue { return {v.varId, v.usage}; },
		[&](Id v) -> ExprValue { return {v, PointerUsage::NotPointer}; });
}
ExprValue Visitor::visit(const ConstantExpr* expr) {
	Id arrValueId = bd->GetConstArrayId(expr->data(), expr->type());
	return {
		arrValueId,
		PointerUsage::UniformConstant};
}
ExprValue Visitor::visit(const CallExpr* expr) {
	if (expr->is_builtin()) {
		auto range = vstd::IRangeImpl(
			vstd::CacheEndRange(expr->arguments()) |
			vstd::TransformRange([&](Expression const* expr) -> Variable {
				auto exprValue = Accept(expr);
				return Variable(bd, expr->type(), exprValue.valueId, exprValue.usage);
			}));
		return {builtinFunc.CallFunc(expr->type(), expr->op(), range), PointerUsage::NotPointer};
	} else {
		auto builder = bd->Str();
		Id newId;
		if (expr->type()) {
			newId = bd->NewId();
			builder << newId.ToString() << " = OpFunctionCall "sv << bd->GetTypeId(expr->type(), PointerUsage::NotPointer).ToString();
		} else {
			builder << "OpFunctionCall "sv << Id::VoidId().ToString();
		}
		auto argRefl = expr->custom().arguments();
		auto args = expr->arguments();
		for (auto i : vstd::range(argRefl.size())) {
			auto exprValue = Accept(args[i]);
			if (argRefl[i].tag() != luisa::compute::Variable::Tag::REFERENCE &&
				exprValue.usage != PointerUsage::NotPointer) {
				exprValue.valueId = ReadAccept(argRefl[i].type(), exprValue.valueId);
			}
			builder << ' ' << exprValue.valueId.ToString();
		}
		return {newId, PointerUsage::NotPointer};
	}
}
void Visitor::CullRedundentThread(ScopeStmt const* scope) {
	Id threadCompareVec(bd->NewId());
	Id threadCompareScalar(bd->NewId());
	bd->Str()
		<< threadCompareVec.ToString() << " = OpULessThan "sv
		<< bd->GetTypeId(InternalType(InternalType::Tag::BOOL, 3), PointerUsage::NotPointer).ToString() << ' '
		<< dispatchedThreadId.ToString() << ' '
		<< dispatchedCountId.ToString() << '\n'
		<< threadCompareScalar.ToString() << " = OpAll "sv
		<< bd->GetTypeId(InternalType(InternalType::Tag::BOOL, 1), PointerUsage::NotPointer).ToString() << ' '
		<< threadCompareVec.ToString() << '\n';
	IfBranch branch(bd, threadCompareScalar);
	{
		Block trueBlock(bd, branch.trueBranch, branch.mergeBlock);
		scope->accept(*this);
	}
	Block falseBlock(bd, branch.falseBranch, branch.mergeBlock);
}

ExprValue Visitor::visit(const CastExpr* expr) {
	auto value = ReadAccept(expr->expression());
	auto srcType = InternalType::GetType(expr->expression()->type());
	auto dstType = InternalType::GetType(expr->type());
	assert(srcType && dstType);
	if (srcType->tag == dstType->tag && srcType->dimension == dstType->dimension) {
		return {value, PointerUsage::NotPointer};
	}
	return {TypeCaster::TryCast(bd, *srcType, *dstType, value), PointerUsage::NotPointer};
	//TODO
}
}// namespace toolhub::spv