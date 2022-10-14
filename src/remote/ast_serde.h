#pragma once
#include <ast/type.h>
#include <remote/IJsonObject.h>
#include <ast/expression.h>
#include <ast/statement.h>
#include <ast/function.h>
#include <vstl/StackAllocator.h>
namespace luisa::compute {
using namespace toolhub::db;

class AstSerializer : public vstd::IOperatorNewBase {
	vstd::optional<vstd::StackAllocator> stackAlloc;
	IJsonDatabase* db = nullptr;
	IJsonDict* localVarDict = nullptr;
	IJsonDict* builtinVarDict = nullptr;
	IJsonDict* sharedVarDict = nullptr;
	IJsonDict* arguments = nullptr;
	IJsonDict* constDict = nullptr;
	IJsonDict* typeDict = nullptr;
	IJsonDict* funcDict = nullptr;

	struct Stack {
		IJsonDict* localVarDict;
		IJsonDict* builtinVarDict;
		IJsonDict* sharedVarDict;
		IJsonDict* arguments;
		IJsonDict* constDict;
	};
	vstd::vector<Stack> stacks;
	void PushStack();
	void PopStack();

	vstd::HashMap<uint64, luisa::shared_ptr<detail::FunctionBuilder>> callables;
	detail::FunctionBuilder* builder = nullptr;
	void Serialize(Variable const& t, IJsonDict* r);
	void DeSerialize(Variable& t, IJsonDict const* dict);
	void Serialize(ConstantData const& t, IJsonDict* r);
	ConstantData DeSerialize(IJsonDict const* dict);
	void Serialize(LiteralExpr::Value const& t, IJsonDict* r);
	void DeSerialize(LiteralExpr::Value& t, IJsonDict const* dict);
	void SerScope(ScopeStmt const* stmt, IJsonDict* dict, vstd::string_view key);

	template<typename T>
	void SerStmtDerive(T const* stmt, IJsonDict* dict);
	template<typename T>
	void SerExprDerive(T const* expr, IJsonDict* dict);
	template<typename T>
	T* DeserExprDerive(Type const* type, IJsonDict const* dict);
	template<typename T>
	T* DeserStmtDerive(IJsonDict const* dict);
	vstd::unique_ptr<IJsonDict> SerStmt(Statement const* stmt);
	vstd::unique_ptr<IJsonDict> SerExpr(Expression const* stmt);
	void SerStmt(Statement const* stmt, IJsonDict* dict);
	void SerExpr(Expression const* expr, IJsonDict* dict);
	void DeserStmtScope(ScopeStmt& value, IJsonArray const* arr);
	Statement* DeserStmt(IJsonDict const* dict);
	Expression* DeserExpr(IJsonDict const* dict);
	int64 SerType(Type const* type);
	Type const* DeserType(int64 hash);
	int64 SerFunction(Function func);
	void DeserFunction(detail::FunctionBuilder* builder, IJsonDict const* dict);
	luisa::shared_ptr<detail::FunctionBuilder> const& DeserFunction(uint64 hash);
	vstd::unique_ptr<IJsonDict> SerNewFunction(Function func);

	template<typename T, typename... Args>
		requires(std::is_constructible_v<T, Args&&...>)
	T* Alloc(Args&&...);

public:
	void SerializeKernel(Function kernel, IJsonDatabase* db, IJsonDict* root = nullptr);
	void DeserializeKernel(detail::FunctionBuilder* builder, IJsonDict const* dict);
};
}// namespace luisa::compute