#include "serialize.h"
#include <vstl/variant_util.h>
#include <ast/function_builder.h>
using namespace toolhub::db;
namespace luisa::compute {
using ReadVar = vstd::VariantVisitor_t<ReadJsonVariant>;
template<typename T, typename... Args>
	requires(std::is_constructible_v<T, Args&&...>)
T* AstSerializer::Alloc(Args&&... args) {
	if constexpr (std::is_base_of_v<Expression, T>) {
		auto ptr = luisa::make_unique<T>(std::forward<Args>(args)...);
		auto retPtr = ptr.get();
		builder->_all_expressions.emplace_back(std::move(ptr));
		return retPtr;
	} else if constexpr (std::is_base_of_v<Statement, T>) {
		auto ptr = luisa::make_unique<T>(std::forward<Args>(args)...);
		auto retPtr = ptr.get();
		builder->_all_statements.emplace_back(std::move(ptr));
		return retPtr;
	} else {
		static_assert(vstd::AlwaysFalse<T>, "illegal type");
	}
}
void AstSerializer::DeserStmtScope(ScopeStmt& value, IJsonArray const* arr) {
	value._statements.reserve(arr->Length());
	for (auto&& i : *arr) {
		auto dict = i.get_or<IJsonDict*>(nullptr);
		assert(dict);
		value._statements.emplace_back(DeserStmt(dict));
	}
}

template<>
BreakStmt* AstSerializer::DeserStmtDerive(IJsonDict const* dict) {
	auto ptr = Alloc<BreakStmt>();
	return ptr;
}
template<>
ContinueStmt* AstSerializer::DeserStmtDerive(IJsonDict const* dict) {
	auto ptr = Alloc<ContinueStmt>();
	return ptr;
}
template<>
ReturnStmt* AstSerializer::DeserStmtDerive(IJsonDict const* dict) {
	auto ptr = Alloc<ReturnStmt>(DeserExpr(dict));
	return ptr;
}
template<>
ScopeStmt* AstSerializer::DeserStmtDerive(IJsonDict const* dict) {
	auto ptr = Alloc<ScopeStmt>();
	auto arr = dict->Get("scope"sv).get_or<IJsonArray*>(nullptr);
	assert(arr);
	DeserStmtScope(*ptr, arr);
}
template<>
IfStmt* AstSerializer::DeserStmtDerive(IJsonDict const* dict) {
	auto condExpr = dict->Get("condition"sv).get_or<IJsonDict*>(nullptr);
	auto trueStmt = dict->Get("true"sv).get_or<IJsonArray*>(nullptr);
	auto falseStmt = dict->Get("false"sv).get_or<IJsonArray*>(nullptr);
	assert(condExpr && trueStmt && falseStmt);
	auto ptr = Alloc<IfStmt>(DeserExpr(condExpr));
	DeserStmtScope(ptr->_true_branch, trueStmt);
	DeserStmtScope(ptr->_false_branch, falseStmt);
	return ptr;
}
template<>
LoopStmt* AstSerializer::DeserStmtDerive(IJsonDict const* dict) {
	auto ptr = Alloc<LoopStmt>();
	auto arr = dict->Get("body"sv).get_or<IJsonArray*>(nullptr);
	assert(arr);
	DeserStmtScope(ptr->_body, arr);
	return ptr;
}
template<>
ExprStmt* AstSerializer::DeserStmtDerive(IJsonDict const* dict) {
	auto ptr = Alloc<ExprStmt>(DeserExpr(dict));
	return ptr;
}
template<>
SwitchStmt* AstSerializer::DeserStmtDerive(IJsonDict const* dict) {
	auto expr = dict->Get("expr"sv).get_or<IJsonDict*>(nullptr);
	auto body = dict->Get("body"sv).get_or<IJsonArray*>(nullptr);
	assert(expr && body);
	auto ptr = Alloc<SwitchStmt>(DeserExpr(expr));
	DeserStmtScope(ptr->_body, body);
	return ptr;
}
template<>
SwitchCaseStmt* AstSerializer::DeserStmtDerive(IJsonDict const* dict) {
	auto ptr = Alloc<SwitchCaseStmt>(DeserExpr(dict));
	auto arr = dict->Get("body"sv).get_or<IJsonArray*>(nullptr);
	assert(arr);
	DeserStmtScope(ptr->_body, arr);
	return ptr;
}
template<>
SwitchDefaultStmt* AstSerializer::DeserStmtDerive(IJsonDict const* dict) {
	auto ptr = Alloc<SwitchDefaultStmt>();
	auto arr = dict->Get("body"sv).get_or<IJsonArray*>(nullptr);
	assert(arr);
	DeserStmtScope(ptr->_body, arr);
	return ptr;
}
template<>
AssignStmt* AstSerializer::DeserStmtDerive(IJsonDict const* dict) {
	auto lhs = dict->Get("lhs"sv).get_or<IJsonDict*>(nullptr);
	auto rhs = dict->Get("rhs"sv).get_or<IJsonDict*>(nullptr);
	auto ptr = Alloc<AssignStmt>(DeserExpr(lhs), DeserExpr(rhs));
	return ptr;
}
template<>
ForStmt* AstSerializer::DeserStmtDerive(IJsonDict const* dict) {
	auto var = dict->Get("var"sv).get_or<IJsonDict*>(nullptr);
	auto cond = dict->Get("cond"sv).get_or<IJsonDict*>(nullptr);
	auto step = dict->Get("step"sv).get_or<IJsonDict*>(nullptr);
	auto ptr = Alloc<ForStmt>(
		DeserExpr(var),
		DeserExpr(cond),
		DeserExpr(step));
	return ptr;
}
template<>
UnaryExpr* AstSerializer::DeserExprDerive(Type const* type, IJsonDict const* dict) {
	auto operandDict = dict->Get("operand"sv).get_or<IJsonDict*>(nullptr);
	auto op = static_cast<UnaryOp>(dict->Get("op"sv).get_or<int64>(0));
	assert(operandDict);
	return Alloc<UnaryExpr>(type, op, DeserExpr(operandDict));
}
template<>
BinaryExpr* AstSerializer::DeserExprDerive(Type const* type, IJsonDict const* dict) {
	auto lhs = dict->Get("lhs"sv).get_or<IJsonDict*>(nullptr);
	auto rhs = dict->Get("rhs"sv).get_or<IJsonDict*>(nullptr);
	assert(lhs && rhs);
	auto op = static_cast<BinaryOp>(dict->Get("op"sv).get_or<int64>(0));
	return Alloc<BinaryExpr>(
		type,
		op,
		DeserExpr(lhs),
		DeserExpr(rhs));
}
template<>
AccessExpr* AstSerializer::DeserExprDerive(Type const* type, IJsonDict const* dict) {
	auto range = dict->Get("range"sv).get_or<IJsonDict*>(nullptr);
	auto index = dict->Get("index"sv).get_or<IJsonDict*>(nullptr);
	assert(range && index);
	return Alloc<AccessExpr>(
		type,
		DeserExpr(range),
		DeserExpr(index));
}
template<>
MemberExpr* AstSerializer::DeserExprDerive(Type const* type, IJsonDict const* dict) {
	auto self = dict->Get("self"sv).get_or<IJsonDict*>(nullptr);
	auto swizzleSizeIndex = dict->Get("swizzlesize"sv).try_get<int64>();
	//swizzle
	if (swizzleSizeIndex) {
		return Alloc<MemberExpr>(
			type,
			DeserExpr(self),
			*swizzleSizeIndex,
			dict->Get("swizzlecode"sv).get_or<int64>(0));
	}
	//member
	else {
		return Alloc<MemberExpr>(
			type,
			DeserExpr(self),
			dict->Get("member"sv).get_or<int64>(0));
	}
}
template<>
RefExpr* AstSerializer::DeserExprDerive(Type const* type, IJsonDict const* dict) {
	auto uid = dict->Get("var"sv).try_get<int64>();
	auto tag = static_cast<Variable::Tag>(dict->Get("vartag"sv).get_or<int64>(0));
	assert(uid);
	IJsonDict const* varDict;
	switch (tag) {
		case Variable::Tag::SHARED:
			varDict = sharedVarDict;
			break;
		case Variable::Tag::THREAD_ID:
		case Variable::Tag::BLOCK_ID:
		case Variable::Tag::DISPATCH_ID:
		case Variable::Tag::DISPATCH_SIZE:
			varDict = builtinVarDict;
		default:
			varDict = localVarDict;
			break;
	}
	auto varValueDict = varDict->Get(uid).get_or<IJsonDict*>(nullptr);
	assert(varValueDict);
	Variable var;
	DeSerialize(var, varValueDict);
	return Alloc<RefExpr>(
		var);
}
template<>
LiteralExpr* AstSerializer::DeserExprDerive(Type const* type, IJsonDict const* dict) {
	LiteralExpr::Value value;
	DeSerialize(value, dict);
	return Alloc<LiteralExpr>(type, std::move(value));
}
template<>
ConstantExpr* AstSerializer::DeserExprDerive(Type const* type, IJsonDict const* dict) {
	auto hash = dict->Get("value"sv).get_or<int64>(0);
	auto constDict = this->constDict->Get(hash).get_or<IJsonDict*>(nullptr);
	assert(constDict);
	return Alloc<ConstantExpr>(
		type,
		DeSerialize(constDict));
}
template<>
CallExpr* AstSerializer::DeserExprDerive(Type const* type, IJsonDict const* dict) {
	auto op = static_cast<CallOp>(dict->Get("op").get_or<int64>(0));
	auto argListArr = dict->Get("arglist"sv).get_or<IJsonArray*>(nullptr);
	assert(argListArr);
	CallExpr::ArgumentList argList;
	argList.reserve(argListArr->Length());
	for (auto i : *argListArr) {
		auto subDict = i.get_or<IJsonDict*>(nullptr);
		assert(subDict);
		argList.emplace_back(DeserExpr(subDict));
	}
	if (op == CallOp::CUSTOM) {
		auto custom = dict->Get("custom").get_or<int64>(0);
		assert(custom);
		auto&& func =
			callables
				.Emplace(
					custom,
					vstd::LazyEval([&] {
						auto customDict = funcDict->Get(custom).get_or<IJsonDict*>(nullptr);
						assert(customDict);
						auto builder = new detail::FunctionBuilder();
						DeserFunction(builder, customDict);
						return builder;
					}))
				.Value();
		return Alloc<CallExpr>(
			type,
			func,
			std::move(argList));
	} else {
		return Alloc<CallExpr>(
			type,
			op,
			std::move(argList));
	}
}
template<>
CastExpr* AstSerializer::DeserExprDerive(Type const* type, IJsonDict const* dict) {
	auto exprDict = dict->Get("src"sv).get_or<IJsonDict*>(nullptr);
	assert(exprDict);
	auto expr = DeserExpr(exprDict);
	auto op = static_cast<CastOp>(dict->Get("op"sv).get_or<int64>(0));
	return Alloc<CastExpr>(
		type,
		op,
		expr);
}

void AstSerializer::DeserFunction(detail::FunctionBuilder* builder, IJsonDict const* dict) {
	auto GetVar = [&](vstd::string_view name, luisa::vector<Variable>& dst, IJsonDict*& sub) {
		sub = dict->Get(name).get_or<IJsonDict*>(nullptr);
		assert(sub);
		dst.reserve(sub->Length());
		for (auto&& i : *sub) {
			auto varDict = i.value.get_or<IJsonDict*>(nullptr);
			assert(varDict);
			Variable& v = dst.emplace_back();
			DeSerialize(v, varDict);
		}
	};
	auto GetMapVar = [&](vstd::string_view name, luisa::unordered_map<uint64, Variable>& dst, IJsonDict*& sub) {
		sub = dict->Get(name).get_or<IJsonDict*>(nullptr);
		assert(sub);
		dst.reserve(sub->Length());
		for (auto&& i : *sub) {
			auto varDict = i.value.get_or<IJsonDict*>(nullptr);
			assert(varDict);
			Variable& v = dst.try_emplace(static_cast<uint64>(i.key.get_or<int64>(0))).first->second;
			DeSerialize(v, varDict);
		}
	};
	GetVar("buildinvar"sv, builder->_builtin_variables, builtinVarDict);
	GetVar("localvar"sv, builder->_local_variables, localVarDict);
	GetVar("args", builder->_arguments, arguments);
	GetMapVar("sharedvar"sv, builder->_shared_variables, sharedVarDict);
	{
		constDict = dict->Get("consts").get_or<IJsonDict*>(nullptr);
		assert(constDict);
		builder->_captured_constants.reserve(constDict->Length());
		for (auto&& i : *constDict) {
			auto v = i.value.get_or<IJsonDict*>(nullptr);
			assert(v);
			//TODO
			/*
			builder->_captured_constants.emplace(
				i.key.get_or<int64>(0),
				Function::Constant(DeSerialize(v)));*/
		}
	}
	//TODO
}

void AstSerializer::DeserializeKernel(detail::FunctionBuilder* builder, IJsonDict const* dict) {
	callables.Clear();
	vstd::VEngineMallocVisitor mallocVisitor;
	stackAlloc.New(1024, &mallocVisitor);
	this->builder = builder;
	funcDict = dict->Get("funcs"sv).get_or<IJsonDict*>(nullptr);
	typeDict = dict->Get("types"sv).get_or<IJsonDict*>(nullptr);
	assert(funcDict && typeDict && constDict);
	DeserFunction(builder, dict);

	auto disposeStackAlloc = vstd::create_disposer([&] { stackAlloc.Delete(); });
}
Statement* AstSerializer::DeserStmt(IJsonDict const* dict) {
	auto hashPtr = dict->Get("stmthash"sv).try_get<int64>();
	assert(hashPtr);
	auto tagPtr = dict->Get("stmttag"sv).try_get<int64>();
	assert(tagPtr);
	auto tag = static_cast<Statement::Tag>(*tagPtr);
	Statement* stmt;
	switch (tag) {
		case Statement::Tag::BREAK:
			stmt = DeserStmtDerive<BreakStmt>(dict);
			break;
		case Statement::Tag::CONTINUE:
			stmt = DeserStmtDerive<ContinueStmt>(dict);
			break;
		case Statement::Tag::RETURN:
			stmt = DeserStmtDerive<ReturnStmt>(dict);
			break;
		case Statement::Tag::SCOPE:
			stmt = DeserStmtDerive<ScopeStmt>(dict);
			break;
		case Statement::Tag::IF:
			stmt = DeserStmtDerive<IfStmt>(dict);
			break;
		case Statement::Tag::LOOP:
			stmt = DeserStmtDerive<LoopStmt>(dict);
			break;
		case Statement::Tag::EXPR:
			stmt = DeserStmtDerive<ExprStmt>(dict);
			break;
		case Statement::Tag::SWITCH:
			stmt = DeserStmtDerive<SwitchStmt>(dict);
			break;
		case Statement::Tag::SWITCH_CASE:
			stmt = DeserStmtDerive<SwitchCaseStmt>(dict);
			break;
		case Statement::Tag::SWITCH_DEFAULT:
			stmt = DeserStmtDerive<SwitchDefaultStmt>(dict);
			break;
		case Statement::Tag::ASSIGN:
			stmt = DeserStmtDerive<AssignStmt>(dict);
			break;
		case Statement::Tag::FOR:
			stmt = DeserStmtDerive<ForStmt>(dict);
			break;
		default:
			stmt = nullptr;
			break;
	}
	stmt->_hash = *hashPtr;
	return stmt;
}
Expression* AstSerializer::DeserExpr(IJsonDict const* dict) {
	auto hashPtr = dict->Get("exprhash"sv).try_get<int64>();
	assert(hashPtr);
	auto tagPtr = dict->Get("exprtag"sv).try_get<int64>();
	assert(tagPtr);
	auto typePtr = dict->Get("exprtype"sv).try_get<vstd::string_view>();
	Type const* type;
	if (typePtr) {
		type = nullptr;
	} else {
		type = Type::from(*typePtr);
	}
	auto tag = static_cast<Expression::Tag>(*tagPtr);
	Expression* expr;
	switch (tag) {
		case Expression::Tag::UNARY:
			expr = DeserExprDerive<UnaryExpr>(type, dict);
			break;
		case Expression::Tag::BINARY:
			expr = DeserExprDerive<BinaryExpr>(type, dict);
			break;
		case Expression::Tag::MEMBER:
			expr = DeserExprDerive<MemberExpr>(type, dict);
			break;
		case Expression::Tag::ACCESS:
			expr = DeserExprDerive<AccessExpr>(type, dict);
			break;
		case Expression::Tag::LITERAL:
			expr = DeserExprDerive<LiteralExpr>(type, dict);
			break;
		case Expression::Tag::REF:
			expr = DeserExprDerive<RefExpr>(type, dict);
			break;
		case Expression::Tag::CONSTANT:
			expr = DeserExprDerive<ConstantExpr>(type, dict);
			break;
		case Expression::Tag::CALL:
			expr = DeserExprDerive<CallExpr>(type, dict);
			break;
		case Expression::Tag::CAST:
			expr = DeserExprDerive<CastExpr>(type, dict);
			break;
		default:
			expr = nullptr;
			break;
	}
	//TODO: in

	expr->_hash = *hashPtr;
	return expr;
}
}// namespace luisa::compute