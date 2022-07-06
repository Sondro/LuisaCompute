#include "internal_type.h"
namespace toolhub::spv {
vstd::optional<InternalType> InternalType::GetType(Type const* type) {
	if (static_cast<uint>(type->tag()) > static_cast<uint>(Type::Tag::MATRIX)) return {};
	vstd::optional<InternalType> tp;
	tp.New();
	switch (type->tag()) {
		case Type::Tag::MATRIX:
			tp->tag = Tag::MATRIX;
			tp->dimension = type->dimension();
			break;
		case Type::Tag::VECTOR:
			tp->dimension = type->dimension();
			tp->tag = static_cast<Tag>(type->element()->tag());
			break;
		default:
			tp->tag = static_cast<Tag>(type->tag());
			tp->dimension = 1;
			break;
	}
	if (type->element()) {
		tp->tag = static_cast<Tag>(type->element()->tag());
	}
	return tp;
}
Id InternalType::TypeId() const {
	if (tag == Tag::MATRIX) {
		return Id::MatId(dimension);
	} else {
		auto scalarTag = [&] {
			switch (tag) {
				case Tag::BOOL:
					return Id::BoolId();
				case Tag::FLOAT:
					return Id::FloatId();
				case Tag::INT:
					return Id::IntId();
				case Tag::UINT:
					return Id::UIntId();
			}
		}();
		if (dimension > 1) {
			scalarTag = Id::VecId(scalarTag, dimension);
		}
		return scalarTag;
	}
}
}// namespace toolhub::spv