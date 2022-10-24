#include "ast_serde.h"
#include <vstl/variant_util.h>
#include <ast/function_builder.h>

namespace luisa::compute {
using namespace toolhub::db;
template<typename T>
struct BasicType;
template<>
struct BasicType<float> {
    using Type = double;
};
template<>
struct BasicType<int> {
    using Type = int64;
};
template<>
struct BasicType<uint> {
    using Type = int64;
};
template<>
struct BasicType<bool> {
    using Type = bool;
};
using ReadVar = vstd::VariantVisitor_t<ReadJsonVariant>;
template<typename T>
using BasicType_t = typename BasicType<T>::Type;
void AstSerializer::Serialize(Variable const &t, IJsonDict *r) {
    if (t.type())
        r->Set("type", (int64)t.type()->_hash);
    r->Set("uid", (int64)t.uid());
    r->Set("tag", static_cast<int64>(t.tag()));
}

template<typename T>
struct SerArrayVisitor {
    void SerArray(
        IJsonArray &r,
        luisa::span<T const> a) const {
        for (auto &&i : a) {
            r << static_cast<BasicType_t<T>>(i);
        }
    }
    void SerValue(
        IJsonDict *dict,
        IJsonDatabase *db,
        T const &a) const {
        dict->Set("value", static_cast<BasicType_t<T>>(a));
    }
};
template<typename T, size_t n>
struct SerArrayVisitor<luisa::Vector<T, n>> {
    using Type = luisa::Vector<T, n>;
    void SerArray(
        IJsonArray &r,
        luisa::span<Type const> a) const {
        for (auto &&i : a) {
            auto ptr = reinterpret_cast<T const *>(&i);
            for (auto id : vstd::range(n)) {
                r << static_cast<BasicType_t<T>>(ptr[id]);
            }
        }
    }
    void SerValue(
        IJsonDict *dict,
        IJsonDatabase *db,
        Type const &a) const {
        auto arr = db->CreateArray();
        arr->Reserve(n);
        auto ptr = reinterpret_cast<T const *>(&a);
        for (auto id : vstd::range(n)) {
            (*arr) << static_cast<BasicType_t<T>>(ptr[id]);
        }
        dict->Set("value", std::move(arr));
    }
};
template<size_t n>
struct SerArrayVisitor<luisa::Matrix<n>> {
    using Type = luisa::Matrix<n>;
    void SerArray(
        IJsonArray &r,
        luisa::span<Type const> a) const {
        for (auto &&i : a) {
            for (auto &&col : i.cols) {
                float const *ptr = reinterpret_cast<float const *>(&col);
                for (auto &&row : vstd::range(n)) {
                    r << static_cast<double>(ptr[row]);
                }
            }
        }
    }
    void SerValue(
        IJsonDict *dict,
        IJsonDatabase *db,
        Type const &a) const {
        auto arr = db->CreateArray();
        arr->Reserve(n * n);
        for (auto &&col : a.cols) {
            float const *ptr = reinterpret_cast<float const *>(&col);
            for (auto &&row : vstd::range(n)) {
                *arr << static_cast<double>(ptr[row]);
            }
        }
        dict->Set("value", std::move(arr));
    }
};
void AstSerializer::Serialize(ConstantData const &t, IJsonDict *r) {
    auto &&view = t.view();
    r->Set("viewtype", (int64)view.index());
    auto arr = db->CreateArray();
    luisa::visit(
        [&]<typename T>(T const &t) {
            SerArrayVisitor<std::remove_cvref_t<typename T::element_type>>().SerArray(*arr, t);
        },
        view);
    r->Set("values", std::move(arr));
}
void AstSerializer::DeSerialize(Variable &t, IJsonDict const *r) {
    auto type = ReadVar::try_get<int64>(r->Get("type"));
    if (type)
        t._type = Type::find(*type);
    else
        t._type = nullptr;
    t._tag = static_cast<Variable::Tag>(ReadVar::get_or<int64>(r->Get("tag"), 0));
    t._uid = ReadVar::get_or<int64>(r->Get("uid"), 0);
}
template<typename T>
struct DeserArray {
    template<typename Func>
    void operator()(
        IJsonArray const &arr,
        vstd::StackAllocator &evt,
        Func &&setView) const {
        size_t sz = arr.Length() * sizeof(T);
        auto chunk = evt.Allocate(sz);
        T *ptr = reinterpret_cast<T *>(chunk.handle + chunk.offset);

        setView(luisa::span<T const>(ptr, arr.Length()));
        for (auto &&i : arr) {
            *ptr = static_cast<T>(ReadVar::get_or<BasicType_t<T>>(i, 0));
            ptr++;
        }
    }
};
template<typename T, size_t n>
struct DeserArray<luisa::Vector<T, n>> {
    template<typename Func>
    void operator()(
        IJsonArray const &arr,
        vstd::StackAllocator &evt,
        Func &&setView) const {
        size_t sz = arr.Length() * sizeof(T);
        auto chunk = evt.Allocate(sz);
        T *ptr = reinterpret_cast<T *>(chunk.handle + chunk.offset);
        setView(luisa::span<luisa::Vector<T, n> const>((luisa::Vector<T, n> *)ptr, arr.Length() / n));
        for (auto &&i : arr) {
            *ptr = static_cast<T>(ReadVar::get_or<BasicType_t<T>>(i, 0));
            ptr++;
        }
    }
};
template<size_t n>
struct DeserArray<luisa::Matrix<n>> {
    template<typename Func>
    void operator()(
        IJsonArray const &arr,
        vstd::StackAllocator &evt,
        Func &&setView) const {
        size_t sz = arr.Length() * sizeof(float);
        auto chunk = evt.Allocate(sz);
        float *ptr = reinterpret_cast<float *>(chunk.handle + chunk.offset);
        setView(luisa::span<luisa::Matrix<n> const>((luisa::Matrix<n> *)ptr, arr.Length() / (n * n)));
        for (auto &&i : arr) {
            *ptr = ReadVar::get_or<double>(i, 0);
            ptr++;
        }
    }
};
ConstantData AstSerializer::DeSerialize(IJsonDict const *r) {
    auto arrOpt = ReadVar::try_get<IJsonArray *>(r->Get("values"));
    auto type = ReadVar::get_or(r->Get("viewtype"), std::numeric_limits<int64>::max());
    if (arrOpt) {
        stackAlloc->Clear();
        ConstantData::View view;
        auto &&arr = **arrOpt;
        vstd::VariantVisitor_t<basic_types>()(
            [&]<typename T>() {
                auto setFunc = [&](auto &&v) {
                    view = v;
                };
                DeserArray<T>().template operator()<decltype(setFunc)>(
                    arr,
                    *stackAlloc,
                    std::move(setFunc));
            },
            type);
        return ConstantData::create(view);
    }
    return ConstantData{};
}
void AstSerializer::Serialize(LiteralExpr::Value const &t, IJsonDict *r) {
    luisa::visit(
        [&]<typename T>(T const &t) {
            SerArrayVisitor<T>().SerValue(r, db, t);
        },
        t);
    r->Set("valuetype", (int64)t.index());
}
template<typename T>
struct DeserLiteral {
    void operator()(
        IJsonDict const *r,
        LiteralExpr::Value &t) const {
        t = static_cast<T>(ReadVar::get_or<BasicType_t<T>>(r->Get("value"), 0));
    }
};
template<typename T, size_t n>
struct DeserLiteral<luisa::Vector<T, n>> {
    void operator()(
        IJsonDict const *r,
        LiteralExpr::Value &t) const {
        auto arr = ReadVar::get_or<IJsonArray *>(r->Get("value"), nullptr);
        if (!arr || arr->Length() < n) return;
        luisa::Vector<T, n> vec;
        T *vecPtr = reinterpret_cast<T *>(&vec);
        for (auto i : vstd::range(n)) {
            vecPtr[i] = ReadVar::get_or<BasicType_t<T>>(arr->Get(i), 0);
        }
        t = vec;
    }
};

template<size_t n>
struct DeserLiteral<luisa::Matrix<n>> {
    void operator()(
        IJsonDict const *r,
        LiteralExpr::Value &t) const {
        auto arr = ReadVar::get_or<IJsonArray *>(r->Get("value"), nullptr);
        if (!arr || arr->Length() < (n * n)) return;
        luisa::Matrix<n> vec;
        float *vecPtr = reinterpret_cast<float *>(&vec);
        for (auto i : vstd::range(n * n)) {
            vecPtr[i] = ReadVar::get_or<double>(arr->Get(i), 0);
        }
        t = vec;
    }
};
void AstSerializer::DeSerialize(LiteralExpr::Value &t, IJsonDict const *r) {
    auto type = ReadVar::try_get<int64>(r->Get("valuetype"));
    if (!type)
        return;
    vstd::VariantVisitor_t<basic_types>()(
        [&]<typename T>() {
            DeserLiteral<T>()(
                r,
                t);
        },
        *type);
}

int64 AstSerializer::SerType(Type const *type) {
    auto hs = type->hash();
    typeDict->TrySet(
        hs,
        [&] { return vstd::string(type->description()); });
    return hs;
}

template<>
void AstSerializer::SerExprDerive(UnaryExpr const *expr, IJsonDict *dict) {
    dict->Set("operand"sv, SerExpr(expr->operand()));
    dict->Set("op"sv, static_cast<int64>(expr->op()));
}
template<>
void AstSerializer::SerExprDerive(BinaryExpr const *expr, IJsonDict *dict) {
    dict->Set("lhs"sv, SerExpr(expr->lhs()));
    dict->Set("rhs"sv, SerExpr(expr->rhs()));
    dict->Set("op"sv, static_cast<int64>(expr->op()));
}
template<>
void AstSerializer::SerExprDerive(AccessExpr const *expr, IJsonDict *dict) {
    dict->Set("index"sv, SerExpr(expr->index()));
    dict->Set("range"sv, SerExpr(expr->range()));
}
template<>
void AstSerializer::SerExprDerive(MemberExpr const *expr, IJsonDict *dict) {
    dict->Set("self"sv, SerExpr(expr->self()));
    if (expr->is_swizzle()) {
        dict->Set("swizzlesize"sv, expr->swizzle_size());
        dict->Set("swizzlecode"sv, expr->swizzle_code());
    } else {
        dict->Set("member"sv, expr->member_index());
    }
}
template<>
void AstSerializer::SerExprDerive(RefExpr const *expr, IJsonDict *dict) {
    dict->Set("vartag"sv, static_cast<int64>(expr->variable().tag()));
    dict->Set("var"sv, expr->variable().uid());
}
template<>
void AstSerializer::SerExprDerive(LiteralExpr const *expr, IJsonDict *dict) {
    Serialize(expr->value(), dict);
}
template<>
void AstSerializer::SerExprDerive(ConstantExpr const *expr, IJsonDict *dict) {
    dict->Set("value"sv, expr->hash());
}
template<>
void AstSerializer::SerExprDerive(CallExpr const *expr, IJsonDict *dict) {
    dict->Set("op"sv, static_cast<int64>(expr->op()));
    auto argList = db->CreateArray();
    auto args = expr->arguments();
    argList->Reserve(args.size());
    for (auto &&i : args) {
        argList->Add(SerExpr(i));
    }
    dict->Set("arglist"sv, std::move(argList));
    if (expr->op() == CallOp::CUSTOM) {
        dict->Set("custom"sv, SerFunction(expr->custom()));
    }
}
template<>
void AstSerializer::SerExprDerive(CastExpr const *expr, IJsonDict *dict) {
    dict->Set("src"sv, SerExpr(expr->expression()));
    dict->Set("op"sv, static_cast<int64>(expr->op()));
}
template<>
void AstSerializer::SerStmtDerive(BreakStmt const *stmt, IJsonDict *dict) {}
template<>
void AstSerializer::SerStmtDerive(ContinueStmt const *stmt, IJsonDict *dict) {}
template<>
void AstSerializer::SerStmtDerive(ReturnStmt const *stmt, IJsonDict *dict) {
    SerExpr(stmt->expression(), dict);
}
void AstSerializer::SerScope(ScopeStmt const *stmt, IJsonDict *dict, vstd::string_view key) {
    auto arr = db->CreateArray();
    auto stmts = stmt->statements();
    arr->Reserve(stmts.size());
    for (auto &&i : stmts) {
        arr->Add(SerStmt(i));
    }
    dict->Set(key, std::move(arr));
}

template<>
void AstSerializer::SerStmtDerive(ScopeStmt const *stmt, IJsonDict *dict) {
    SerScope(stmt, dict, "scope"sv);
}
template<>
void AstSerializer::SerStmtDerive(AssignStmt const *stmt, IJsonDict *dict) {
    dict->Set("lhs"sv, SerExpr(stmt->lhs()));
    dict->Set("rhs"sv, SerExpr(stmt->rhs()));
}
template<>
void AstSerializer::SerStmtDerive(IfStmt const *stmt, IJsonDict *dict) {
    dict->Set("condition"sv, SerExpr(stmt->condition()));
    SerScope(stmt->true_branch(), dict, "true"sv);
    SerScope(stmt->false_branch(), dict, "false"sv);
}
template<>
void AstSerializer::SerStmtDerive(LoopStmt const *stmt, IJsonDict *dict) {
    SerScope(stmt->body(), dict, "body"sv);
}
template<>
void AstSerializer::SerStmtDerive(SwitchStmt const *stmt, IJsonDict *dict) {
    dict->Set("expr"sv, SerExpr(stmt->expression()));
    SerScope(stmt->body(), dict, "body"sv);
}
template<>
void AstSerializer::SerStmtDerive(SwitchCaseStmt const *stmt, IJsonDict *dict) {
    SerExpr(stmt->expression(), dict);
    SerScope(stmt->body(), dict, "body"sv);
}
template<>
void AstSerializer::SerStmtDerive(SwitchDefaultStmt const *stmt, IJsonDict *dict) {
    SerScope(stmt->body(), dict, "body"sv);
}
template<>
void AstSerializer::SerStmtDerive(ExprStmt const *stmt, IJsonDict *dict) {
    SerExpr(stmt->expression(), dict);
}
template<>
void AstSerializer::SerStmtDerive(ForStmt const *stmt, IJsonDict *dict) {
    dict->Set("var"sv, SerExpr(stmt->variable()));
    dict->Set("cond"sv, SerExpr(stmt->condition()));
    dict->Set("step"sv, SerExpr(stmt->step()));
    SerScope(stmt->body(), dict, "body"sv);
}

void AstSerializer::SerStmt(Statement const *stmt, IJsonDict *dict) {
    dict->Set("stmthash"sv, stmt->hash());
    dict->Set("stmttag"sv, static_cast<int64>(stmt->tag()));
    switch (stmt->tag()) {
        case Statement::Tag::BREAK:
            SerStmtDerive(static_cast<BreakStmt const *>(stmt), dict);
            break;
        case Statement::Tag::CONTINUE:
            SerStmtDerive(static_cast<ContinueStmt const *>(stmt), dict);
            break;
        case Statement::Tag::RETURN:
            SerStmtDerive(static_cast<ReturnStmt const *>(stmt), dict);
            break;
        case Statement::Tag::SCOPE:
            SerStmtDerive(static_cast<ScopeStmt const *>(stmt), dict);
            break;
        case Statement::Tag::IF:
            SerStmtDerive(static_cast<IfStmt const *>(stmt), dict);
            break;
        case Statement::Tag::LOOP:
            SerStmtDerive(static_cast<LoopStmt const *>(stmt), dict);
            break;
        case Statement::Tag::EXPR:
            SerStmtDerive(static_cast<ExprStmt const *>(stmt), dict);
            break;
        case Statement::Tag::SWITCH:
            SerStmtDerive(static_cast<SwitchStmt const *>(stmt), dict);
            break;
        case Statement::Tag::SWITCH_CASE:
            SerStmtDerive(static_cast<SwitchCaseStmt const *>(stmt), dict);
            break;
        case Statement::Tag::SWITCH_DEFAULT:
            SerStmtDerive(static_cast<SwitchDefaultStmt const *>(stmt), dict);
            break;
        case Statement::Tag::ASSIGN:
            SerStmtDerive(static_cast<AssignStmt const *>(stmt), dict);
            break;
        case Statement::Tag::FOR:
            SerStmtDerive(static_cast<ForStmt const *>(stmt), dict);
            break;
    }
}
void AstSerializer::SerExpr(Expression const *expr, IJsonDict *dict) {
    dict->Set("exprhash"sv, expr->hash());
    dict->Set("exprtag"sv, static_cast<int64>(expr->tag()));
    if (expr->type())
        dict->Set("exprtype"sv, SerType(expr->type()));
    switch (expr->tag()) {
        case Expression::Tag::UNARY:
            SerExprDerive(static_cast<UnaryExpr const *>(expr), dict);
            break;
        case Expression::Tag::BINARY:
            SerExprDerive(static_cast<BinaryExpr const *>(expr), dict);
            break;
        case Expression::Tag::MEMBER:
            SerExprDerive(static_cast<MemberExpr const *>(expr), dict);
            break;
        case Expression::Tag::ACCESS:
            SerExprDerive(static_cast<AccessExpr const *>(expr), dict);
            break;
        case Expression::Tag::LITERAL:
            SerExprDerive(static_cast<LiteralExpr const *>(expr), dict);
            break;
        case Expression::Tag::REF:
            SerExprDerive(static_cast<RefExpr const *>(expr), dict);
            break;
        case Expression::Tag::CONSTANT:
            SerExprDerive(static_cast<ConstantExpr const *>(expr), dict);
            break;
        case Expression::Tag::CALL:
            SerExprDerive(static_cast<CallExpr const *>(expr), dict);
            break;
        case Expression::Tag::CAST:
            SerExprDerive(static_cast<CastExpr const *>(expr), dict);
            break;
    }
}
vstd::unique_ptr<IJsonDict> AstSerializer::SerExpr(Expression const *expr) {
    auto dict = db->CreateDict();
    SerExpr(expr, dict.get());
    return dict;
    //TYPE
}
vstd::unique_ptr<IJsonDict> AstSerializer::SerStmt(Statement const *stmt) {
    auto dict = db->CreateDict();
    SerStmt(stmt, dict.get());
    return dict;
}
vstd::unique_ptr<IJsonDict> AstSerializer::SerNewFunction(Function func) {
    auto dict = db->CreateDict();
    auto AddVars = [&](auto &&vars, vstd::string_view name, size_t reserveSize, IJsonDict *&dst) {
        auto varsDict = db->CreateDict();
        varsDict->Reserve(reserveSize);
        dst = varsDict.get();
        for (auto &&i : vars) {
            auto varDict = db->CreateDict();
            Serialize(i, varDict.get());
            varsDict->Set(i.uid(), std::move(varDict));
        }

        dict->Set(name, std::move(varsDict));
    };
    //////////////////// Builtin vars
    AddVars(func.builtin_variables(), "builtinvar"sv, func.builtin_variables().size(), builtinVarDict);
    //////////////////// Local vars
    AddVars(func.local_variables(), "localvar"sv, func.local_variables().size(), localVarDict);
    //////////////////// Arguments
    AddVars(func.arguments(), "args"sv, func.arguments().size(), arguments);
    dict->Set("usagesize"sv, func.builder()->_variable_usages.size());
    //////////////////// Shared vars
    AddVars(
        vstd::CacheEndRange(func.shared_variables()),
        "sharedvar"sv,
        func.shared_variables().size(),
        sharedVarDict);
    //////////////////// Captured
    {
        auto &&arr = func.builder()->argument_bindings();
        if (!arr.empty()) {
            //TODO: binding
            auto binds = db->CreateArray();
            binds->Reserve(arr.size());
            for (auto &&i : arr) {
                luisa::visit(
                    [&]<typename T>(T const &v) {
                        auto CreateDict = [&](int64 type) {
                            auto d = db->CreateDict();
                            d->Set("type"sv, type);
                            return d;
                        };
                        if constexpr (std::is_same_v<T, detail::FunctionBuilder::BufferBinding>) {
                            vstd::unique_ptr<IJsonDict> d = CreateDict(0);
                            d->Set("handle"sv, v.handle);
                            d->Set("offset"sv, v.offset_bytes);
                            d->Set("size"sv, v.size_bytes);
                            binds->Add(std::move(d));
                        } else if constexpr (std::is_same_v<T, detail::FunctionBuilder::TextureBinding>) {
                            vstd::unique_ptr<IJsonDict> d = CreateDict(1);
                            d->Set("handle"sv, v.handle);
                            d->Set("level"sv, v.level);
                            binds->Add(std::move(d));
                        } else if constexpr (std::is_same_v<T, detail::FunctionBuilder::BindlessArrayBinding>) {
                            vstd::unique_ptr<IJsonDict> d = CreateDict(2);
                            d->Set("handle"sv, v.handle);
                            binds->Add(std::move(d));
                        } else if constexpr (std::is_same_v<T, detail::FunctionBuilder::AccelBinding>) {
                            vstd::unique_ptr<IJsonDict> d = CreateDict(3);
                            d->Set("handle"sv, v.handle);
                            binds->Add(std::move(d));
                        } else {
                            binds->Add(WriteJsonVariant{});
                        }
                    },
                    i);
            }
            dict->Set("binds"sv, std::move(binds));
        }
    }
    //////////////////// Constant
    {
        auto constDict = db->CreateDict();
        this->constDict = constDict.get();
        auto consts = func.constants();
        constDict->Reserve(consts.size());
        for (auto &&i : consts) {
            auto localDict = db->CreateDict();
            Serialize(i.data, localDict.get());
            localDict->Set("consttype"sv, SerType(i.type));
            constDict->Set(i.hash(), std::move(localDict));
        }
        dict->Set("consts"sv, std::move(constDict));
    }
    PushStack();
    auto popStackTask = vstd::scope_exit([&] { PopStack(); });
    //////////////////// Callop
    {
        auto &&set = func.builtin_callables()._bits;
        auto arr = db->CreateArray();
        uint count = 0;
        int64 result = 0;
        for (auto i : vstd::range(set.size())) {
            bool v = set[i];
            result <<= 1;
            if (v) result |= 1;
            if ((++count) >= 64) {
                (*arr) << result;
                result = 0;
                count = 0;
            }
        }
        if (count != 0) {
            result <<= 64 - count;
            (*arr) << result;
        }
        dict->Set("callop"sv, std::move(arr));
    }
    //////////////////// Custom Functions
    {
        auto customFuncHash = db->CreateArray();
        auto &&funcs = func.custom_callables();
        customFuncHash->Reserve(funcs.size());
        for (auto &&i : funcs) {
            (*customFuncHash) << SerFunction(Function(i.get()));
        }
        dict->Set("callables"sv, std::move(customFuncHash));
    }
    //////////////////// Tag
    dict->Set("tag"sv, static_cast<int64>(func.tag()));

    //////////////////// Block size
    if (func.tag() == Function::Tag::KERNEL) {
        auto sz = func.block_size();
        auto arr = db->CreateArray();
        arr->Reserve(3);
        (*arr) << sz.x << sz.y << sz.z;
        dict->Set("blocksize"sv, std::move(arr));
    }

    //////////////////// Return Type
    if (func.return_type()) {
        dict->Set("return"sv, SerType(func.return_type()));
    }
    //////////////////// Body
    SerScope(func.body(), dict.get(), "body"sv);
    //////////////////// Use atomic
    dict->Set("atomicfloat"sv, func.requires_atomic_float());
    return dict;
}

int64 AstSerializer::SerFunction(Function func) {
    funcDict->TrySet(
        func.hash(),
        [&] { return SerNewFunction(func); });
    return func.hash();
}
void AstSerializer::SerializeKernel(Function kernel, IJsonDatabase *db, IJsonDict *root) {
    if (!root) root = db->GetRootNode();
    this->db = db;
    auto funcDict = db->CreateDict();
    auto varDict = db->CreateDict();
    auto typeDict = db->CreateDict();
    this->funcDict = funcDict.get();
    this->typeDict = typeDict.get();
    root->Set("kernel"sv, SerNewFunction(kernel));
    root->Set("funcs"sv, std::move(funcDict));
    root->Set("types"sv, std::move(typeDict));
}
#define VENGINE_SET_STACK(v) v = stack.v
void AstSerializer::PushStack() {
    stacks.emplace_back(Stack{
        localVarDict,
        builtinVarDict,
        sharedVarDict,
        arguments,
        constDict});
}
void AstSerializer::PopStack() {
    stacks.pop_back();
    if (!stacks.empty()) {
        auto &&stack = *stacks.last();
        VENGINE_SET_STACK(localVarDict);
        VENGINE_SET_STACK(builtinVarDict);
        VENGINE_SET_STACK(sharedVarDict);
        VENGINE_SET_STACK(arguments);
        VENGINE_SET_STACK(constDict);
    }
}
#undef VENGINE_SET_STACK
}// namespace luisa::compute
