#ifdef VENGINE_DB_EXPORT_C
#include <serde_lib/SimpleJsonValue.h>
namespace toolhub::db {
enum class CSharpKeyType : uint {
	Int64,
	String,
	Guid,
	None
};
enum class CSharpValueType : uint {
	Int64,
	Double,
	String,
	Dict,
	Array,
	Guid,
	Bool,
	None
};

Key GetCSharpKey(void* ptr, CSharpKeyType keyType) {

	switch (keyType) {
		case CSharpKeyType::Int64:
			return Key(*reinterpret_cast<int64*>(ptr));
		case CSharpKeyType::Guid:
			return Key(*reinterpret_cast<vstd::Guid*>(ptr));
		case CSharpKeyType::String:
			return Key(*reinterpret_cast<vstd::string_view*>(ptr));
		default:
			return {};
	}
}
void SetCSharpKey(void* ptr, CSharpKeyType keyType, Key const& key) {
	switch (keyType) {
		case CSharpKeyType::Int64: {
			*reinterpret_cast<int64*>(ptr) =
				(key.template IsTypeOf<int64>()) ? key.force_get<int64>() : 0;
		} break;
		case CSharpKeyType::Guid: {
			*reinterpret_cast<vstd::Guid*>(ptr) =
				(key.template IsTypeOf<vstd::Guid>()) ? key.force_get<vstd::Guid>() : vstd::Guid(false);
		} break;
		case CSharpKeyType::String: {
			*reinterpret_cast<vstd::string_view*>(ptr) =
				(key.template IsTypeOf<vstd::string_view>()) ? key.force_get<vstd::string_view>() : vstd::string_view(nullptr, (size_t)0);
		} break;
	}
}
CSharpKeyType SetCSharpKey(void* ptr, Key const& key) {

	CSharpKeyType keyType;
	switch (key.GetType()) {
		case Key::IndexOf<int64>:
			*reinterpret_cast<int64*>(ptr) = key.force_get<int64>();
			keyType = CSharpKeyType::Int64;
			break;
		case Key::IndexOf<vstd::string_view>:
			*reinterpret_cast<vstd::string_view*>(ptr) = key.force_get<vstd::string_view>();
			keyType = CSharpKeyType::String;
			break;
		case Key::IndexOf<vstd::Guid>:
			*reinterpret_cast<vstd::Guid*>(ptr) = key.force_get<vstd::Guid>();
			keyType = CSharpKeyType::Guid;
			break;
		default:
			keyType = CSharpKeyType::None;
	}
	return keyType;
}
WriteJsonVariant GetCSharpWriteValue(void* ptr, CSharpValueType valueType) {
	switch (valueType) {
		case CSharpValueType::Array:
			return WriteJsonVariant(vstd::unique_ptr<IJsonArray>(*reinterpret_cast<SimpleJsonValueArray**>(ptr)));
		case CSharpValueType::Dict:
			return WriteJsonVariant(vstd::unique_ptr<IJsonDict>(*reinterpret_cast<SimpleJsonValueDict**>(ptr)));
		case CSharpValueType::Double:
			return WriteJsonVariant(*reinterpret_cast<double*>(ptr));
		case CSharpValueType::Guid:
			return WriteJsonVariant(*reinterpret_cast<vstd::Guid*>(ptr));
		case CSharpValueType::Int64:
			return WriteJsonVariant(*reinterpret_cast<int64*>(ptr));
		case CSharpValueType::String:
			return WriteJsonVariant(*reinterpret_cast<vstd::string_view*>(ptr));
		case CSharpValueType::Bool:
			return {*reinterpret_cast<bool*>(ptr)};
		default:
			return {};
	}
}
void SetCSharpReadValue(void* ptr, CSharpValueType valueType, ReadJsonVariant const& readValue) {
	switch (valueType) {

		case CSharpValueType::Array:
			*reinterpret_cast<SimpleJsonValueArray**>(ptr) =
				(readValue.template IsTypeOf<IJsonArray*>()) ? (static_cast<SimpleJsonValueArray*>(readValue.force_get<IJsonArray*>())) : nullptr;
			break;

		case CSharpValueType::Dict:
			*reinterpret_cast<SimpleJsonValueDict**>(ptr) =
				(readValue.template IsTypeOf<IJsonDict*>()) ? (static_cast<SimpleJsonValueDict*>(readValue.force_get<IJsonDict*>())) : nullptr;
			break;
		case CSharpValueType::Double:
			if (readValue.template IsTypeOf<int64>()) {
				*reinterpret_cast<double*>(ptr) = readValue.force_get<int64>();
			} else if (readValue.template IsTypeOf<double>()) {
				*reinterpret_cast<double*>(ptr) = readValue.force_get<double>();
			}
			break;
		case CSharpValueType::Guid:
			*reinterpret_cast<vstd::Guid*>(ptr) =
				(readValue.template IsTypeOf<vstd::Guid>()) ? (readValue.force_get<vstd::Guid>()) : vstd::Guid(false);
			break;
		case CSharpValueType::Int64:
			if (readValue.template IsTypeOf<int64>()) {
				*reinterpret_cast<int64*>(ptr) = readValue.force_get<int64>();
			} else if (readValue.template IsTypeOf<double>()) {
				*reinterpret_cast<int64*>(ptr) = readValue.force_get<double>();
			}
			break;
		case CSharpValueType::String:
			*reinterpret_cast<vstd::string_view*>(ptr) =
				(readValue.template IsTypeOf<vstd::string_view>()) ? readValue.force_get<vstd::string_view>() : vstd::string_view(nullptr, (size_t)0);
			break;
		case CSharpValueType::Bool:
			*reinterpret_cast<bool*>(ptr) =
				(readValue.template IsTypeOf<bool>()) ? readValue.force_get<bool>() : false;
			break;
		default:
			*reinterpret_cast<bool*>(ptr) = true;
			break;
	}
}

CSharpValueType SetCSharpReadValue(void* ptr, ReadJsonVariant const& readValue) {
	CSharpValueType resultType;
	switch (readValue.GetType()) {
		case ReadJsonVariant::IndexOf<int64>:
			*reinterpret_cast<int64*>(ptr) = readValue.force_get<int64>();
			resultType = CSharpValueType::Int64;
			break;
		case ReadJsonVariant::IndexOf<double>:
			*reinterpret_cast<double*>(ptr) = readValue.force_get<double>();
			resultType = CSharpValueType::Double;
			break;
		case ReadJsonVariant::IndexOf<vstd::string_view>:
			*reinterpret_cast<vstd::string_view*>(ptr) = readValue.force_get<vstd::string_view>();
			resultType = CSharpValueType::String;
			break;
		case ReadJsonVariant::IndexOf<vstd::Guid>:
			*reinterpret_cast<vstd::Guid*>(ptr) = readValue.force_get<vstd::Guid>();
			resultType = CSharpValueType::Guid;
			break;
		case ReadJsonVariant::IndexOf<IJsonDict*>:
			*reinterpret_cast<IJsonDict**>(ptr) = readValue.force_get<IJsonDict*>();
			resultType = CSharpValueType::Dict;
			break;
		case ReadJsonVariant::IndexOf<IJsonArray*>:
			*reinterpret_cast<IJsonArray**>(ptr) = readValue.force_get<IJsonArray*>();
			resultType = CSharpValueType::Array;
			break;
		case ReadJsonVariant::IndexOf<bool>:
			*reinterpret_cast<bool*>(ptr) = readValue.force_get<bool>();
			resultType = CSharpValueType::Bool;
			break;
		default:
			resultType = CSharpValueType::None;
			break;
	}
	return resultType;
}

using DictIterator = decltype(std::declval<SimpleJsonValueDict>().vars)::Iterator;
using ArrayIterator = size_t;

}// namespace toolhub::db

//#define EXTERN_UNITY
#ifdef VENGINE_CSHARP_SUPPORT
#include <vstl/BinaryReader.h>
#include <serde_lib/Struct.h>
#include <serde_lib/DatabaseInclude.h>

namespace toolhub::db {
template<typename T>
void DBParse(T* db, Span<char> strv, vstd::funcPtr_t<void(Span<char>)> errorCallback, bool clearLast) {
	auto errorMsg = db->Parse(strv, clearLast);
	if (errorMsg) {
		errorCallback(errorMsg->message);
	} else {
		errorCallback(Span<char>(nullptr, 0));
	}
}

LC_SERDE_LIB_API void db_compile_from_py(wchar_t const* datas, int64 sz, bool* result) {
	vstd::string str;
	str.resize(sz);
	for (auto i : vstd::range(sz)) {
		str[i] = datas[i];
	}
	*result = Database_GetFactory()->CompileFromPython(str.data());
}
LC_SERDE_LIB_API void db_dict_print(SimpleJsonValueDict* db, vstd::funcPtr_t<void(Span<char>)> callback) {
	auto str = db->Print();
	callback(str);
}

LC_SERDE_LIB_API void db_arr_print(SimpleJsonValueArray* db, vstd::funcPtr_t<void(char const*, uint64)> callback) {
	auto str = db->Print();
	callback(str.data(), str.size());
}
LC_SERDE_LIB_API void db_dict_format_print(SimpleJsonValueDict* db, vstd::funcPtr_t<void(Span<char>)> callback) {
	auto str = db->FormattedPrint();
	callback(str);
}
LC_SERDE_LIB_API void db_arr_format_print(SimpleJsonValueArray* db, vstd::funcPtr_t<void(char const*, uint64)> callback) {
	auto str = db->FormattedPrint();
	callback(str.data(), str.size());
}
LC_SERDE_LIB_API void db_create_dict(SimpleJsonValueDict** pp) {
	*pp = new SimpleJsonValueDict();
}
LC_SERDE_LIB_API void db_create_array(SimpleJsonValueArray** pp) {
	*pp = new SimpleJsonValueArray();
}

LC_SERDE_LIB_API void db_arr_ser(SimpleJsonValueArray* db, vstd::funcPtr_t<void(std::byte*, uint64)> callback) {
	auto vec = db->Serialize();
	callback(vec.data(), vec.size());
}
LC_SERDE_LIB_API void db_dict_ser(SimpleJsonValueDict* db, vstd::funcPtr_t<void(std::byte*, uint64)> callback) {
	auto vec = db->Serialize();
	callback(vec.data(), vec.size());
}
LC_SERDE_LIB_API void db_arr_deser(SimpleJsonValueArray* db, std::byte* ptr, uint64 len, bool* success, bool clearLast) {
	*success = db->Read({ptr, len}, clearLast);
}
LC_SERDE_LIB_API void db_dict_deser(SimpleJsonValueDict* db, std::byte* ptr, uint64 len, bool* success, bool clearLast) {
	*success = db->Read({ptr, len}, clearLast);
}

LC_SERDE_LIB_API void db_dispose_arr(SimpleJsonValueArray* p) {
	delete p;
}

////////////////// Dict Area

LC_SERDE_LIB_API void db_dispose_dict(SimpleJsonValueDict* ptr) {
	delete ptr;
}
LC_SERDE_LIB_API void db_dict_set(
	SimpleJsonValueDict* dict,
	void* keyPtr,
	CSharpKeyType keyType,
	void* valuePtr,
	CSharpValueType valueType) {
	dict->Set(GetCSharpKey(keyPtr, keyType), GetCSharpWriteValue(valuePtr, valueType));
}
LC_SERDE_LIB_API void db_dict_tryset(
	SimpleJsonValueDict* dict,
	void* keyPtr,
	CSharpKeyType keyType,
	void* valuePtr,
	CSharpValueType valueType,
	bool* isTry) {
	Key key;
	WriteJsonVariant value;
	*isTry = false;
	dict->TrySet(GetCSharpKey(keyPtr, keyType), [&]() {
		*isTry = true;
		return GetCSharpWriteValue(valuePtr, valueType);
	});
}
LC_SERDE_LIB_API void db_dict_get(
	SimpleJsonValueDict* dict,
	void* keyPtr,
	CSharpKeyType keyType,
	CSharpValueType targetValueType,
	void* valuePtr) {
	SetCSharpReadValue(valuePtr, targetValueType, dict->Get(GetCSharpKey(keyPtr, keyType)));
}
LC_SERDE_LIB_API void db_dict_get_variant(
	SimpleJsonValueDict* dict,
	void* keyPtr,
	CSharpKeyType keyType,
	CSharpValueType* targetValueType,
	void* valuePtr) {
	auto value = dict->Get(GetCSharpKey(keyPtr, keyType));
	*targetValueType = SetCSharpReadValue(valuePtr, value);
}
LC_SERDE_LIB_API void db_dict_remove(SimpleJsonValueDict* dict, void* keyPtr, CSharpKeyType keyType) {
	dict->Remove(GetCSharpKey(keyPtr, keyType));
}
LC_SERDE_LIB_API void db_dict_len(SimpleJsonValueDict* dict, int32* sz) { *sz = dict->Length(); }
LC_SERDE_LIB_API void db_dict_itebegin(SimpleJsonValueDict* dict, DictIterator* ptr) { *ptr = dict->vars.begin(); }
LC_SERDE_LIB_API void db_dict_iteend(SimpleJsonValueDict* dict, DictIterator* end, bool* result) { *result = (*end == dict->vars.end()); }
LC_SERDE_LIB_API void db_dict_ite_next(DictIterator* end) { (*end)++; }
LC_SERDE_LIB_API void db_dict_ite_get(DictIterator ite, void* valuePtr, CSharpValueType valueType) {
	SetCSharpReadValue(valuePtr, valueType, ite->second.GetVariant());
}
LC_SERDE_LIB_API void db_dict_ite_get_variant(DictIterator ite, void* valuePtr, CSharpValueType* valueType) {
	*valueType = SetCSharpReadValue(valuePtr, ite->second.GetVariant());
}

LC_SERDE_LIB_API void db_dict_ite_getkey(DictIterator ite, void* keyPtr, CSharpKeyType keyType) {
	SetCSharpKey(keyPtr, keyType, ite->first.GetKey());
}
LC_SERDE_LIB_API void db_dict_ite_getkey_variant(DictIterator ite, void* keyPtr, CSharpKeyType* keyType) {
	*keyType = SetCSharpKey(keyPtr, ite->first.GetKey());
}
LC_SERDE_LIB_API void db_dict_reset(
	SimpleJsonValueDict* dict) {
	dict->Reset();
}
LC_SERDE_LIB_API void db_dict_parse(SimpleJsonValueDict* db, Span<char> strv, vstd::funcPtr_t<void(Span<char>)> errorCallback, bool clearLast) {
	DBParse(db, strv, errorCallback, clearLast);
}
////////////////// Array Area
LC_SERDE_LIB_API void db_arr_reset(
	SimpleJsonValueArray* arr) {
	arr->Reset();
}
LC_SERDE_LIB_API void db_arr_len(SimpleJsonValueArray* arr, int32* sz) {
	*sz = arr->Length();
}
LC_SERDE_LIB_API void db_arr_get_value(SimpleJsonValueArray* arr, int32 index, void* valuePtr, CSharpValueType valueType) {
	SetCSharpReadValue(valuePtr, valueType, arr->Get(index));
}
LC_SERDE_LIB_API void db_arr_get_value_variant(SimpleJsonValueArray* arr, int32 index, void* valuePtr, CSharpValueType* valueType) {
	*valueType = SetCSharpReadValue(valuePtr, arr->Get(index));
}
LC_SERDE_LIB_API void db_arr_set_value(SimpleJsonValueArray* arr, int32 index, void* valuePtr, CSharpValueType valueType) {
	arr->Set(index, GetCSharpWriteValue(valuePtr, valueType));
}
LC_SERDE_LIB_API void db_arr_add_value(SimpleJsonValueArray* arr, void* valuePtr, CSharpValueType valueType) {
	arr->Add(GetCSharpWriteValue(valuePtr, valueType));
}
LC_SERDE_LIB_API void db_arr_remove(SimpleJsonValueArray* arr, int32 index) {
	arr->Remove(index);
}
LC_SERDE_LIB_API void db_arr_itebegin(SimpleJsonValueArray* arr, ArrayIterator* ptr) {
	*ptr = 0;
}
LC_SERDE_LIB_API void db_arr_iteend(SimpleJsonValueArray* arr, ArrayIterator* ptr, bool* result) {
	*result = arr->arr.visit_or(
		true,
		[&](auto&& arr) -> bool {
			return (*ptr == arr.size());
		});
}
LC_SERDE_LIB_API void db_arr_ite_next(SimpleJsonValueArray* arr, ArrayIterator* ptr) {
	(*ptr)++;
}
LC_SERDE_LIB_API void db_arr_ite_get(SimpleJsonValueArray* arr, ArrayIterator ite, void* valuePtr, CSharpValueType valueType) {
	SetCSharpReadValue(valuePtr, valueType, arr->Get(ite));
}
LC_SERDE_LIB_API void db_arr_ite_get_variant(SimpleJsonValueArray* arr, ArrayIterator ite, void* valuePtr, CSharpValueType* valueType) {
	*valueType = SetCSharpReadValue(valuePtr, arr->Get(ite));
}
LC_SERDE_LIB_API void db_arr_parse(SimpleJsonValueArray* db, Span<char> strv, vstd::funcPtr_t<void(Span<char>)> errorCallback, bool clearLast) {
	DBParse(db, strv, errorCallback, clearLast);
}
}// namespace toolhub::db
#endif

#ifdef VENGINE_PYTHON_SUPPORT
#include <serde_lib/Python_Extern.inl>
#endif

#endif