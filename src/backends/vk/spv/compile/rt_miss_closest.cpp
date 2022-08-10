#include "rt_miss_closest.h"
#include <vstl/BinaryReader.h>
namespace toolhub::spv {
RTMissClosestData::RTMissClosestData(vstd::string const& path) {
	BinaryReader reader(path);
	reader.Read(&missSize, sizeof(size_t));
	reader.Read(&closestSize, sizeof(size_t));
	data.resize(missSize + closestSize);
	reader.Read(data.data(), data.byte_size());
}
RTMissClosestData::~RTMissClosestData() {}
vstd::span<uint const> RTMissClosestData::MissKernel() const {
	return {
		reinterpret_cast<uint const*>(data.data()),
		missSize / sizeof(uint)};
}
vstd::span<uint const> RTMissClosestData::ClosestKernel() const {
	return {
		reinterpret_cast<uint const*>(data.data() + missSize),
		closestSize / sizeof(uint)};
}
}// namespace toolhub::spv