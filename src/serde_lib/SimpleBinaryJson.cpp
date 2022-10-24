#include <serde_lib/DatabaseInclude.h>
#include <serde_lib/SimpleJsonObject.h>
#include <serde_lib/SimpleJsonValue.h>
#include <core/dynamic_module.h>
namespace toolhub::db {
#ifdef VENGINE_PYTHON_SUPPORT
static std::mutex pyMtx;
static SimpleBinaryJson* cur_Obj = nullptr;

LC_SERDE_LIB_API SimpleBinaryJson* db_get_curobj() {
	return cur_Obj;
}
bool SimpleBinaryJson::CompileFromPython(char const* code) {
	std::lock_guard lck(pyMtx);
	auto pyLibData = py::PythonLibImpl::Current();
	cur_Obj = this;
	pyLibData->Initialize();
	auto disp = vstd::scope_exit([&]() {
		cur_Obj = nullptr;
		pyLibData->Finalize();
	});
	return pyLibData->ExecutePythonString(code);
}
#endif
////////////////// Single Thread DB

vstd::unique_ptr<IJsonDict> Database_Impl::CreateDict() const {
	return vstd::create_unique<IJsonDict>(new SimpleJsonValueDict());
	//return vstd::create_unique<IJsonDict>(dictValuePool.New(this));
}
vstd::unique_ptr<IJsonArray> Database_Impl::CreateArray() const {
	return vstd::create_unique<IJsonArray>(new SimpleJsonValueArray());
}
IJsonDict* Database_Impl::CreateDict_RawPtr() const {
	return new SimpleJsonValueDict();
}
IJsonArray* Database_Impl::CreateArray_RawPtr() const {
	return new SimpleJsonValueArray();
}
luisa::vector<vstd::unique_ptr<IJsonDict>> Database_Impl::CreateDicts(size_t count) const {
	luisa::vector<vstd::unique_ptr<IJsonDict>> vec;
	vec.reserve(count);
	for (auto i : vstd::range(count)) {
		vec.emplace_back(new SimpleJsonValueDict());
	}
	return vec;
}
luisa::vector<vstd::unique_ptr<IJsonArray>> Database_Impl::CreateArrays(size_t count) const {
	luisa::vector<vstd::unique_ptr<IJsonArray>> vec;
	vec.reserve(count);
	for (auto i : vstd::range(count)) {
		vec.emplace_back(new SimpleJsonValueArray());
	}
	return vec;
}
luisa::vector<IJsonDict*> Database_Impl::CreateDicts_RawPtr(size_t count) const {
	luisa::vector<IJsonDict*> vec;
	vec.reserve(count);
	for (auto i : vstd::range(count)) {
		vec.emplace_back(new SimpleJsonValueDict());
	}
	return vec;
}
luisa::vector<IJsonArray*> Database_Impl::CreateArrays_RawPtr(size_t count) const {
	luisa::vector<IJsonArray*> vec;
	vec.reserve(count);
	for (auto i : vstd::range(count)) {
		vec.emplace_back(new SimpleJsonValueArray());
	}
	return vec;
}

static vstd::optional<Database_Impl> database_Impl;
LC_SERDE_LIB_API toolhub::db::Database const* Database_GetFactory() {
	database_Impl.New();
	return database_Impl;
}

}// namespace toolhub::db