#pragma once
#include <vstl/Common.h>
#include <serde_lib/IJsonObject.h>
namespace toolhub::db {
class Database {
public:
	virtual vstd::unique_ptr<IJsonDict> CreateDict() const= 0;
	virtual luisa::vector<vstd::unique_ptr<IJsonDict>> CreateDicts(size_t count) const= 0;
	virtual vstd::unique_ptr<IJsonArray> CreateArray() const= 0;
	virtual luisa::vector<vstd::unique_ptr<IJsonArray>> CreateArrays(size_t count) const= 0;
	virtual IJsonDict* CreateDict_RawPtr() const= 0;
	virtual luisa::vector<IJsonDict*> CreateDicts_RawPtr(size_t count) const= 0;
	virtual IJsonArray* CreateArray_RawPtr() const= 0;
	virtual luisa::vector<IJsonArray*> CreateArrays_RawPtr(size_t count) const= 0;
	////////// Extension
	virtual bool CompileFromPython(char const* code) const{
		//Not Implemented
		return false;
	}
};
#ifdef LC_SERDE_LIB_EXPORT_DLL
LC_SERDE_LIB_API Database const* Database_GetFactory();
class Database_Impl final : public Database {
public:
	vstd::unique_ptr<IJsonDict> CreateDict() const override;
	luisa::vector<vstd::unique_ptr<IJsonDict>> CreateDicts(size_t count) const override;
	vstd::unique_ptr<IJsonArray> CreateArray() const override;
	luisa::vector<vstd::unique_ptr<IJsonArray>> CreateArrays(size_t count) const override;
	IJsonDict* CreateDict_RawPtr() const override;
	IJsonArray* CreateArray_RawPtr() const override;
	luisa::vector<IJsonDict*> CreateDicts_RawPtr(size_t count) const override;
	luisa::vector<IJsonArray*> CreateArrays_RawPtr(size_t count) const override;
};
#endif
}// namespace toolhub::db