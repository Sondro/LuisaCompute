
#include "codegen_stack_data.h"
namespace toolhub::directx {
CodegenStackData::CodegenStackData()
    : generateStruct(
          [this](Type const *t) {
              return CreateStruct(t);
          }) {
    structReplaceName.Emplace(
        "float3"sv, "float4"sv);
    structReplaceName.Emplace(
        "int3"sv, "int4"sv);
    structReplaceName.Emplace(
        "uint3"sv, "uint4"sv);
}
void CodegenStackData::Clear() {
    rayDesc = nullptr;
    hitDesc = nullptr;
    tempSwitchExpr = nullptr;
    arguments.Clear();
    scopeCount = -1;
    tempSwitchCounter = 0;
    structTypes.Clear();
    constTypes.Clear();
    funcTypes.Clear();
    bindlessBufferTypes.Clear();
    customStruct.Clear();
    customStructVector.clear();
    sharedVariable.clear();
    constCount = 0;
    count = 0;
    structCount = 0;
    funcCount = 0;
    tempCount = 0;
    bindlessBufferCount = 0;
}
uint CodegenStackData::AddBindlessType(Type const *type) {
    return bindlessBufferTypes
        .Emplace(
            type,
            vstd::LazyEval([&] {
                return bindlessBufferCount++;
            }))
        .Value();
}
/*
static thread_local bool gIsCodegenSpirv = false;
bool &CodegenStackData::ThreadLocalSpirv() {
    return gIsCodegenSpirv;
}*/

StructGenerator *CodegenStackData::CreateStruct(Type const *t) {
    bool isRayType = t->description() == CodegenUtility::rayTypeDesc;
    bool isHitType = t->description() == CodegenUtility::hitTypeDesc;
    StructGenerator *newPtr;
    auto ite = customStruct.TryEmplace(
        t,
        vstd::LazyEval([&] {
            newPtr = new StructGenerator(
                t,
                structCount++,
                generateStruct);
            return vstd::create_unique(newPtr);
        }));
    if (!ite.second) {
        return ite.first.Value().get();
    }

    if (isRayType) {
        rayDesc = newPtr;
        newPtr->SetStructName(vstd::string("LCRayDesc"sv));
    } else if (isHitType) {
        hitDesc = newPtr;
        newPtr->SetStructName(vstd::string("RayPayload"sv));
    } else {
        customStructVector.emplace_back(newPtr);
    }
    return newPtr;
}
std::pair<uint64, bool> CodegenStackData::GetConstCount(uint64 data) {
    bool newValue = false;
    auto ite = constTypes.Emplace(
        data,
        vstd::LazyEval(
            [&] {
                newValue = true;
                return constCount++;
            }));
    return {ite.Value(), newValue};
}

uint64 CodegenStackData::GetFuncCount(uint64 data) {
    auto ite = funcTypes.Emplace(
        data,
        vstd::LazyEval(
            [&] {
                return funcCount++;
            }));
    return ite.Value();
}
uint64 CodegenStackData::GetTypeCount(Type const *t) {
    auto ite = structTypes.Emplace(
        t,
        vstd::LazyEval(
            [&] {
                return count++;
            }));
    return ite.Value();
}
namespace detail {

struct CodegenGlobalPool {
    std::mutex mtx;
    vstd::vector<vstd::unique_ptr<CodegenStackData>> allCodegen;
    vstd::unique_ptr<CodegenStackData> Allocate() {
        std::lock_guard lck(mtx);
        if (!allCodegen.empty()) {
            auto ite = allCodegen.erase_last();
            ite->Clear();
            return ite;
        }
        return vstd::unique_ptr<CodegenStackData>(new CodegenStackData());
    }
    void DeAllocate(vstd::unique_ptr<CodegenStackData> &&v) {
        std::lock_guard lck(mtx);
        allCodegen.emplace_back(std::move(v));
    }
};
static CodegenGlobalPool codegenGlobalPool;
}// namespace detail
CodegenStackData::~CodegenStackData() {}
vstd::unique_ptr<CodegenStackData> CodegenStackData::Allocate() {
    return detail::codegenGlobalPool.Allocate();
}
void CodegenStackData::DeAllocate(vstd::unique_ptr<CodegenStackData> &&v) {
    detail::codegenGlobalPool.DeAllocate(std::move(v));
}
}// namespace toolhub::directx