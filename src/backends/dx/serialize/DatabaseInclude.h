#pragma once
#include <vstl/Common.h>
#include <serialize/IJsonObject.h>
namespace toolhub::db {
IJsonDatabase* CreateDatabase();
}// namespace toolhub::db