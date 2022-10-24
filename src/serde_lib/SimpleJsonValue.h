#pragma once
#include <serde_lib/IJsonObject.h>
#include <serde_lib/SimpleJsonLoader.h>
namespace toolhub::db {
class ConcurrentBinaryJson;
class SimpleBinaryJson;
struct SimpleJsonKey {
	using ValueType = vstd::variant<int64,
									vstd::string,
									vstd::Guid>;

	ValueType value;
	SimpleJsonKey(ValueType const& value)
		: value(value) {}
	SimpleJsonKey(ValueType&& value)
		: value(std::move(value)) {}
	SimpleJsonKey(Key const& v) {
		if (v.GetType() < ValueType::argSize) {
			switch (v.GetType()) {
				case ValueType::IndexOf<int64>:
					value.reset_as<int64>(v.force_get<int64>());
					break;
				case ValueType::IndexOf<vstd::string>:
					value.reset_as<vstd::string>(v.force_get<vstd::string_view>());
					break;
				case ValueType::IndexOf<vstd::Guid>:
					value.reset_as<vstd::Guid>(v.force_get<vstd::Guid>());
					break;
			}
		}
	}
	SimpleJsonKey(Key&& v)
		: SimpleJsonKey(v) {}
	static Key GetKey(ValueType const& value) {
		switch (value.GetType()) {
			case ValueType::IndexOf<int64>:
				return Key(value.force_get<int64>());
			case ValueType::IndexOf<vstd::Guid>:
				return Key(value.force_get<vstd::Guid>());
			case ValueType::IndexOf<vstd::string>:
				return Key(value.force_get<vstd::string>());
			default:
				return Key();
		}
	}
	Key GetKey() const {
		return GetKey(value);
	}
	int32 compare(Key const& key) const {
		if (key.GetType() == value.GetType()) {
			switch (value.GetType()) {
				case ValueType::IndexOf<int64>: {
					static const vstd::compare<int64> cm;
					return cm(value.force_get<int64>(), key.force_get<int64>());
				}
				case ValueType::IndexOf<vstd::Guid>: {
					static const vstd::compare<vstd::Guid> cm;
					return cm(value.force_get<vstd::Guid>(), key.force_get<vstd::Guid>());
				}
				case ValueType::IndexOf<vstd::string>: {
					static const vstd::compare<vstd::string_view> cm;
					return cm(value.force_get<vstd::string>(), key.force_get<vstd::string_view>());
				}
			}
			return 0;
		} else
			return (key.GetType() > value.GetType()) ? 1 : -1;
	}
	int32 compare(ValueType const& key) const {
		if (key.GetType() == value.GetType()) {
			switch (value.GetType()) {
				case ValueType::IndexOf<int64>: {
					static const vstd::compare<int64> cm;
					return cm(value.force_get<int64>(), key.force_get<int64>());
				}
				case ValueType::IndexOf<vstd::Guid>: {
					static const vstd::compare<vstd::Guid> cm;
					return cm(value.force_get<vstd::Guid>(), key.force_get<vstd::Guid>());
				}
				case ValueType::IndexOf<vstd::string>: {
					static const vstd::compare<vstd::string_view> cm;
					return cm(value.force_get<vstd::string>(), key.force_get<vstd::string>());
				}
				default:
					return 0;
			}
		} else
			return (key.GetType() > value.GetType()) ? 1 : -1;
	}
	size_t GetHashCode() const {
		auto getHash = [](auto&& v) {
			vstd::hash<std::remove_cvref_t<decltype(v)>> h;
			return h(v);
		};
		switch (value.GetType()) {
			case ValueType::IndexOf<int64>:
				return getHash(*reinterpret_cast<int64 const*>(value.GetPlaceHolder()));

			case ValueType::IndexOf<vstd::Guid>:
				return getHash(*reinterpret_cast<vstd::Guid const*>(value.GetPlaceHolder()));

			case ValueType::IndexOf<vstd::string>:
				return getHash(*reinterpret_cast<vstd::string const*>(value.GetPlaceHolder()));
			default:
				return 0;
		}
	}
};
struct SimpleJsonKeyHash {
	template<typename T>
	size_t operator()(T const& key) const {
		if constexpr (std::is_same_v<std::remove_cvref_t<T>, SimpleJsonKey>)
			return key.GetHashCode();
		else {
			return vstd::hash<T>()(key);
		}
	}
};
struct SimpleJsonKeyEqual {
	int32 operator()(SimpleJsonKey const& key, SimpleJsonKey const& v) const {
		return key.compare(v.value);
	}
	int32 operator()(SimpleJsonKey const& key, SimpleJsonKey::ValueType const& v) const {
		return key.compare(v);
	}

	int32 operator()(SimpleJsonKey const& key, Key const& t) const {
		return key.compare(t);
	}
	int32 operator()(SimpleJsonKey const& key, int64 const& t) const {
		if (key.value.index() == 0) {
			vstd::compare<int64> c;
			return c(key.value.template get<0>(), t);
		} else {
			return -1;
		}
	}
	int32 operator()(SimpleJsonKey const& key, vstd::string_view const& t) const {
		if (key.value.index() == 1) {
			vstd::compare<vstd::string_view> c;
			return c(key.value.template get<1>(), t);
		} else {
			return (key.value.index() < 1) ? 1 : -1;
		}
	}
	int32 operator()(SimpleJsonKey const& key, vstd::Guid const& t) const {
		if (key.value.index() == 2) {
			vstd::compare<vstd::Guid> c;
			return c(key.value.template get<2>(), t);
		} else {
			return 1;
		}
	}
};
using KVMap = vstd::HashMap<SimpleJsonKey, SimpleJsonVariant, SimpleJsonKeyHash, SimpleJsonKeyEqual>;

class SimpleJsonValueDict final : public IJsonDict {
public:
	KVMap vars;
	SimpleJsonValueDict();
	bool Contains(Key const& key) const override;
	//void Add(Key const& key, WriteJsonVariant&& value) override;

	~SimpleJsonValueDict();
	/* SimpleJsonValueDict(
		SimpleBinaryJson* db,
		IJsonDict* src);*/
	ReadJsonVariant Get(Key const& key) const override;

	void Set(Key const& key, WriteJsonVariant&& value) override;
	ReadJsonVariant TrySet(Key const& key, vstd::function<WriteJsonVariant()> const& value) override;
	void TryReplace(Key const& key, vstd::function<WriteJsonVariant(ReadJsonVariant const&)> const& value) override;
	void Remove(Key const& key) override;
	vstd::optional<WriteJsonVariant> GetAndRemove(Key const& key) override;
	vstd::optional<WriteJsonVariant> GetAndSet(Key const& key, WriteJsonVariant&& newValue) override;
	vstd::Iterator<JsonKeyPair> begin() const& override;
	vstd::Iterator<MoveJsonKeyPair> begin() && override;
	size_t Length() const override;
	luisa::vector<std::byte> Serialize() const override;
	void Serialize(luisa::vector<std::byte>& vec) const override;
	void M_GetSerData(luisa::vector<std::byte>& arr) const;
	void LoadFromSer(vstd::span<std::byte const>& arr);
	void LoadFromSer_DiffEnding(vstd::span<std::byte const>& arr);
	bool Read(vstd::span<std::byte const> sp,
			  bool clearLast) override;
	void Reset() override;
	void Reserve(size_t capacity) override;
	vstd::optional<ParsingException> Parse(
		vstd::string_view str, bool clearLast) override;
	bool IsEmpty() const override { return vars.size() == 0; }
	void M_Print(vstd::string& str) const;
	void M_Print_Compress(vstd::string& str) const;
	vstd::string FormattedPrint() const override {
		vstd::string str;
		M_Print(str);
		return str;
	}
	vstd::string Print() const override {
		vstd::string str;
		M_Print_Compress(str);
		return str;
	}
	vstd::MD5 GetMD5() const override;
};
namespace detail {
template<typename T>
struct VariantVector;
template<typename... Args>
struct VariantVector<vstd::variant<Args...>> {
	using Type = vstd::variant<luisa::vector<Args>..., luisa::vector<std::nullptr_t>>;
};
}// namespace detail
class SimpleJsonValueArray final : public IJsonArray {
private:
	using ArrayType = typename detail::VariantVector<WriteJsonVariant>::Type;
	template<typename Func>
	void MatchAndInit(WriteJsonVariant& var, Func&& func);
	template<typename Func>
	void Match(WriteJsonVariant& var, Func&& func);
	void ArrayTypeTraits(size_t typeIndex);
	// template <typename Func>
	// void Match(WriteJsonVariant& var, Func&& func) const;

public:
	ArrayType arr;
	SimpleJsonValueArray();
	~SimpleJsonValueArray();
	/* SimpleJsonValueArray(
		SimpleBinaryJson* db,
		IJsonArray* src);*/
	size_t Length() const override;
	void Reserve(size_t capacity) override;
	vstd::optional<ParsingException> Parse(
		vstd::string_view str,
		bool clearLast) override;
	luisa::vector<std::byte> Serialize() const override;
	void Serialize(luisa::vector<std::byte>& vec) const override;
	void M_GetSerData(luisa::vector<std::byte>& result) const;
	void LoadFromSer(vstd::span<std::byte const>& arr);
	void LoadFromSer_DiffEnding(vstd::span<std::byte const>& arr);
	bool Read(vstd::span<std::byte const> sp, bool clearLast) override;
	void Reset() override;
	vstd::optional<WriteJsonVariant> GetAndRemove(size_t index) override;
	vstd::optional<WriteJsonVariant> GetAndSet(size_t index, WriteJsonVariant&& newValue) override;
	void Set(size_t index, WriteJsonVariant&& value) override;
	ReadJsonVariant Get(size_t index) const override;
	void Remove(size_t index) override;
	void Add(WriteJsonVariant&& value) override;
	vstd::Iterator<ReadJsonVariant> begin() const& override;
	vstd::Iterator<WriteJsonVariant> begin() && override;
	bool IsEmpty() const override { return Length() == 0; }
	void M_Print(vstd::string& str) const;
	vstd::string FormattedPrint() const override {
		vstd::string str;
		M_Print(str);
		return str;
	}
	void M_Print_Compress(vstd::string& str) const;
	vstd::string Print() const override {
		vstd::string str;
		M_Print_Compress(str);
		return str;
	}
	vstd::MD5 GetMD5() const override;
};
}// namespace toolhub::db