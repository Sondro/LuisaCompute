
#include <serde_lib/SimpleJsonValue.h>
#include <vstl/Common.h>
#include <vstl/StringUtility.h>
namespace toolhub::db {

template<typename Func>
void SimpleJsonValueArray::MatchAndInit(WriteJsonVariant& var, Func&& func) {
	if (var.valid()) {
		if (arr.valid()) {
			ArrayTypeTraits(var.index());
		} else {
			arr.reset_as(var.index());
		}
		arr.visit([&]<typename T>(T& arr) {
			var.visit([&]<typename Ele>(Ele& var) {
				if constexpr (std::is_constructible_v<typename T::value_type, Ele&>) {
					func(arr, typename T::value_type(var));
				}
			});
		});
	} else {
		if (var.valid()) {
			assert(arr.index() == WriteJsonVariant::argSize);
		} else {
			arr.reset_as(WriteJsonVariant::argSize);
		}
		auto&& nullVec = arr.get<WriteJsonVariant::argSize>();
		func(nullVec, nullptr);
	}
}

template<typename Func>
void SimpleJsonValueArray::Match(WriteJsonVariant& var, Func&& func) {
	if (var.valid()) {
		assert(arr.valid());
		ArrayTypeTraits(var.index());
		arr.visit([&]<typename T>(T& arr) {
			var.visit([&]<typename Ele>(Ele& var) {
				if constexpr (std::is_constructible_v<typename T::value_type, Ele&>) {
					func(arr, typename T::value_type(var));
				}
			});
		});
	} else {
		assert(arr.index() == WriteJsonVariant::argSize);
		auto&& nullVec = arr.get<WriteJsonVariant::argSize>();
		func(nullVec, nullptr);
	}
}
/*template<typename Func>
void SimpleJsonValueArray::Match(WriteJsonVariant& var, Func&& func) const {
	if (var.valid()) {
		assert(arr.index() == var.index());
		arr.visit([&]<typename T>(T const& arr) {
			var.visit([&]<typename Ele>(Ele& var) {
				if constexpr (std::is_same_v<T, Ele>) {
					func(arr, var);
				}
			});
		});
	} else {
		assert(arr.index() == WriteJsonVariant::argSize);
		auto&& nullVec = arr.get<WriteJsonVariant::argSize>();
		func(nullVec, nullptr);
	}
}*/
//Bind Python Object
class DictIEnumerator : public vstd::IEnumerable<JsonKeyPair> {
public:
	KVMap::Iterator ite;
	KVMap::Iterator end;
	DictIEnumerator(
		KVMap::Iterator&& ite,
		KVMap::Iterator&& end)
		: ite(ite),
		  end(end) {}
	JsonKeyPair GetValue() override {
		return JsonKeyPair{ite->first.GetKey(), ite->second.GetVariant()};
	};
	bool End() override {
		return ite == end;
	};
	void GetNext() override {
		++ite;
	};
};

template<typename T>
class ArrayIEnumerator : public vstd::IEnumerable<ReadJsonVariant> {
public:
	T const* ite;
	T const* end;
	ArrayIEnumerator(
		T const* ite,
		T const* end)
		: ite(ite),
		  end(end) {}
	ReadJsonVariant GetValue() override {
		return SimpleJsonVariant::GetVariantFromValue(*ite);
	};
	bool End() override {
		return ite == end;
	};
	void GetNext() override {
		++ite;
	};
};
template<typename Func>
class MoveDictIEnumerator : public vstd::IEnumerable<MoveJsonKeyPair> {
public:
	Func f;
	KVMap::Iterator ite;
	KVMap::Iterator end;
	MoveDictIEnumerator(
		Func&& func,
		KVMap::Iterator&& ite,
		KVMap::Iterator&& end)
		: ite(ite),
		  f(std::move(func)),
		  end(end) {}
	~MoveDictIEnumerator() { f(); }
	MoveJsonKeyPair GetValue() override {
		return MoveJsonKeyPair{ite->first.GetKey(), std::move(ite->second.value)};
	};
	bool End() override {
		return ite == end;
	};
	void GetNext() override {
		++ite;
	};
};
template<typename T, typename Func>
class MoveArrayIEnumerator : public vstd::IEnumerable<WriteJsonVariant> {
public:
	Func f;
	T* ite;
	T* end;
	MoveArrayIEnumerator(
		Func&& func,
		T* ite,
		T* end)
		: ite(ite),
		  f(std::move(func)),
		  end(end) {}
	~MoveArrayIEnumerator() { f(); }
	WriteJsonVariant GetValue() override {
		return {std::move(*ite)};
	};
	bool End() override {
		return ite == end;
	};
	void GetNext() override {
		++ite;
	};
};
struct BinaryHeader {
	uint64 preDefine;
	uint64 size;
	uint64 postDefine;
};
static void SerPreProcess(vstd::vector<std::byte>& data) {
	data.resize(data.size() + sizeof(uint64));
}
static bool IsMachineLittleEnding() {
	uint i = 0x12345678;
	uint8_t* p = reinterpret_cast<uint8_t*>(&i);
	return (*p == 0x78) && (*(p + 1) == 0x56);
}
template<bool isDict>
static void SerPostProcess(vstd::vector<std::byte>& data, size_t initOffset) {
	uint64 hashValue;
	vstd::hash<BinaryHeader> hasher;
	if constexpr (isDict) {
		hashValue = hasher(BinaryHeader{3551484578062615571ull, (data.size() - initOffset), 13190554206192427769ull});
	} else {
		hashValue = hasher(BinaryHeader{917074095154020627ull, (data.size() - initOffset), 12719994496415311585ull});
	}
	*reinterpret_cast<uint64*>(data.data() + initOffset) = hashValue;
}
template<bool isDict>
static bool DeserCheck(vstd::span<std::byte const>& sp, bool& sameEnding) {
	uint64 hashValue;
	vstd::hash<BinaryHeader> hasher;
	auto ending = vstd::SerDe<std::byte>::Get(sp);
	sameEnding = (bool(ending) == IsMachineLittleEnding());
	if constexpr (isDict) {
		hashValue = hasher(BinaryHeader{3551484578062615571ull, sp.size(), 13190554206192427769ull});
	} else {
		hashValue = hasher(BinaryHeader{917074095154020627ull, sp.size(), 12719994496415311585ull});
	}
	auto hashInside = vstd::SerDe<uint64>::Get(sp);
	if (!sameEnding)
		vstd::ReverseBytes(hashInside);
	return hashValue == hashInside;
}
static void PrintString(vstd::string const& str, vstd::string& result) {
	result << '\"';
	char const* last = str.data();
	auto Flush = [&](char const* ptr) {
		result.append(last, ptr - last);
		last = ptr + 1;
	};
	for (auto&& i : str) {
		switch (i) {
			case '\t':
				Flush(&i);
				result += "\\t"_sv;
				break;
			case '\r':
				Flush(&i);
				result += "\\r"_sv;
				break;
			case '\n':
				Flush(&i);
				result += "\\n"_sv;
				break;
			case '\'':
				Flush(&i);
				result += "\\\'"_sv;
				break;
			case '\"':
				Flush(&i);
				result += "\\\""_sv;
				break;
		}
	}
	Flush(str.data() + str.size());
	result << '\"';
}
template<typename Dict, typename Array>
static void PrintSimpleJsonVariant(SimpleJsonVariant const& v, vstd::string& str, bool emptySpaceBeforeOb) {
	auto func = [&](auto&& v) {
		//str.append(valueLayer, '\t');
		auto a = vstd::to_string(v);
		vstd::to_string(v, str);
	};
	switch (v.value.GetType()) {
		case WriteJsonVariant::IndexOf<int64>:
			func(v.value.template force_get<int64>());
			break;
		case WriteJsonVariant::IndexOf<double>:
			func(v.value.template force_get<double>());
			break;
		case WriteJsonVariant::IndexOf<vstd::string>:
			[&](vstd::string const& s) {
				//str.append(valueLayer, '\t');
				auto&& ss = str;
				PrintString(s, str);
			}(v.value.template force_get<vstd::string>());
			break;
		case WriteJsonVariant::IndexOf<vstd::unique_ptr<IJsonDict>>:
			[&](vstd::unique_ptr<IJsonDict> const& ptr) {
				if (emptySpaceBeforeOb)
					str << '\n';
				auto&& ss = str;
				static_cast<Dict*>(ptr.get())->M_Print(str);
			}(v.value.template force_get<vstd::unique_ptr<IJsonDict>>());
			break;
		case WriteJsonVariant::IndexOf<vstd::unique_ptr<IJsonArray>>:
			[&](vstd::unique_ptr<IJsonArray> const& ptr) {
				auto&& ssr = str;
				if (emptySpaceBeforeOb)
					str << '\n';
				auto&& ss = str;
				static_cast<Array*>(ptr.get())->M_Print(str);
			}(v.value.template force_get<vstd::unique_ptr<IJsonArray>>());
			break;
		case WriteJsonVariant::IndexOf<vstd::Guid>:
			[&](vstd::Guid const& guid) {
				//str.append(valueLayer, '\t');
				str << '$';
				size_t offst = str.size();
				str.resize(offst + 32);
				guid.ToString(str.data() + offst, true);
			}(v.value.template force_get<vstd::Guid>());
			break;
		case WriteJsonVariant::IndexOf<bool>:
			if (v.value.template force_get<bool>())
				str += "true";
			else
				str += "false";
			break;
		default:
			str += "null";
			break;
	}
}
template<typename T, typename Dict, typename Array>
static void PrintElement(T const& v, vstd::string& str, bool emptySpaceBeforeOb) {
	auto func = [&](auto&& v) {
		//str.append(valueLayer, '\t');
		auto a = vstd::to_string(v);
		vstd::to_string(v, str);
	};

	if constexpr (std::is_same_v<T, int64>) {
		func(v);
	} else if constexpr (std::is_same_v<T, double>) {
		func(v);

	} else if constexpr (std::is_same_v<T, vstd::string>) {
		auto&& ss = str;
		PrintString(v, str);
	} else if constexpr (std::is_same_v<T, vstd::unique_ptr<IJsonDict>>) {
		if (emptySpaceBeforeOb)
			str << '\n';
		auto&& ss = str;
		static_cast<Dict*>(v.get())->M_Print(str);
	} else if constexpr (std::is_same_v<T, vstd::unique_ptr<IJsonArray>>) {
		auto&& ssr = str;
		if (emptySpaceBeforeOb)
			str << '\n';
		auto&& ss = str;
		static_cast<Array*>(v.get())->M_Print(str);
	} else if constexpr (std::is_same_v<T, vstd::Guid>) {
		//str.append(valueLayer, '\t');
		str << '$';
		size_t offst = str.size();
		str.resize(offst + 32);
		v.ToString(str.data() + offst, true);
	} else if constexpr (std::is_same_v<T, bool>) {
		if (v)
			str += "true"_sv;
		else
			str += "false"_sv;
	} else {
		str += "null"_sv;
	}
}
template<typename Dict, typename Array>
static void CompressPrintSimpleJsonVariant(SimpleJsonVariant const& v, vstd::string& str) {
	auto func = [&](auto&& v) {
		vstd::to_string(v, str);
	};
	switch (v.value.GetType()) {
		case WriteJsonVariant::IndexOf<int64>:
			func(v.value.template force_get<int64>());
			break;
		case WriteJsonVariant::IndexOf<double>:
			func(v.value.template force_get<double>());
			break;
		case WriteJsonVariant::IndexOf<vstd::string>:
			[&](vstd::string const& s) {
				PrintString(s, str);
			}(v.value.template force_get<vstd::string>());
			break;
		case WriteJsonVariant::IndexOf<vstd::unique_ptr<IJsonDict>>:
			[&](vstd::unique_ptr<IJsonDict> const& ptr) {
				static_cast<Dict*>(ptr.get())->M_Print_Compress(str);
			}(v.value.template force_get<vstd::unique_ptr<IJsonDict>>());
			break;
		case WriteJsonVariant::IndexOf<vstd::unique_ptr<IJsonArray>>:
			[&](vstd::unique_ptr<IJsonArray> const& ptr) {
				static_cast<Array*>(ptr.get())->M_Print_Compress(str);
			}(v.value.template force_get<vstd::unique_ptr<IJsonArray>>());
			break;
		case WriteJsonVariant::IndexOf<vstd::Guid>:
			[&](vstd::Guid const& guid) {
				str << '$';
				size_t offst = str.size();
				str.resize(offst + 32);
				guid.ToString(str.data() + offst, true);
			}(v.value.template force_get<vstd::Guid>());
			break;
		case WriteJsonVariant::IndexOf<bool>:
			if (v.value.template force_get<bool>())
				str += "true";
			else
				str += "false";
			break;
		case WriteJsonVariant::IndexOf<std::nullptr_t>:
			str += "null";
			break;
	}
}
template<typename T, typename Dict, typename Array>
static void CompressPrintElement(T const& v, vstd::string& str) {
	auto func = [&](auto&& v) {
		vstd::to_string(v, str);
	};
	if constexpr (std::is_same_v<T, int64>) {
		func(v);
	} else if constexpr (std::is_same_v<T, double>) {
		func(v);
	} else if constexpr (std::is_same_v<T, vstd::string>) {
		PrintString(v, str);
	} else if constexpr (std::is_same_v<T, vstd::unique_ptr<IJsonDict>>) {
		static_cast<Dict*>(v.get())->M_Print_Compress(str);
	} else if constexpr (std::is_same_v<T, vstd::unique_ptr<IJsonArray>>) {
		static_cast<Array*>(v.get())->M_Print_Compress(str);
	} else if constexpr (std::is_same_v<T, vstd::Guid>) {
		//str.append(valueLayer, '\t');
		str << '$';
		size_t offst = str.size();
		str.resize(offst + 32);
		v.ToString(str.data() + offst, true);
	} else if constexpr (std::is_same_v<T, bool>) {
		if (v)
			str += "true"_sv;
		else
			str += "false"_sv;
	} else {
		str += "null"_sv;
	}
}
template<typename Dict, typename Array>
static void PrintSimpleJsonVariantYaml(SimpleJsonVariant const& v, vstd::string& str, size_t space) {
	auto func = [&](auto&& v) {
		vstd::to_string(v, str);
	};
	switch (v.value.GetType()) {
		case WriteJsonVariant::IndexOf<int64>:
			func(v.value.template force_get<int64>());
			break;
		case WriteJsonVariant::IndexOf<double>:
			func(v.value.template force_get<double>());
			break;
		case WriteJsonVariant::IndexOf<vstd::string>:
			[&](vstd::string const& s) {
				PrintString(s, str);
			}(v.value.template force_get<vstd::string>());
			break;
		case WriteJsonVariant::IndexOf<vstd::unique_ptr<IJsonDict>>:
			[&](vstd::unique_ptr<IJsonDict> const& ptr) {
				static_cast<Dict*>(ptr.get())->PrintYaml(str, space + 2);
			}(v.value.template force_get<vstd::unique_ptr<IJsonDict>>());
			break;
		case WriteJsonVariant::IndexOf<vstd::unique_ptr<IJsonArray>>:
			[&](vstd::unique_ptr<IJsonArray> const& ptr) {
				static_cast<Array*>(ptr.get())->PrintYaml(str, space + 2);
			}(v.value.template force_get<vstd::unique_ptr<IJsonArray>>());
			break;
		case WriteJsonVariant::IndexOf<vstd::Guid>:
			[&](vstd::Guid const& guid) {
				size_t offst = str.size();
				str.resize(offst + 32);
				guid.ToString(str.data() + offst, true);
			}(v.value.template force_get<vstd::Guid>());
			break;
		case WriteJsonVariant::IndexOf<bool>:
			if (v.value.template force_get<bool>())
				str += "true";
			else
				str += "false";
			break;
		default:
			str += "null";
			break;
	}
}

static void PrintKeyVariant(SimpleJsonKey const& v, vstd::string& str) {
	auto func = [&](auto&& v) {
		vstd::to_string(v, str);
	};
	switch (v.value.GetType()) {
		case SimpleJsonKey::ValueType::IndexOf<int64>:
			vstd::to_string(v.value.template force_get<int64>(), str);
			break;
		case SimpleJsonKey::ValueType::IndexOf<vstd::string>:
			PrintString(v.value.template force_get<vstd::string>(), str);
			break;
		case SimpleJsonKey::ValueType::IndexOf<vstd::Guid>:
			[&](vstd::Guid const& guid) {
				str << '$';
				size_t offst = str.size();
				str.resize(offst + 32);
				guid.ToString(str.data() + offst, true);
			}(v.value.template force_get<vstd::Guid>());
			break;
	}
}
template<typename Dict, typename Array>
static void PrintDict(KVMap const& vars, vstd::string& str) {
	//str.append(space, '\t');
	str << "{\n"_sv;
	auto disp = vstd::scope_exit([&]() {
		//str.append(space, '\t');
		str << '}';
	});
	size_t varsSize = vars.size() - 1;
	size_t index = 0;
	for (auto&& i : vars) {
		//str.append(space, '\t');
		PrintKeyVariant(i.first, str);
		str << ": "_sv;
		PrintSimpleJsonVariant<Dict, Array>(i.second, str, true);
		if (index == varsSize) {
			str << '\n';
		} else {
			str << ",\n"_sv;
		}
		index++;
	}
}
template<typename Dict, typename Array>
static void CompressPrintDict(KVMap const& vars, vstd::string& str) {
	str << '{';
	auto disp = vstd::scope_exit([&]() {
		str << '}';
	});
	size_t varsSize = vars.size() - 1;
	size_t index = 0;
	for (auto&& i : vars) {
		PrintKeyVariant(i.first, str);
		str << ':';
		CompressPrintSimpleJsonVariant<Dict, Array>(i.second, str);
		if (index != varsSize) {
			str << ',';
		}
		index++;
	}
}
template<typename Vector, typename Dict, typename Array>
static void PrintArray(Vector const& arr, vstd::string& str) {
	//str.append(space, '\t');
	str << "[\n"_sv;
	auto disp = vstd::scope_exit([&]() {
		//str.append(space, '\t');
		str << ']';
	});
	size_t arrSize = arr.size() - 1;
	size_t index = 0;
	for (auto&& i : arr) {
		PrintElement<typename Vector::value_type, Dict, Array>(i, str, false);
		if (index == arrSize) {
			str << '\n';
		} else {
			str << ",\n"_sv;
		}
		index++;
	}
}
template<typename Vector, typename Dict, typename Array>
static void CompressPrintArray(Vector const& arr, vstd::string& str) {
	str << '[';
	auto disp = vstd::scope_exit([&]() {
		str << ']';
	});
	size_t arrSize = arr.size() - 1;
	size_t index = 0;
	for (auto&& i : arr) {
		CompressPrintElement<typename Vector::value_type, Dict, Array>(i, str);
		if (index != arrSize) {
			str << ',';
		}
		index++;
	}
}
//////////////////////////  Single Thread
SimpleJsonValueDict::SimpleJsonValueDict()
	: vars(4) {
}
SimpleJsonValueDict::~SimpleJsonValueDict() {
}
bool SimpleJsonValueDict::Contains(Key const& key) const {
	return vars.Find(key);
}
ReadJsonVariant SimpleJsonValueDict::Get(Key const& key) const {
	auto ite = vars.Find(key);
	if (ite) {
		return ite.Value().GetVariant();
	}
	return {};
}
void SimpleJsonValueDict::Set(Key const& key, WriteJsonVariant&& value) {
	vars.ForceEmplace(key, std::move(value));
}
ReadJsonVariant SimpleJsonValueDict::TrySet(Key const& key, vstd::function<WriteJsonVariant()> const& value) {
	return vars.Emplace(key, vstd::LazyEval(value)).Value().GetVariant();
}

void SimpleJsonValueDict::TryReplace(Key const& key, vstd::function<WriteJsonVariant(ReadJsonVariant const&)> const& value) {
	auto ite = vars.TryEmplace(key);
	if (ite.second) {
		ite.first.Value().value = value({});
	} else {
		ite.first.Value().value = value(ite.first.Value().GetVariant());
	}
}
void SimpleJsonValueDict::Remove(Key const& key) {
	vars.Remove(key);
}
vstd::optional<WriteJsonVariant> SimpleJsonValueDict::GetAndRemove(Key const& key) {
	auto ite = vars.Find(key);
	if (!ite) return {};
	auto d = vstd::scope_exit([&] {
		vars.Remove(ite);
	});
	return std::move(ite.Value().value);
}
vstd::optional<WriteJsonVariant> SimpleJsonValueDict::GetAndSet(Key const& key, WriteJsonVariant&& newValue) {
	auto ite = vars.Find(key);
	if (!ite) {
		vars.Emplace(key, std::move(newValue));
		return {};
	}
	auto d = vstd::scope_exit([&] {
		ite.Value() = std::move(newValue);
	});
	return std::move(ite.Value().value);
}

size_t SimpleJsonValueDict::Length() const {
	return vars.size();
}
vstd::vector<std::byte> SimpleJsonValueDict::Serialize() const {
	vstd::vector<std::byte> result;
	result.emplace_back(IsMachineLittleEnding() ? static_cast<std::byte>(std::numeric_limits<uint8_t>::max()) : static_cast<std::byte>(0));
	SerPreProcess(result);
	M_GetSerData(result);
	SerPostProcess<true>(result, 1);
	return result;
}
void SimpleJsonValueDict::Serialize(vstd::vector<std::byte>& result) const {
	result.emplace_back(IsMachineLittleEnding() ? static_cast<std::byte>(std::numeric_limits<uint8_t>::max()) : static_cast<std::byte>(0));
	auto sz = result.size();
	SerPreProcess(result);
	M_GetSerData(result);
	SerPostProcess<true>(result, sz);
}
void SimpleJsonValueDict::M_GetSerData(vstd::vector<std::byte>& data) const {
	PushDataToVector<uint64>(vars.size(), data);
	for (auto&& kv : vars) {
		PushDataToVector(kv.first.value, data);
		SimpleJsonLoader::Serialize(kv.second, data);
	}
}

void SimpleJsonValueDict::LoadFromSer(vstd::span<std::byte const>& sp) {
	auto sz = PopValue<uint64>(sp);
	vars.reserve(sz);
	for (auto i : vstd::range(sz)) {
		auto key = PopValue<SimpleJsonKey::ValueType>(sp);

		auto value = SimpleJsonLoader::DeSerialize(sp);
		if (key.GetType() == 1) {
			auto&& s = key.template force_get<vstd::string>();
			int x = 0;
		}
		vars.Emplace(std::move(key), std::move(value));
	}
}
void SimpleJsonValueDict::LoadFromSer_DiffEnding(vstd::span<std::byte const>& sp) {
	auto sz = PopValueReverse<uint64>(sp);
	vars.reserve(sz);
	for (auto i : vstd::range(sz)) {
		auto key = PopValueReverse<SimpleJsonKey::ValueType>(sp);
		auto value = SimpleJsonLoader::DeSerialize_DiffEnding(sp);
		vars.Emplace(std::move(key), std::move(value));
	}
}

void SimpleJsonValueDict::Reserve(size_t capacity) {
	vars.reserve(capacity);
}
void SimpleJsonValueArray::Reserve(size_t capacity) {
	arr.visit([&]<typename T>(T& t) {
		t.reserve(capacity);
	});
}

void SimpleJsonValueDict::Reset() {
	vars.Clear();
}

SimpleJsonValueArray::SimpleJsonValueArray() {
}
SimpleJsonValueArray::~SimpleJsonValueArray() {
}

size_t SimpleJsonValueArray::Length() const {
	return arr.visit_or(
		size_t(0),
		[&]<typename T>(T& t) {
			return t.size();
		});
}

vstd::vector<std::byte> SimpleJsonValueArray::Serialize() const {
	vstd::vector<std::byte> result;
	result.emplace_back(IsMachineLittleEnding() ? static_cast<std::byte>(std::numeric_limits<uint8_t>::max()) : static_cast<std::byte>(0));
	SerPreProcess(result);
	M_GetSerData(result);
	SerPostProcess<false>(result, 1);
	return result;
}
void SimpleJsonValueArray::Serialize(vstd::vector<std::byte>& result) const {
	result.emplace_back(IsMachineLittleEnding() ? static_cast<std::byte>(std::numeric_limits<uint8_t>::max()) : static_cast<std::byte>(0));
	auto sz = result.size();
	SerPreProcess(result);
	M_GetSerData(result);
	SerPostProcess<false>(result, sz);
}

void SimpleJsonValueArray::M_GetSerData(vstd::vector<std::byte>& data) const {
	PushDataToVector<uint64>(Length(), data);
	PushDataToVector<uint8_t>(arr.index(), data);
	arr.visit([&]<typename T>(T const& arr) {
		if constexpr (std::is_same_v<T, vstd::vector<int64>>) {
			for (auto&& v : arr) {
				PushDataToVector(v, data);
			}
		} else if constexpr (std::is_same_v<T, vstd::vector<double>>) {
			for (auto&& v : arr) {
				PushDataToVector(v, data);
			}
		} else if constexpr (std::is_same_v<T, vstd::vector<vstd::string>>) {
			for (auto&& v : arr) {
				PushDataToVector(v, data);
			}
		} else if constexpr (std::is_same_v<T, vstd::vector<vstd::unique_ptr<IJsonDict>>>) {
			for (auto&& v : arr) {
				static_cast<SimpleJsonValueDict*>(v.get())->M_GetSerData(data);
			}
		} else if constexpr (std::is_same_v<T, vstd::vector<vstd::unique_ptr<IJsonArray>>>) {
			for (auto&& v : arr) {
				static_cast<SimpleJsonValueArray*>(v.get())->M_GetSerData(data);
			}
		} else if constexpr (std::is_same_v<T, vstd::vector<vstd::Guid>>) {
			for (auto&& v : arr) {
				PushDataToVector(v, data);
			}
		} else if constexpr (std::is_same_v<T, vstd::vector<bool>>) {
			for (auto&& v : arr) {
				PushDataToVector(v, data);
			}
		}
	});
}

void SimpleJsonValueArray::LoadFromSer(vstd::span<std::byte const>& sp) {
	auto sz = PopValue<uint64>(sp);
	auto arrIndex = PopValue<std::byte>(sp);
	Reserve(sz);
	for (auto i : vstd::range(sz)) {
		auto v = SimpleJsonLoader::DeSerialize(sp, (ValueType)arrIndex);
		Add(std::move(v.value));
	}
}
void SimpleJsonValueArray::LoadFromSer_DiffEnding(vstd::span<std::byte const>& sp) {
	auto sz = PopValueReverse<uint64>(sp);
	auto arrIndex = PopValueReverse<std::byte>(sp);
	Reserve(sz);
	for (auto i : vstd::range(sz)) {
		auto v = SimpleJsonLoader::DeSerialize_DiffEnding(sp, (ValueType)arrIndex);
		Add(std::move(v.value));
	}
}

void SimpleJsonValueArray::Reset() {
	arr.visit([&](auto&& t) {
		t.clear();
	});
}

ReadJsonVariant SimpleJsonValueArray::Get(size_t index) const {
	assert(arr.valid() && index < Length());
	return arr.visit_or(
		ReadJsonVariant(nullptr),
		[&](auto&& arr) -> ReadJsonVariant {
			return SimpleJsonVariant::GetVariantFromValue(arr[index]);
		});
}

void SimpleJsonValueArray::Set(size_t index, WriteJsonVariant&& value) {
	assert(arr.valid() && index < Length());
	Match(
		value,
		[&](auto&& arr, auto&& v) {
			arr[index] = std::move(v);
		});
}

void SimpleJsonValueArray::Remove(size_t index) {
	assert(arr.valid() && index < Length());
	arr.visit([&](auto&& t) {
		t.erase(t.begin() + index);
	});
}
vstd::optional<WriteJsonVariant> SimpleJsonValueArray::GetAndRemove(size_t index) {
	assert(arr.valid() && index < Length());
	return arr.visit_or(
		vstd::UndefEval<vstd::optional<WriteJsonVariant>>{},
		[&](auto&& arr) -> vstd::optional<WriteJsonVariant> {
			vstd::optional<WriteJsonVariant> v;
			v.New(std::move(arr[index]));
			arr.erase(arr.begin() + index);
			return v;
		});
}
vstd::optional<WriteJsonVariant> SimpleJsonValueArray::GetAndSet(size_t index, WriteJsonVariant&& newValue) {
	assert(arr.valid() && index < Length());
	vstd::optional<WriteJsonVariant> result;
	Match(
		newValue,
		[&](auto&& arr, auto&& v) {
			auto&& value = arr[index];
			result.New(std::move(value));
			vstd::reset(value, std::move(v));
		});
	return result;
}

void SimpleJsonValueArray::Add(WriteJsonVariant&& value) {
	MatchAndInit(
		value,
		[&](auto&& arr, auto&& v) {
			arr.emplace_back(std::move(v));
		});
}
void SimpleJsonValueDict::M_Print(vstd::string& str) const {
	PrintDict<SimpleJsonValueDict, SimpleJsonValueArray>(vars, str);
}
void SimpleJsonValueArray::M_Print(vstd::string& str) const {
	if (arr.valid()) {
		arr.visit([&]<typename T>(T const& arr) {
			PrintArray<T, SimpleJsonValueDict, SimpleJsonValueArray>(arr, str);
		});
	} else {
		str << "[]";
	}
}
void SimpleJsonValueDict::M_Print_Compress(vstd::string& str) const {
	CompressPrintDict<SimpleJsonValueDict, SimpleJsonValueArray>(vars, str);
}
void SimpleJsonValueArray::M_Print_Compress(vstd::string& str) const {
	if (arr.valid()) {
		arr.visit([&]<typename T>(T const& arr) {
			CompressPrintArray<T, SimpleJsonValueDict, SimpleJsonValueArray>(arr, str);
		});
	} else {
		str << "[]";
	}
}
vstd::MD5 SimpleJsonValueDict::GetMD5() const {
	vstd::vector<std::byte> vec;
	M_GetSerData(vec);
	return vstd::MD5(vstd::span<uint8_t const>{
		reinterpret_cast<uint8_t const*>(vec.data()),
		vec.size()});
}
vstd::MD5 SimpleJsonValueArray::GetMD5() const {
	vstd::vector<std::byte> vec;
	M_GetSerData(vec);
	return vstd::MD5(vstd::span<uint8_t const>{
		reinterpret_cast<uint8_t const*>(vec.data()),
		vec.size()});
}
bool SimpleJsonValueDict::Read(vstd::span<std::byte const> sp, bool clearLast) {
	bool sameEnding;
	if (!DeserCheck<true>(sp, sameEnding)) return false;
	if (clearLast) {
		vars.Clear();
	}
	if (sameEnding) {
		LoadFromSer(sp);
	} else {
		LoadFromSer_DiffEnding(sp);
	}
	return true;
}

bool SimpleJsonValueArray::Read(vstd::span<std::byte const> sp, bool clearLast) {
	bool sameEnding;
	if (!DeserCheck<false>(sp, sameEnding)) return false;
	if (clearLast) {
		Reset();
	}
	if (sameEnding) {
		LoadFromSer(sp);
	} else {
		LoadFromSer_DiffEnding(sp);
	}
	return true;
}
vstd::Iterator<JsonKeyPair> SimpleJsonValueDict::begin() const& {
	return [&](void* ptr) { return new (ptr) DictIEnumerator(vars.begin(), vars.end()); };
}

vstd::Iterator<ReadJsonVariant> SimpleJsonValueArray::begin() const& {
	return arr.visit_or(
		vstd::UndefEval<vstd::Iterator<ReadJsonVariant>>{},
		[&](auto&& arr) -> vstd::Iterator<ReadJsonVariant> {
			return [&](void* ptr) { return new (ptr) ArrayIEnumerator(arr.begin(), arr.end()); };
		});
}
vstd::Iterator<MoveJsonKeyPair> SimpleJsonValueDict::begin() && {
	return [&](void* ptr) {
		auto deleterFunc = [&] { vars.Clear(); };
		return new (ptr) MoveDictIEnumerator<decltype(deleterFunc)>(std::move(deleterFunc), vars.begin(), vars.end()); };
}

vstd::Iterator<WriteJsonVariant> SimpleJsonValueArray::begin() && {
	return arr.visit_or(
		vstd::UndefEval<vstd::Iterator<WriteJsonVariant>>{},
		[&]<typename Arr>(Arr& arr) -> vstd::Iterator<WriteJsonVariant> {
			return [&](void* ptr) {
				auto deleterFunc = [&] { arr.clear(); };
				return new (ptr) MoveArrayIEnumerator<typename Arr::value_type, decltype(deleterFunc)>(std::move(deleterFunc), arr.begin(), arr.end()); };
		});
}
void SimpleJsonValueArray::ArrayTypeTraits(size_t typeIndex) {
	if (arr.index() == typeIndex || (typeIndex == WriteJsonVariant::IndexOf<int64> && arr.index() == WriteJsonVariant::IndexOf<double>)) return;
	if (typeIndex == WriteJsonVariant::IndexOf<double> && arr.index() == WriteJsonVariant::IndexOf<int64>) {
		auto& int64Vec = arr.get<WriteJsonVariant::IndexOf<int64>>();
		if (!int64Vec.empty()) {
			// I confess
			// Use magic here: reinterpret vector<int64> to vector<double>
			auto doubleVec = std::move(reinterpret_cast<vstd::vector<double>&>(int64Vec));
			for(auto&& i : doubleVec){
				auto doubleValue = static_cast<double>(reinterpret_cast<int64&>(i));
				i = doubleValue;
			}
			arr = std::move(doubleVec);
		}
	} else {
#ifdef DEBUG
		arr.visit([&](auto&& arr) {
			assert(arr.empty());
		});
#endif
		arr.reset_as(typeIndex);
	}
}

}// namespace toolhub::db