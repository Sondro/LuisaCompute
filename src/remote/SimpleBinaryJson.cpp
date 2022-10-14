
#include <remote/SimpleBinaryJson.h>
#include <remote/DatabaseInclude.h>
namespace toolhub::db {
#ifdef VENGINE_PYTHON_SUPPORT
static std::mutex pyMtx;
static SimpleBinaryJson* cur_Obj = nullptr;

VENGINE_UNITY_EXTERN SimpleBinaryJson* db_get_curobj() {
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
SimpleBinaryJson::SimpleBinaryJson()
	: arrValuePool(32, false), dictValuePool(32, false) {
	root.New(this);
}

IJsonDict* SimpleBinaryJson::GetRootNode() {
	return root;
}
SimpleBinaryJson ::~SimpleBinaryJson() {
	root.Delete();
}
vstd::unique_ptr<IJsonDict> SimpleBinaryJson::CreateDict() {
	return vstd::create_unique<IJsonDict>(dictValuePool.New(this));
}
vstd::unique_ptr<IJsonArray> SimpleBinaryJson::CreateArray() {
	return vstd::create_unique<IJsonArray>(arrValuePool.New(this));
}
SimpleJsonValueDict* SimpleBinaryJson::CreateDict_Nake() {
	auto ptr = dictValuePool.New(this);
	return ptr;
}
SimpleJsonValueArray* SimpleBinaryJson::CreateArray_Nake() {
	auto ptr = arrValuePool.New(this);
	return ptr;
}

vstd::vector<vstd::unique_ptr<IJsonDict>> SimpleBinaryJson::CreateDicts(size_t count) {
	vstd::vector<vstd::unique_ptr<IJsonDict>> vec;
	vec.reserve(count);
	for (auto i : vstd::range(count)) {
		vec.emplace_back(CreateDict_Nake());
	}
	return vec;
}
vstd::vector<vstd::unique_ptr<IJsonArray>> SimpleBinaryJson::CreateArrays(size_t count) {
	vstd::vector<vstd::unique_ptr<IJsonArray>> vec;
	vec.reserve(count);
	for (auto i : vstd::range(count)) {
		vec.emplace_back(CreateArray_Nake());
	}
	return vec;
}
vstd::vector<IJsonDict*> SimpleBinaryJson::CreateDicts_RawPtr(size_t count) {
	vstd::vector<IJsonDict*> vec;
	vec.reserve(count);
	for (auto i : vstd::range(count)) {
		vec.emplace_back(CreateDict_Nake());
	}
	return vec;
}
vstd::vector<IJsonArray*> SimpleBinaryJson::CreateArrays_RawPtr(size_t count) {
	vstd::vector<IJsonArray*> vec;
	vec.reserve(count);
	for (auto i : vstd::range(count)) {
		vec.emplace_back(CreateArray_Nake());
	}
	return vec;
}

IJsonDatabase* CreateDatabase() {
	return new SimpleBinaryJson();
}
}// namespace toolhub::db