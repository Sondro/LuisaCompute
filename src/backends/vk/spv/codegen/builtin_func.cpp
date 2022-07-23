#include "builtin_func.h"
#include "builder.h"
#include "type_caster.h"
namespace toolhub::spv {
BuiltinFunc::BuiltinFunc(Builder* builder)
	: Component(builder) {}
template<typename Ite>
	requires(vstd::IterableType<Ite>)
Id BuiltinFunc::FuncCall(Id type, vstd::string_view name, Ite&& args) {
	Id newId(bd->NewId());
	auto builder = bd->Str();
	builder << newId.ToString() << " = OpExtInst "sv << type.ToString() << ' ' << Id::BuiltinFuncId().ToString() << ' ' << name;
	for (Id i : args) {
		builder << ' ' << i.ToString();
	}
	builder << '\n';
	return newId;
}
template<typename Ite>
	requires(vstd::IterableType<Ite>)
Id BuiltinFunc::OpCall(Id type, vstd::string_view name, Ite&& args) {
	Id newId(bd->NewId());
	auto builder = bd->Str();
	builder << newId.ToString() << " = "sv << name << ' ' << type.ToString();
	for (Id i : args) {
		builder << ' ' << i.ToString();
	}
	builder << '\n';
	return newId;
}
Id BuiltinFunc::OpCall(Id type, vstd::string_view name, Id arg) {
	Id newId(bd->NewId());
	bd->Str() << newId.ToString() << " = "sv << name << ' ' << type.ToString() << ' ' << arg.ToString() << '\n';
}

Id BuiltinFunc::CallFunc(
	luisa::compute::Type const* callType,
	CallOp op,
	vstd::IRange<Variable>& arg) {
	auto getFirst = vstd::TransformRange([](Variable const& var) { return var.varId; });
	auto CastFunctor = [&](InternalType const& dstType) {
		return vstd::TransformRange([&, dstType](Variable const& var) {
			auto srcType = InternalType::GetType(var.type);
			assert(srcType);
			return TypeCaster::TryCast(bd, *srcType, dstType, var.varId);
		});
	};
	auto ReturnTypeCast = [&](InternalType& dstType, auto&& isDiffType, auto&& transformType) {
		vstd::optional<InternalType> originType;
		if (isDiffType(dstType)) {
			originType = dstType;
			transformType(dstType);
		}
		return [&, o = *originType, dstType](Id& newId) {
			newId = TypeCaster::TryCast(bd, dstType, o, newId);
		};
	};
	auto ReturnTypeCastToFloat = [&](InternalType& dstType) {
		return ReturnTypeCast(
			dstType,
			[](InternalType type) {
				return type.tag != InternalType::Tag::FLOAT &&
					   type.tag != InternalType::Tag::MATRIX;
			},
			[](InternalType& type) {
				type.tag = InternalType::Tag::FLOAT;
			});
	};
	auto FuncFUSName =
		[](InternalType const& type,
		   vstd::string_view floatName,
		   vstd::string_view intName,
		   vstd::string_view uintName) {
			switch (type.tag) {
				case InternalType::Tag::FLOAT:
				case InternalType::Tag::MATRIX:
					return floatName;
				case InternalType::Tag::INT:
					return intName;
				case InternalType::Tag::UINT:
					return uintName;
				default:
					assert(false);
			}
		};
	auto FuncFIName =
		[](InternalType const& type,
		   vstd::string_view floatName,
		   vstd::string_view intName) {
			switch (type.tag) {
				case InternalType::Tag::FLOAT:
				case InternalType::Tag::MATRIX:
					return floatName;
				case InternalType::Tag::INT:
				case InternalType::Tag::UINT:
					return intName;
				default:
					assert(false);
			}
		};
	auto atomicOp = [&](vstd::string_view op) {
		vstd::vector<Variable, VEngine_AllocType::VEngine, 3> args;
		for (auto&& a : arg) {
			args.emplace_back(a);
		}
		assert(args.size() == 3);
		Id newId(bd->NewId());
		auto uint1 = bd->GetConstId(1u);
		bd->Str()
			<< newId.ToString()
			<< " = "sv << op << ' '
			<< bd->GetTypeId(callType, PointerUsage::NotPointer).ToString() << ' '
			<< args[0].AccessBufferEle(Id::ZeroId(), args[1].varId).ToString() << ' '
			<< uint1.ToString() << ' '
			<< Id::ZeroId().ToString() << ' '
			<< args[2].varId.ToString() << '\n';
		return newId;
	};
	switch (op) {
		case CallOp::ALL: {
			return OpCall(
				Id::BoolId(),
				"OpAll"sv,
				arg | getFirst);
		}
		case CallOp::ANY: {
			return OpCall(
				Id::BoolId(),
				"OpAny"sv,
				arg | getFirst);
		}
		case CallOp::SELECT: {
			auto type = InternalType::GetType(callType);
			assert(type);
			return OpCall(
				bd->GetTypeId(*type, PointerUsage::NotPointer),
				"OpSelect"sv,
				arg | CastFunctor(*type));
		}
		case CallOp::CLAMP: {
			auto type = InternalType::GetType(callType);
			assert(type);
			return FuncCall(
				bd->GetTypeId(*type, PointerUsage::NotPointer),
				FuncFUSName(*type, "FClamp"sv, "SClamp"sv, "UClamp"sv),
				arg | CastFunctor(*type));
		}
		case CallOp::LERP: {
			auto type = InternalType::GetType(callType);
			assert(type);
			// require type transform
			auto disp = ReturnTypeCastToFloat(*type);
			auto result = FuncCall(
				bd->GetTypeId(*type, PointerUsage::NotPointer),
				"FMix"sv,
				arg | CastFunctor(*type));
			disp(result);
			return result;
		}
		case CallOp::STEP: {
			auto type = InternalType::GetType(callType);
			assert(type);
			auto disp = ReturnTypeCastToFloat(*type);
			auto result = FuncCall(
				bd->GetTypeId(*type, PointerUsage::NotPointer),
				"Step"sv,
				arg | CastFunctor(*type));
			disp(result);
			return result;
		}
		case CallOp::ABS: {
			auto type = InternalType::GetType(callType);
			assert(type && (type->tag != InternalType::Tag::UINT || type->tag != InternalType::Tag::BOOL));
			vstd::string_view name = FuncFIName(
				*type,
				"FAbs"sv,
				"SAbs"sv);
			return FuncCall(
				bd->GetTypeId(*type, PointerUsage::NotPointer),
				name,
				arg | getFirst);
		}
		case CallOp::MIN: {
			auto type = InternalType::GetType(callType);
			assert(type);
			return FuncCall(
				bd->GetTypeId(*type, PointerUsage::NotPointer),
				FuncFUSName(*type, "FMin"sv, "SMin"sv, "UMin"sv),
				arg | CastFunctor(*type));
		}
		case CallOp::MAX: {
			auto type = InternalType::GetType(callType);
			assert(type);
			return FuncCall(
				bd->GetTypeId(*type, PointerUsage::NotPointer),
				FuncFUSName(*type, "FMax"sv, "SMax"sv, "UMax"sv),
				arg | CastFunctor(*type));
		}
		case CallOp::CLZ: {
			auto type = InternalType::GetType(callType);
			assert(type && (type->tag == InternalType::Tag::INT || type->tag == InternalType::Tag::UINT));
			return FuncCall(
				bd->GetTypeId(*type, PointerUsage::NotPointer),
				(type->tag == InternalType::Tag::INT) ? "FindSMsb"sv : "FindUMsb"sv,
				arg | getFirst);
		}
		case CallOp::CTZ: {
			auto type = InternalType::GetType(callType);
			assert(type && (type->tag == InternalType::Tag::INT || type->tag == InternalType::Tag::UINT));
			return FuncCall(
				bd->GetTypeId(*type, PointerUsage::NotPointer),
				"FindILsb"sv,
				arg | getFirst);
		}
		case CallOp::POPCOUNT: {
			auto type = InternalType::GetType(callType);
			assert(type && (type->tag == InternalType::Tag::INT || type->tag == InternalType::Tag::UINT));
			return OpCall(
				bd->GetTypeId(*type, PointerUsage::NotPointer),
				"OpBitCount"sv,
				arg | getFirst);
		}
		case CallOp::REVERSE: {
			auto type = InternalType::GetType(callType);
			assert(type && (type->tag == InternalType::Tag::INT || type->tag == InternalType::Tag::UINT));
			return OpCall(
				bd->GetTypeId(*type, PointerUsage::NotPointer),
				"OpBitReverse"sv,
				arg | getFirst);
		}
		case CallOp::ISINF: {
			return OpCall(
				bd->GetTypeId(callType, PointerUsage::NotPointer),
				"OpIsInf"sv,
				arg | getFirst);
		}
		case CallOp::ISNAN: {
			return OpCall(
				bd->GetTypeId(callType, PointerUsage::NotPointer),
				"OpIsNan"sv,
				arg | getFirst);
		}
		case CallOp::ACOS: {
			return FuncCall(
				bd->GetTypeId(callType, PointerUsage::NotPointer),
				"Acos"sv,
				arg | getFirst);
		}
		case CallOp::ACOSH: {
			return FuncCall(
				bd->GetTypeId(callType, PointerUsage::NotPointer),
				"Acosh"sv,
				arg | getFirst);
		}
		case CallOp::ASIN: {
			return FuncCall(
				bd->GetTypeId(callType, PointerUsage::NotPointer),
				"Asin"sv,
				arg | getFirst);
		}
		case CallOp::ASINH: {
			return FuncCall(
				bd->GetTypeId(callType, PointerUsage::NotPointer),
				"Asinh"sv,
				arg | getFirst);
		}
		case CallOp::ATAN: {
			return FuncCall(
				bd->GetTypeId(callType, PointerUsage::NotPointer),
				"Atan"sv,
				arg | getFirst);
		}
		case CallOp::ATAN2: {
			return FuncCall(
				bd->GetTypeId(callType, PointerUsage::NotPointer),
				"Atan2"sv,
				arg | getFirst);
		}
		case CallOp::ATANH: {
			return FuncCall(
				bd->GetTypeId(callType, PointerUsage::NotPointer),
				"Atanh"sv,
				arg | getFirst);
		}
		case CallOp::COS: {
			return FuncCall(
				bd->GetTypeId(callType, PointerUsage::NotPointer),
				"Cos"sv,
				arg | getFirst);
		}
		case CallOp::COSH: {
			return FuncCall(
				bd->GetTypeId(callType, PointerUsage::NotPointer),
				"Cosh"sv,
				arg | getFirst);
		}
		case CallOp::SIN: {
			return FuncCall(
				bd->GetTypeId(callType, PointerUsage::NotPointer),
				"Sin"sv,
				arg | getFirst);
		}
		case CallOp::SINH: {
			return FuncCall(
				bd->GetTypeId(callType, PointerUsage::NotPointer),
				"Sinh"sv,
				arg | getFirst);
		}
		case CallOp::TAN: {
			return FuncCall(
				bd->GetTypeId(callType, PointerUsage::NotPointer),
				"Tan"sv,
				arg | getFirst);
		}
		case CallOp::TANH: {
			return FuncCall(
				bd->GetTypeId(callType, PointerUsage::NotPointer),
				"Tanh"sv,
				arg | getFirst);
		}
		case CallOp::EXP: {
			return FuncCall(
				bd->GetTypeId(callType, PointerUsage::NotPointer),
				"Exp"sv,
				arg | getFirst);
		}
		case CallOp::EXP2: {
			return FuncCall(
				bd->GetTypeId(callType, PointerUsage::NotPointer),
				"Exp2"sv,
				arg | getFirst);
		}
		case CallOp::EXP10: {
			auto type = InternalType::GetType(callType);
			assert(type);
			auto float10 = bd->GetConstId(type->LiteralValue(10.0f));
			vstd::string name;
			name << "Pow "sv << float10.ToString();
			return FuncCall(
				bd->GetTypeId(*type, PointerUsage::NotPointer),
				name,
				arg | getFirst);
		}
		case CallOp::LOG: {
			return FuncCall(
				bd->GetTypeId(callType, PointerUsage::NotPointer),
				"Log"sv,
				arg | getFirst);
		}
		case CallOp::LOG2: {
			return FuncCall(
				bd->GetTypeId(callType, PointerUsage::NotPointer),
				"Log2"sv,
				arg | getFirst);
		}
		case CallOp::LOG10: {
			auto typeId = bd->GetTypeId(callType, PointerUsage::NotPointer);
			auto log2Value = FuncCall(
				typeId,
				"Log2"sv,
				arg | getFirst);
			Id newId(bd->NewId());
			bd->Str() << newId.ToString() << " = OpFMul "sv << typeId.ToString() << ' ' << log2Value.ToString() << ' ' << bd->GetConstId(0.30103001f).ToString() << '\n';
			return newId;
		}
		case CallOp::POW: {
			return FuncCall(
				bd->GetTypeId(callType, PointerUsage::NotPointer),
				"Pow"sv,
				arg | getFirst);
		}
		case CallOp::SQRT: {
			return FuncCall(
				bd->GetTypeId(callType, PointerUsage::NotPointer),
				"Sqrt"sv,
				arg | getFirst);
		}
		case CallOp::RSQRT: {
			return FuncCall(
				bd->GetTypeId(callType, PointerUsage::NotPointer),
				"InverseSqrt"sv,
				arg | getFirst);
		}
		case CallOp::CEIL: {
			return FuncCall(
				bd->GetTypeId(callType, PointerUsage::NotPointer),
				"Ceil"sv,
				arg | getFirst);
		}
		case CallOp::FLOOR: {
			return FuncCall(
				bd->GetTypeId(callType, PointerUsage::NotPointer),
				"Floor"sv,
				arg | getFirst);
		}
		case CallOp::FRACT: {
			return FuncCall(
				bd->GetTypeId(callType, PointerUsage::NotPointer),
				"Fract"sv,
				arg | getFirst);
		}
		case CallOp::TRUNC: {
			return FuncCall(
				bd->GetTypeId(callType, PointerUsage::NotPointer),
				"Trunc"sv,
				arg | getFirst);
		}
		case CallOp::ROUND: {
			return FuncCall(
				bd->GetTypeId(callType, PointerUsage::NotPointer),
				"Round"sv,
				arg | getFirst);
		}
		case CallOp::FMA: {
			auto type = InternalType::GetType(callType);
			assert(type);
			return FuncCall(
				bd->GetTypeId(*type, PointerUsage::NotPointer),
				"Fma"sv,
				arg | CastFunctor(*type));
		}
		case CallOp::COPYSIGN: {
			auto type = InternalType::GetType(callType);
			assert(type && type->tag == InternalType::Tag::FLOAT);
			auto uintType = *type;
			uintType.tag = InternalType::Tag::UINT;
			auto uintTypeId = bd->GetTypeId(uintType, PointerUsage::NotPointer);
			auto floatTypeId = bd->GetTypeId(*type, PointerUsage::NotPointer);
			Id args[2];
			{
				size_t idx = 0;
				for (auto&& i : arg) {
					assert(idx < 2);
					auto srcType = InternalType::GetType(i.type);
					assert(srcType && srcType->tag == InternalType::Tag::FLOAT);
					args[idx] = OpCall(
						uintTypeId,
						"OpBitcast"sv,
						i.varId);
					++idx;
				}
			}
			auto aConst = bd->GetConstId(0x7fffffffu);
			auto bConst = bd->GetConstId(0x80000000u);
			auto aIds = {aConst, args[0]};
			auto aAnd = OpCall(
				uintTypeId,
				"OpBitwiseAnd"sv,
				vstd::CacheEndRange(aIds));
			auto bIds = {bConst, args[1]};
			auto bAnd = OpCall(
				uintTypeId,
				"OpBitwiseAnd"sv,
				vstd::CacheEndRange(bIds));
			auto abIds = {aAnd, bAnd};
			auto abOr = OpCall(
				uintTypeId,
				"OpBitwiseOr"sv,
				vstd::CacheEndRange(abIds));
			return OpCall(
				floatTypeId,
				"OpBitcast"sv,
				abOr);
			//	TODO
			//asfloat((asuint(a) & 0x7fffffffu) | (asuint(b) & 0x80000000u))
		}
		case CallOp::CROSS: {
			return FuncCall(
				bd->GetTypeId(callType, PointerUsage::NotPointer),
				"Cross"sv,
				arg | getFirst);
		}
		case CallOp::DOT: {
			return OpCall(
				bd->GetTypeId(callType, PointerUsage::NotPointer),
				"OpDot"sv,
				arg | getFirst);
		}
		case CallOp::LENGTH: {
			return FuncCall(
				bd->GetTypeId(callType, PointerUsage::NotPointer),
				"Length"sv,
				arg | getFirst);
		}
		case CallOp::LENGTH_SQUARED: {
			Id argId;
			for (auto&& i : arg) {
				argId = i.varId;
				break;
			}
			assert(argId.valid());
			auto argIds = {argId, argId};
			auto typeId = bd->GetTypeId(callType, PointerUsage::NotPointer);
			return OpCall(
				typeId,
				"OpDot"sv,
				vstd::CacheEndRange(argIds));
		}
		case CallOp::NORMALIZE: {
			return FuncCall(
				bd->GetTypeId(callType, PointerUsage::NotPointer),
				"Normalize"sv,
				arg | getFirst);
		}
		case CallOp::FACEFORWARD: {
			return FuncCall(
				bd->GetTypeId(callType, PointerUsage::NotPointer),
				"FaceForward"sv,
				arg | getFirst);
		}
		case CallOp::DETERMINANT: {
			return FuncCall(
				bd->GetTypeId(callType, PointerUsage::NotPointer),
				"Determinant"sv,
				arg | getFirst);
		}
		case CallOp::TRANSPOSE: {
			return OpCall(
				bd->GetTypeId(callType, PointerUsage::NotPointer),
				"OpTranspose"sv,
				arg | getFirst);
		}
		case CallOp::INVERSE: {
			return FuncCall(
				bd->GetTypeId(callType, PointerUsage::NotPointer),
				"MatrixInverse"sv,
				arg | getFirst);
		}
		case CallOp::SYNCHRONIZE_BLOCK: {
			auto maps = {2u, 2u, 264u};
			auto ite =
				vstd::CacheEndRange(maps) |
				vstd::TransformRange([&](uint v) {
					return bd->GetConstId(v);
				});
			auto builder = bd->Str();
			builder << "OpControlBarrier"sv;
			for (auto&& i : ite) {
				builder << ' ' << i.ToString();
			}
			builder << '\n';
			return Id();
		}
		case CallOp::ATOMIC_EXCHANGE: {
			return atomicOp("OpAtomicExchange"sv);
		}
		case CallOp::ATOMIC_COMPARE_EXCHANGE: {
			vstd::vector<Variable, VEngine_AllocType::VEngine, 4> args;
			for (auto&& a : arg) {
				args.emplace_back(a);
			}
			assert(args.size() == 4);
			Id newId(bd->NewId());
			auto uint1 = bd->GetConstId(1u);
			bd->Str()
				<< newId.ToString()
				<< " = OpAtomicCompareExchange "sv
				<< bd->GetTypeId(callType, PointerUsage::NotPointer).ToString() << ' '
				<< args[0].AccessBufferEle(Id::ZeroId(), args[1].varId).ToString() << ' '
				<< uint1.ToString() << ' '
				<< Id::ZeroId().ToString() << ' '
				<< Id::ZeroId().ToString() << ' '
				<< args[3].varId.ToString() << ' '
				<< args[2].varId.ToString() << '\n';
			return newId;
		}
		case CallOp::ATOMIC_FETCH_ADD: {
			return atomicOp([&] {
				switch (callType->tag()) {
					case Type::Tag::FLOAT:
						return "OpAtomicFAddEXT"sv;
					case Type::Tag::INT:
					case Type::Tag::UINT:
						return "OpAtomicIAdd"sv;
					default: assert(false);
				}
			}());
		}
		case CallOp::ATOMIC_FETCH_SUB: {
			return atomicOp([&] {
				switch (callType->tag()) {
					case Type::Tag::FLOAT:
						return "OpAtomicFSubEXT"sv;
					case Type::Tag::INT:
					case Type::Tag::UINT:
						return "OpAtomicISub"sv;
					default: assert(false);
				}
			}());
		}
		case CallOp::ATOMIC_FETCH_AND: {
			return atomicOp("OpAtomicAnd"sv);
		}
		case CallOp::ATOMIC_FETCH_OR: {
			return atomicOp("OpAtomicOr"sv);
		}
		case CallOp::ATOMIC_FETCH_XOR: {
			return atomicOp("OpAtomicXor"sv);
		}
		case CallOp::ATOMIC_FETCH_MIN: {
			return atomicOp([&] {
				switch (callType->tag()) {
					case Type::Tag::FLOAT:
						return "OpAtomicFMinEXT"sv;
					case Type::Tag::INT:
						return "OpAtomicSMin"sv;
					case Type::Tag::UINT:
						return "OpAtomicUMin"sv;
					default: assert(false);
				}
			}());
		}
		case CallOp::ATOMIC_FETCH_MAX: {
			return atomicOp([&] {
				switch (callType->tag()) {
					case Type::Tag::FLOAT:
						return "OpAtomicFMaxEXT"sv;
					case Type::Tag::INT:
						return "OpAtomicSMax"sv;
					case Type::Tag::UINT:
						return "OpAtomicUMax"sv;
					default: assert(false);
				}
			}());
		}
		case CallOp::BUFFER_READ: {
			// buffer, buf_idx, idx
			vstd::vector<Variable, VEngine_AllocType::VEngine, 2> args;
			for (auto&& a : arg) {
				args.emplace_back(a);
			}
			assert(args.size() == 2);
			return args[0].ReadBufferEle(
				Id::ZeroId(),
				args[1].varId);
		}
		case CallOp::BUFFER_WRITE: {
			// buffer, buf_idx, idx
			vstd::vector<Variable, VEngine_AllocType::VEngine, 3> args;
			for (auto&& a : arg) {
				args.emplace_back(a);
			}
			assert(args.size() == 3);
			args[0].WriteBufferEle(
				Id::ZeroId(),
				args[1].varId,
				args[2].varId);
			return Id();
		}
	}
}
}// namespace toolhub::spv