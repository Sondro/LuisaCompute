#include "tex_descriptor.h"
namespace toolhub::spv {
TexDescriptor::TexDescriptor(
	InternalType vectorType,
	uint dimension,
	TexType texType)
	: vectorType(vectorType),
	  dimension(dimension),
	  texType(texType) {
	assert(dimension >= 2 && dimension <= 4);
	assert(vectorType.tag != InternalType::Tag::BOOL && vectorType.tag != InternalType::Tag::MATRIX);
    
}
}// namespace toolhub::spv