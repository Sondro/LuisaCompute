#pragma once
#include <vstl/Common.h>
#include <serde_lib/SimpleJsonLoader.h>
#include <vstl/VGuid.h>
namespace toolhub::db {

class SimpleJsonLoader;
class SimpleJsonObject : public vstd::IDisposable{
protected:
	vstd::Guid selfGuid;
	SimpleBinaryJson* db = nullptr;
	SimpleJsonObject(
		vstd::Guid const& guid,
		SimpleBinaryJson* db);
	~SimpleJsonObject() {}

public:
	vstd::HashMap<vstd::Guid, std::pair<SimpleJsonObject*, uint8_t>>::Index dbIndexer;
	SimpleBinaryJson* GetDB() const { return db; }
	vstd::Guid const& GetGUID() const { return selfGuid; }
	virtual void M_GetSerData(vstd::vector<uint8_t>& data) = 0;
	virtual void LoadFromData(vstd::span<uint8_t const> data) = 0;
};
}// namespace toolhub::db