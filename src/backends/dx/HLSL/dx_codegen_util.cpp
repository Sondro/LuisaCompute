#include "dx_codegen.h"
#include "vstl/StringUtility.h"
#include "vstl/variant_util.h"
#include "ast/constant_data.h"
#include "struct_generator.h"
#include "codegen_stack_data.h"
#include "shader_header.h"
#include <vstl/pdqsort.h>
namespace toolhub::directx {
namespace detail {
static inline uint64 CalcAlign(uint64 value, uint64 align) {
    return (value + (align - 1)) & ~(align - 1);
}
static vstd::string const &HLSLHeader(vstd::string_view internalDataPath) {
    static vstd::string header = ReadInternalHLSLFile("hlsl_header", internalDataPath);
    return header;
}
static vstd::string const &HLSLHeaderSpirv(vstd::string_view internalDataPath) {
    static vstd::string header = ReadInternalHLSLFile("hlsl_header_spirv", internalDataPath);
    return header;
}
static vstd::string const &RayTracingHeader(vstd::string_view internalDataPath) {
    static vstd::string rtHeader = ReadInternalHLSLFile("raytracing_header", internalDataPath);
    return rtHeader;
}
}// namespace detail
static thread_local vstd::unique_ptr<CodegenStackData> opt;
#ifdef USE_SPIRV
CodegenStackData *CodegenUtility::StackData() { return opt.get(); }
#endif
uint CodegenUtility::IsBool(Type const &type) {
    if (type.tag() == Type::Tag::BOOL) {
        return 1;
    } else if (type.tag() == Type::Tag::VECTOR && type.element()->tag() == Type::Tag::BOOL) {
        return type.dimension();
    }
    return 0;
};
vstd::string CodegenUtility::GetNewTempVarName() {
    vstd::string name = "tmp";
    vstd::to_string(opt->tempCount, name);
    opt->tempCount++;
    return name;
}
StructGenerator const *CodegenUtility::GetStruct(
    Type const *type) {
    return opt->CreateStruct(type);
}
void CodegenUtility::RegistStructType(Type const *type) {
    if (type->is_structure() || type->is_array())
        opt->structTypes.Emplace(type, opt->count++);
    else if (type->is_buffer()) {
        RegistStructType(type->element());
    }
}

static bool IsVarWritable(Function func, Variable i) {
    return ((uint)func.variable_usage(i.uid()) & (uint)Usage::WRITE) != 0;
}
void CodegenUtility::GetVariableName(Variable::Tag type, uint id, vstd::string &str) {
    switch (type) {
        case Variable::Tag::BLOCK_ID:
            str << "thdId"sv;
            break;
        case Variable::Tag::DISPATCH_ID:
            str << "dspId"sv;
            break;
        case Variable::Tag::THREAD_ID:
            str << "grpId"sv;
            break;
        case Variable::Tag::DISPATCH_SIZE:
            str << "dsp_c[0]"sv;
            break;
        case Variable::Tag::OBJECT_ID:
            str << "obj_id"sv;
            break;
        case Variable::Tag::LOCAL:
            switch (opt->funcType) {
                case CodegenStackData::FuncType::Kernel:
                case CodegenStackData::FuncType::Vert:
                    if (opt->arguments.Find(id)) {
                        str << "a.l"sv;
                    } else {
                        str << 'l';
                    }
                    vstd::to_string(id, str);
                    break;
                case CodegenStackData::FuncType::Pixel: {
                    auto ite = opt->arguments.Find(id);
                    if (!ite) {
                        str << 'l';
                        vstd::to_string(id, str);
                    } else {
                        if (ite.Value() == 0) {
                            if (opt->pixelFirstArgIsStruct) {
                                str << 'p';
                            } else {
                                str << "p.v0"sv;
                            }
                        } else {
                            str << "a.l"sv;
                            vstd::to_string(id, str);
                        }
                    }
                } break;
                default: {
                    str << 'l';
                    vstd::to_string(id, str);
                }
            }
            break;
        case Variable::Tag::SHARED:
            str << 's';
            vstd::to_string(id, str);
            break;
        case Variable::Tag::REFERENCE:
            str << 'r';
            vstd::to_string(id, str);
            break;
        case Variable::Tag::BUFFER:
            str << 'b';
            vstd::to_string(id, str);
            break;
        case Variable::Tag::TEXTURE:
            str << 't';
            vstd::to_string(id, str);
            break;
        case Variable::Tag::BINDLESS_ARRAY:
            str << "ba"sv;
            vstd::to_string(id, str);
            break;
        case Variable::Tag::ACCEL:
            str << "ac"sv;
            vstd::to_string(id, str);
            break;
        default:
            str << 'v';
            vstd::to_string(id, str);
            break;
    }
}

void CodegenUtility::GetVariableName(Variable const &type, vstd::string &str) {
    GetVariableName(type.tag(), type.uid(), str);
}
bool CodegenUtility::GetConstName(uint64 hash, ConstantData const &data, vstd::string &str) {
    auto constCount = opt->GetConstCount(hash);
    str << "c";
    vstd::to_string((constCount.first), str);
    return constCount.second;
}
void CodegenUtility::GetConstantStruct(ConstantData const &data, vstd::string &str) {
    uint64 constCount = opt->GetConstCount(data.hash()).first;
    //auto typeName = CodegenUtility::GetBasicTypeName(view.index());
    str << "struct tc";
    vstd::to_string((constCount), str);
    uint64 varCount = 1;
    eastl::visit(
        [&](auto &&arr) {
            varCount = arr.size();
        },
        data.view());
    str << "{\n";
    str << CodegenUtility::GetBasicTypeName(data.view().index()) << " v[";
    vstd::to_string((varCount), str);
    str << "];\n";
    str << "};\n";
}
void CodegenUtility::GetConstantData(ConstantData const &data, vstd::string &str) {
    auto &&view = data.view();
    uint64 constCount = opt->GetConstCount(data.hash()).first;

    vstd::string name = vstd::to_string((constCount));
    str << "uniform const tc" << name << " c" << name;
    str << "={{";
    eastl::visit(
        [&](auto &&arr) {
            for (auto const &ele : arr) {
                PrintValue<std::remove_cvref_t<typename std::remove_cvref_t<decltype(arr)>::element_type>> prt;
                prt(ele, str);
                str << ',';
            }
        },
        view);
    auto last = str.end() - 1;
    if (*last == ',')
        *last = '}';
    else
        str << '}';
    str << "};\n";
}

void CodegenUtility::GetTypeName(Type const &type, vstd::string &str, Usage usage) {
    switch (type.tag()) {
        case Type::Tag::BOOL:
            str << "bool"sv;
            return;
        case Type::Tag::FLOAT:
            str << "float"sv;
            return;
        case Type::Tag::INT:
            str << "int"sv;
            return;
        case Type::Tag::UINT:
            str << "uint"sv;
            return;
        case Type::Tag::MATRIX: {
            CodegenUtility::GetTypeName(*type.element(), str, usage);
            vstd::to_string(type.dimension(), str);
            str << 'x';
            vstd::to_string((type.dimension() == 3) ? 4 : type.dimension(), str);
        }
            return;
        case Type::Tag::VECTOR: {
            CodegenUtility::GetTypeName(*type.element(), str, usage);
            vstd::to_string((type.dimension()), str);
        }
            return;
        case Type::Tag::ARRAY: {
            if (type.element()->tag() == Type::Tag::FLOAT && type.dimension() == 3) {
                str << "FLOATV3";
            } else {
                auto customType = opt->CreateStruct(&type);
                str << customType->GetStructName();
            }
        }
            return;
        case Type::Tag::STRUCTURE: {
            auto customType = opt->CreateStruct(&type);
            str << customType->GetStructName();
        }
            return;
        case Type::Tag::BUFFER: {

            if ((static_cast<uint>(usage) & static_cast<uint>(Usage::WRITE)) != 0)
                str << "RW"sv;
            str << "StructuredBuffer<"sv;
            auto ele = type.element();
            if (ele->is_matrix()) {
                auto n = ele->dimension();
                str << luisa::format("WrappedFloat{}x{}", n, n);
            } else {
                vstd::string typeName;
                if (ele->is_vector() && ele->dimension() == 3) {
                    typeName << "float4"sv;
                } else {
                    if (opt->kernel.requires_atomic_float() && ele->tag() == Type::Tag::FLOAT) {
                        typeName << "int";
                    } else {
                        GetTypeName(*ele, typeName, usage);
                    }
                }
                auto ite = opt->structReplaceName.Find(typeName);
                if (ite) {
                    str << ite.Value();
                } else {
                    str << typeName;
                }
            }
            str << '>';
        } break;
        case Type::Tag::TEXTURE: {
            if ((static_cast<uint>(usage) & static_cast<uint>(Usage::WRITE)) != 0)
                str << "RW"sv;
            str << "Texture"sv;
            vstd::to_string((type.dimension()), str);
            str << "D<"sv;
            GetTypeName(*type.element(), str, usage);
            if (type.tag() != Type::Tag::VECTOR) {
                str << '4';
            }
            str << '>';
            break;
        }
        case Type::Tag::BINDLESS_ARRAY: {
            str << "BINDLESS_ARRAY"sv;
        } break;
        case Type::Tag::ACCEL: {
            str << "RaytracingAccelerationStructure"sv;
        } break;
        default:
            LUISA_ERROR_WITH_LOCATION("Bad.");
            break;
    }
}

void CodegenUtility::GetFunctionDecl(Function func, vstd::string &funcDecl) {
    vstd::string data;
    uint64 tempIdx = 0;
    auto GetTemplateName = [&] {
        data << 'T';
        vstd::to_string(tempIdx, data);
        tempIdx++;
    };
    auto GetTypeName = [&](Type const *t, Usage usage) {
        if (t->is_texture() || t->is_buffer())
            GetTemplateName();
        else
            CodegenUtility::GetTypeName(*t, data, usage);
    };
    if (func.return_type()) {
        //TODO: return type
        CodegenUtility::GetTypeName(*func.return_type(), data, Usage::READ);
    } else {
        data += "void"sv;
    }
    {
        data += " custom_"sv;
        vstd::to_string((opt->GetFuncCount(func.hash())), data);
        if (func.arguments().empty()) {
            data += "()"sv;
        } else {
            data += '(';
            for (auto &&i : func.arguments()) {
                if (i.tag() == Variable::Tag::REFERENCE) {
                    if (auto usage = func.variable_usage(i.uid());
                        usage == Usage::WRITE) {
                        data += "out ";
                    } else if (usage == Usage::READ_WRITE) {
                        data += "inout ";
                    } else {
                        data += "in ";
                    }
                }
                RegistStructType(i.type());
                Usage usage = func.variable_usage(i.uid());

                vstd::string varName;
                CodegenUtility::GetVariableName(i, varName);
                if (i.type()->is_accel()) {
                    if (usage == Usage::READ) {
                        CodegenUtility::GetTypeName(*i.type(), data, usage);
                        data << ' ' << varName << ',';
                    }
                    GetTemplateName();
                    data << ' ' << varName << "Inst,"sv;
                } else {
                    GetTypeName(i.type(), usage);
                    data << ' ';
                    data << varName << ',';
                }
            }
            data[data.size() - 1] = ')';
        }
    }
    if (tempIdx > 0) {
        funcDecl << "template<"sv;
        for (auto i : vstd::range(tempIdx)) {
            funcDecl << "typename T"sv;
            vstd::to_string(i, funcDecl);
            funcDecl << ',';
        }
        *(funcDecl.end() - 1) = '>';
    }
    funcDecl << '\n'
             << data;
}
void CodegenUtility::GetFunctionName(Function callable, vstd::string &result) {
    result << "custom_"sv << vstd::to_string((opt->GetFuncCount(callable.hash())));
}
void CodegenUtility::GetFunctionName(CallExpr const *expr, vstd::string &str, StringStateVisitor &vis) {
    auto args = expr->arguments();
    auto getPointer = [&]() {
        str << '(';
        uint64 sz = 1;
        if (args.size() >= 1) {
            auto firstArg = static_cast<AccessExpr const *>(args[0]);
            firstArg->range()->accept(vis);
            str << ',';
            firstArg->index()->accept(vis);
            str << ',';
        }
        for (auto i : vstd::range(1, args.size())) {
            ++sz;
            args[i]->accept(vis);
            if (sz != args.size()) {
                str << ',';
            }
        }
        str << ')';
    };
    auto IsNumVec3 = [&](Type const &t) {
        if (t.tag() != Type::Tag::VECTOR || t.dimension() != 3) return false;
        auto &&ele = *t.element();
        switch (ele.tag()) {
            case Type::Tag::INT:
            case Type::Tag::UINT:
            case Type::Tag::FLOAT:
                return true;
            default:
                return false;
        }
    };
    auto PrintArgs = [&] {
        uint64 sz = 0;
        for (auto &&i : args) {
            ++sz;
            i->accept(vis);
            if (sz != args.size()) {
                str << ',';
            }
        }
    };
    switch (expr->op()) {
        case CallOp::CUSTOM:
            str << "custom_"sv << vstd::to_string((opt->GetFuncCount(expr->custom().hash())));
            str << '(';
            {
                uint64 sz = 0;
                for (auto &&i : args) {
                    if (i->type()->is_accel()) {
                        if ((static_cast<uint>(expr->custom().variable_usage(expr->custom().arguments()[sz].uid())) & static_cast<uint>(Usage::WRITE)) == 0) {
                            i->accept(vis);
                            str << ',';
                        }
                        i->accept(vis);
                        str << "Inst"sv;
                    } else {
                        i->accept(vis);
                    }
                    ++sz;
                    if (sz != args.size()) {
                        str << ',';
                    }
                }
            }
            str << ')';
            return;

        case CallOp::ALL:
            str << "all"sv;
            break;
        case CallOp::ANY:
            str << "any"sv;
            break;
        case CallOp::SELECT:
            str << "_select"sv;
            break;
        case CallOp::CLAMP:
            str << "clamp"sv;
            break;
        case CallOp::LERP:
            str << "lerp"sv;
            break;
        case CallOp::STEP:
            str << "step"sv;
            break;
        case CallOp::ABS:
            str << "abs"sv;
            break;
        case CallOp::MAX:
            str << "max"sv;
            break;
        case CallOp::MIN:
            str << "min"sv;
            break;
        case CallOp::POW:
            str << "pow"sv;
            break;
        case CallOp::CLZ:
            str << "firstbithigh"sv;
            break;
        case CallOp::CTZ:
            str << "firstbitlow"sv;
            break;
        case CallOp::POPCOUNT:
            str << "popcount"sv;
            break;
        case CallOp::REVERSE:
            str << "reversebits"sv;
            break;
        case CallOp::ISINF:
            str << "_isinf"sv;
            break;
        case CallOp::ISNAN:
            str << "_isnan"sv;
            break;
        case CallOp::ACOS:
            str << "acos"sv;
            break;
        case CallOp::ACOSH:
            str << "_acosh"sv;
            break;
        case CallOp::ASIN:
            str << "asin"sv;
            break;
        case CallOp::ASINH:
            str << "_asinh"sv;
            break;
        case CallOp::ATAN:
            str << "atan"sv;
            break;
        case CallOp::ATAN2:
            str << "atan2"sv;
            break;
        case CallOp::ATANH:
            str << "_atanh"sv;
            break;
        case CallOp::COS:
            str << "cos"sv;
            break;
        case CallOp::COSH:
            str << "cosh"sv;
            break;
        case CallOp::SIN:
            str << "sin"sv;
            break;
        case CallOp::SINH:
            str << "sinh"sv;
            break;
        case CallOp::TAN:
            str << "tan"sv;
            break;
        case CallOp::TANH:
            str << "tanh"sv;
            break;
        case CallOp::EXP:
            str << "exp"sv;
            break;
        case CallOp::EXP2:
            str << "exp2"sv;
            break;
        case CallOp::EXP10:
            str << "_exp10"sv;
            break;
        case CallOp::LOG:
            str << "log"sv;
            break;
        case CallOp::LOG2:
            str << "log2"sv;
            break;
        case CallOp::LOG10:
            str << "log10"sv;
            break;
        case CallOp::SQRT:
            str << "sqrt"sv;
            break;
        case CallOp::RSQRT:
            str << "rsqrt"sv;
            break;
        case CallOp::CEIL:
            str << "ceil"sv;
            break;
        case CallOp::FLOOR:
            str << "floor"sv;
            break;
        case CallOp::FRACT:
            str << "fract"sv;
            break;
        case CallOp::TRUNC:
            str << "trunc"sv;
            break;
        case CallOp::ROUND:
            str << "round"sv;
            break;
        case CallOp::FMA:
            str << "fma"sv;
            break;
        case CallOp::COPYSIGN:
            str << "copysign"sv;
            break;
        case CallOp::CROSS:
            str << "cross"sv;
            break;
        case CallOp::DOT:
            str << "dot"sv;
            break;
        case CallOp::LENGTH:
            str << "length"sv;
            break;
        case CallOp::LENGTH_SQUARED:
            str << "_length_sqr"sv;
            break;
        case CallOp::NORMALIZE:
            str << "normalize"sv;
            break;
        case CallOp::FACEFORWARD:
            str << "faceforward"sv;
            break;
        case CallOp::DETERMINANT:
            str << "determinant"sv;
            break;
        case CallOp::TRANSPOSE:
            str << "my_transpose"sv;
            break;
        case CallOp::INVERSE:
            str << "inverse"sv;
            break;
        case CallOp::ATOMIC_EXCHANGE: {
            if (expr->type()->tag() == Type::Tag::FLOAT) {
                str << "_atomic_exchange_float"sv;
            } else {
                str << "_atomic_exchange"sv;
            }
            getPointer();
            return;
        }
        case CallOp::ATOMIC_COMPARE_EXCHANGE: {
            if (expr->type()->tag() == Type::Tag::FLOAT) {
                str << "_atomic_compare_exchange_float"sv;
            } else {
                str << "_atomic_compare_exchange"sv;
            }
            getPointer();
            return;
        }
        case CallOp::ATOMIC_FETCH_ADD: {
            if (expr->type()->tag() == Type::Tag::FLOAT)
                str << "_atomic_add_float"sv;
            else
                str << "_atomic_add"sv;
            getPointer();
            return;
        }
        case CallOp::ATOMIC_FETCH_SUB: {
            if (expr->type()->tag() == Type::Tag::FLOAT)
                str << "_atomic_sub_float"sv;
            else
                str << "_atomic_sub"sv;
            getPointer();
            return;
        }
        case CallOp::ATOMIC_FETCH_AND: {
            str << "_atomic_and"sv;
            getPointer();
            return;
        }
        case CallOp::ATOMIC_FETCH_OR: {
            str << "_atomic_or"sv;
            getPointer();
            return;
        }
        case CallOp::ATOMIC_FETCH_XOR: {
            str << "_atomic_xor"sv;
            getPointer();
            return;
        }
        case CallOp::ATOMIC_FETCH_MIN: {
            if (expr->type()->tag() == Type::Tag::FLOAT)
                str << "_atomic_min_float"sv;
            else
                str << "_atomic_min"sv;
            getPointer();
            return;
        }
        case CallOp::ATOMIC_FETCH_MAX: {
            if (expr->type()->tag() == Type::Tag::FLOAT)
                str << "_atomic_max_float"sv;
            else
                str << "_atomic_max"sv;
            getPointer();
            return;
        }
        case CallOp::TEXTURE_READ:
            str << "Smptx";
            break;
        case CallOp::TEXTURE_WRITE:
            str << "Writetx";
            break;
        case CallOp::MAKE_BOOL2:
        case CallOp::MAKE_BOOL3:
        case CallOp::MAKE_BOOL4:
        case CallOp::MAKE_UINT2:
        case CallOp::MAKE_UINT3:
        case CallOp::MAKE_UINT4:
        case CallOp::MAKE_INT2:
        case CallOp::MAKE_INT3:
        case CallOp::MAKE_INT4:
        case CallOp::MAKE_FLOAT2:
        case CallOp::MAKE_FLOAT3:
        case CallOp::MAKE_FLOAT4:
        case CallOp::MAKE_FLOAT2X2:
        case CallOp::MAKE_FLOAT4X4: {
            if (args.size() == 1 && (args[0]->type() == expr->type())) {
                args[0]->accept(vis);
            } else {
                uint tarDim = [&]() -> uint {
                    switch (expr->type()->tag()) {
                        case Type::Tag::VECTOR:
                            return expr->type()->dimension();
                        case Type::Tag::MATRIX:
                            return expr->type()->dimension() * expr->type()->dimension();
                        default:
                            return 1;
                    }
                }();
                auto is_make_matrix = expr->type()->is_matrix();
                auto n = luisa::format("{}", expr->type()->dimension());
                if (args.size() == 1 && args[0]->type()->is_scalar()) {
                    str << "(("sv;
                    if (is_make_matrix) {
                        str << "make_float" << n << "x" << n;
                    } else {
                        GetTypeName(*expr->type(), str, Usage::READ);
                    }
                    str << ")("sv;
                    for (auto &&i : args) {
                        i->accept(vis);
                        str << ',';
                    }
                    *(str.end() - 1) = ')';
                    str << ')';
                } else {
                    if (is_make_matrix) {
                        str << "make_float" << n << "x" << n;
                    } else {
                        GetTypeName(*expr->type(), str, Usage::READ);
                    }
                    str << '(';
                    uint count = 0;
                    for (auto &&i : args) {
                        i->accept(vis);
                        if (i->type()->is_vector()) {
                            auto dim = i->type()->dimension();
                            auto ele = i->type()->element();
                            auto leftEle = tarDim - count;
                            //More lefted
                            if (dim <= leftEle) {
                            } else {
                                auto swizzle = "xyzw";
                                str << '.' << vstd::string_view(swizzle, leftEle);
                            }
                            count += dim;
                        } else if (i->type()->is_scalar()) {
                            count++;
                        }
                        str << ',';
                        if (count >= tarDim) break;
                    }
                    if (count < tarDim) {
                        for (auto i : vstd::range(tarDim - count)) {
                            str << "0,"sv;
                        }
                    }
                    *(str.end() - 1) = ')';
                }
            }
            return;
        }
        case CallOp::MAKE_FLOAT3X3: {
            if (args.size() == 1 && (args[0]->type() == expr->type())) {
                args[0]->accept(vis);
                return;
            } else {
                str << "make_float3x3";
            }
        } break;
        case CallOp::BUFFER_READ: {
            if (opt->kernel.requires_atomic_float() && expr->type()->tag() == Type::Tag::FLOAT) {
                str << "bfread_float"sv;
            } else {
                str << "bfread"sv;
            }
            auto elem = args[0]->type()->element();
            if (IsNumVec3(*elem)) {
                str << "Vec3"sv;
            } else if (elem->is_matrix()) {
                str << "Mat";
            }
        } break;
        case CallOp::BUFFER_WRITE: {
            if (opt->kernel.requires_atomic_float()) {
                str << "bfwrite_float"sv;
            } else {
                str << "bfwrite"sv;
            }
            auto elem = args[0]->type()->element();
            if (IsNumVec3(*elem)) {
                str << "Vec3"sv;
            } else if (elem->is_matrix()) {
                str << "Mat";
            }
        } break;
        case CallOp::TRACE_CLOSEST:
            str << "TraceClosest"sv;
            break;
        case CallOp::TRACE_ANY:
            str << "TraceAny"sv;
            break;
        case CallOp::BINDLESS_BUFFER_READ: {
            if (opt->kernel.requires_atomic_float() && expr->type()->tag() == Type::Tag::FLOAT) {
                str << "READ_BUFFER_FLOAT"sv;
            } else {
                str << "READ_BUFFER"sv;
            }
            if (IsNumVec3(*expr->type())) {
                str << "Vec3"sv;
            }
            auto index = opt->AddBindlessType(expr->type());
            str << '(';
            auto args = expr->arguments();
            for (auto &&i : args) {
                i->accept(vis);
                str << ',';
            }
            str << "bdls"sv
                << vstd::to_string(index)
                << ')';
            return;
        }
        case CallOp::ASSUME:
        case CallOp::UNREACHABLE: {
            return;
        }
        case CallOp::BINDLESS_TEXTURE2D_SAMPLE:
        case CallOp::BINDLESS_TEXTURE2D_SAMPLE_LEVEL:
        case CallOp::BINDLESS_TEXTURE2D_SAMPLE_GRAD:
            str << "SampleTex2D"sv;
            break;
        case CallOp::BINDLESS_TEXTURE3D_SAMPLE:
        case CallOp::BINDLESS_TEXTURE3D_SAMPLE_LEVEL:
        case CallOp::BINDLESS_TEXTURE3D_SAMPLE_GRAD:
            str << "SampleTex3D"sv;
            break;
        case CallOp::BINDLESS_TEXTURE2D_READ:
        case CallOp::BINDLESS_TEXTURE2D_READ_LEVEL:
            str << "ReadTex2D"sv;
            break;
        case CallOp::BINDLESS_TEXTURE3D_READ:
        case CallOp::BINDLESS_TEXTURE3D_READ_LEVEL:
            str << "ReadTex3D"sv;
            break;
        case CallOp::BINDLESS_TEXTURE2D_SIZE:
        case CallOp::BINDLESS_TEXTURE2D_SIZE_LEVEL:
            str << "Tex2DSize"sv;
            break;
        case CallOp::BINDLESS_TEXTURE3D_SIZE:
        case CallOp::BINDLESS_TEXTURE3D_SIZE_LEVEL:
            str << "Tex3DSize"sv;
            break;
        case CallOp::SYNCHRONIZE_BLOCK:
            str << "GroupMemoryBarrierWithGroupSync()"sv;
            return;
        case CallOp::INSTANCE_TO_WORLD_MATRIX: {
            str << "InstMatrix("sv;
            args[0]->accept(vis);
            str << "Inst,"sv;
            args[1]->accept(vis);
            str << ')';
            return;
        }
        case CallOp::SET_INSTANCE_TRANSFORM: {
            str << "SetAccelTransform("sv;
            args[0]->accept(vis);
            str << "Inst,"sv;
            for (auto i : vstd::range(1, args.size())) {
                args[i]->accept(vis);
                if (i != (args.size() - 1)) {
                    str << ',';
                }
            }
            str << ')';
            return;
        }
        case CallOp::SET_INSTANCE_VISIBILITY: {
            str << "SetAccelVis("sv;
            args[0]->accept(vis);
            str << "Inst,"sv;
            for (auto i : vstd::range(1, args.size())) {
                args[i]->accept(vis);
                if (i != (args.size() - 1)) {
                    str << ',';
                }
            }
            str << ')';
            return;
        }
        case CallOp::GET_VERTEX_DATA:
            str << "get_vert"sv;
            break;
        default: {
            auto errorType = expr->op();
            VEngine_Log("Function Not Implemented"sv);
            VSTL_ABORT();
        }
    }
    str << '(';
    PrintArgs();
    str << ')';
}
size_t CodegenUtility::GetTypeSize(Type const &t) {
    switch (t.tag()) {
        case Type::Tag::BOOL:
            return 1;
        case Type::Tag::FLOAT:
        case Type::Tag::INT:
        case Type::Tag::UINT:
            return 4;
        case Type::Tag::VECTOR:
            switch (t.dimension()) {
                case 1:
                    return 4;
                case 2:
                    return 8;
                default:
                    return 16;
            }
        case Type::Tag::MATRIX: {
            return 4 * t.dimension() * sizeof(float);
        }
        case Type::Tag::STRUCTURE: {
            size_t v = 0;
            size_t maxAlign = 0;
            for (auto &&i : t.members()) {
                auto align = GetTypeAlign(*i);
                v = detail::CalcAlign(v, align);
                maxAlign = std::max(align, align);
                v += GetTypeSize(*i);
            }
            v = detail::CalcAlign(v, maxAlign);
            return v;
        }
        case Type::Tag::ARRAY: {
            return GetTypeSize(*t.element()) * t.dimension();
        }
        default:
            return 0;
    }
}

size_t CodegenUtility::GetTypeAlign(Type const &t) {// TODO: use t.alignment()
    switch (t.tag()) {
        case Type::Tag::BOOL:
        case Type::Tag::FLOAT:
        case Type::Tag::INT:
        case Type::Tag::UINT:
            return 4;
            // TODO: incorrect
        case Type::Tag::VECTOR:
            switch (t.dimension()) {
                case 1:
                    return 4;
                case 2:
                    return 8;
                default:
                    return 16;
            }
        case Type::Tag::MATRIX: {
            return 16;
        }
        case Type::Tag::ARRAY: {
            return GetTypeAlign(*t.element());
        }
        case Type::Tag::STRUCTURE: {
            return 16;
        }
        case Type::Tag::BUFFER:
        case Type::Tag::TEXTURE:
        case Type::Tag::ACCEL:
        case Type::Tag::BINDLESS_ARRAY:
            return 8;
        default:
            LUISA_ERROR_WITH_LOCATION(
                "Invalid type: {}.", t.description());
    }
}

template<typename T>
struct TypeNameStruct {
    void operator()(vstd::string &str) {
        using BasicTypeUtil = vstd::VariantVisitor_t<basic_types>;
        if constexpr (std::is_same_v<bool, T>) {
            str << "bool";
        } else if constexpr (std::is_same_v<int, T>) {
            str << "int";
        } else if constexpr (std::is_same_v<uint, T>) {
            str << "uint";
        } else if constexpr (std::is_same_v<float, T>) {
            str << "float";
        } else {
            static_assert(vstd::AlwaysFalse<T>, "illegal type");
        }
    }
};
template<typename T, size_t t>
struct TypeNameStruct<luisa::Vector<T, t>> {
    void operator()(vstd::string &str) {
        TypeNameStruct<T>()(str);
        size_t n = (t == 3) ? 4 : t;
        str += ('0' + n);
    }
};
template<size_t t>
struct TypeNameStruct<luisa::Matrix<t>> {
    void operator()(vstd::string &str) {
        TypeNameStruct<float>()(str);
        if constexpr (t == 2) {
            str << "2x2";
        } else if constexpr (t == 3) {
            str << "4x3";
        } else if constexpr (t == 4) {
            str << "4x4";
        } else {
            static_assert(vstd::AlwaysFalse<luisa::Matrix<t>>, "illegal type");
        }
    }
};
void CodegenUtility::GetBasicTypeName(uint64 typeIndex, vstd::string &str) {
    vstd::VariantVisitor_t<basic_types>()(
        [&]<typename T>() {
            TypeNameStruct<T>()(str);
        },
        typeIndex);
}
void CodegenUtility::CodegenFunction(Function func, vstd::string &result, bool cbufferNonEmpty) {

    auto codegenOneFunc = [&](Function func) {
        auto constants = func.constants();
        for (auto &&i : constants) {
            vstd::string constValueName;
            if (!GetConstName(i.data.hash(), i.data, constValueName)) continue;
            result << "static const "sv;
            GetTypeName(*i.type->element(), result, Usage::READ);
            result << ' ' << constValueName << '[';
            vstd::to_string(i.type->dimension(), result);
            result << "]={"sv;
            auto &&dataView = i.data.view();
            eastl::visit(
                [&]<typename T>(eastl::span<T> const &sp) {
                    for (auto i : vstd::range(sp.size())) {
                        auto &&value = sp[i];
                        PrintValue<std::remove_cvref_t<T>>()(value, result);
                        if (i != (sp.size() - 1)) {
                            result << ',';
                        }
                    }
                },
                dataView);
            result << "};\n"sv;
        }

        if (func.tag() == Function::Tag::KERNEL) {
            result << "[numthreads("
                   << vstd::to_string(func.block_size().x)
                   << ','
                   << vstd::to_string(func.block_size().y)
                   << ','
                   << vstd::to_string(func.block_size().z)
                   << R"()]
void main(uint3 thdId:SV_GroupThreadId,uint3 dspId:SV_DispatchThreadID,uint3 grpId:SV_GroupId){
if(any(dspId >= dsp_c[0])) return;
)"sv;
            if (cbufferNonEmpty) {
                result << "Args a = _Global[0];\n"sv;
            }
            opt->funcType = CodegenStackData::FuncType::Kernel;
            opt->arguments.Clear();
            opt->arguments.reserve(func.arguments().size());
            size_t idx = 0;
            for (auto &&i : func.arguments()) {
                opt->arguments.Emplace(i.uid(), idx);
                ++idx;
            }
        } else {
            GetFunctionDecl(func, result);
            result << "{\n"sv;
            opt->funcType = CodegenStackData::FuncType::Callable;
        }
        StringStateVisitor vis(func, result);
        vis.sharedVariables = &opt->sharedVariable;
        vis.VisitFunction(func);
        result << "}\n"sv;
    };
    vstd::HashMap<void const *> callableMap;
    auto callable = [&](auto &&callable, Function func) -> void {
        for (auto &&i : func.custom_callables()) {
            if (callableMap.TryEmplace(i.get()).second) {
                Function f(i.get());
                callable(callable, f);
            }
        }
        codegenOneFunc(func);
    };
    callable(callable, func);
}
void CodegenUtility::CodegenVertex(Function vert, vstd::string &result, bool cBufferNonEmpty) {
    vstd::HashMap<void const *> callableMap;
    auto gen = [&](auto &callable, Function func) -> void {
        for (auto &&i : func.custom_callables()) {
            if (callableMap.TryEmplace(i.get()).second) {
                Function f(i.get());
                callable(callable, f);
            }
        }
    };
    auto callable = [&](auto &callable, Function func) -> void {
        gen(callable, func);
        CodegenFunction(func, result, cBufferNonEmpty);
    };
    gen(callable, vert);
    vstd::string retName;
    auto retType = vert.return_type();
    GetTypeName(*retType, retName, Usage::READ);
    result << retName << " vert(vertex vt){\n"sv;
    if (cBufferNonEmpty) {
        result << "Args a = _Global[0];\n"sv;
    }
    opt->funcType = CodegenStackData::FuncType::Vert;
    opt->arguments.Clear();
    opt->arguments.reserve(vert.arguments().size());
    size_t idx = 0;
    for (auto &&i : vert.arguments()) {
        opt->arguments.Emplace(i.uid(), idx);
        ++idx;
    }
    {
        StringStateVisitor vis(vert, result);
        vis.sharedVariables = &opt->sharedVariable;
        vis.VisitFunction(vert);
    }
    result << R"(
}
v2p main(vertex vt){
v2p o;
)"sv;

    if (retType->is_vector()) {
        result << "o.v0=vert(vt);\n"sv;
    } else {
        result << retName
               << " r=vert(vt);\n"sv;
        for (auto i : vstd::range(retType->members().size())) {
            auto num = vstd::to_string(i);
            result << "o.v"sv << num << "=r.v"sv << num << ";\n"sv;
        }
    }
    result << R"(return o;
}
)"sv;
}
void CodegenUtility::CodegenPixel(Function pixel, vstd::string &result, bool cBufferNonEmpty) {
    vstd::HashMap<void const *> callableMap;
    auto gen = [&](auto &callable, Function func) -> void {
        for (auto &&i : func.custom_callables()) {
            if (callableMap.TryEmplace(i.get()).second) {
                Function f(i.get());
                callable(callable, f);
            }
        }
    };
    auto callable = [&](auto &callable, Function func) -> void {
        gen(callable, func);
        CodegenFunction(func, result, cBufferNonEmpty);
    };
    gen(callable, pixel);
    vstd::string retName;
    auto retType = pixel.return_type();
    GetTypeName(*retType, retName, Usage::READ);
    result << retName << " pixel(v2p p){\n"sv;
    if (cBufferNonEmpty) {
        result << "Args a = _Global[0];\n"sv;
    }
    opt->funcType = CodegenStackData::FuncType::Pixel;
    opt->pixelFirstArgIsStruct = pixel.arguments()[0].type()->is_structure();
    opt->arguments.Clear();
    opt->arguments.reserve(pixel.arguments().size());
    size_t idx = 0;
    for (auto &&i : pixel.arguments()) {
        opt->arguments.Emplace(i.uid(), idx);
        ++idx;
    }
    {
        StringStateVisitor vis(pixel, result);
        vis.sharedVariables = &opt->sharedVariable;
        vis.VisitFunction(pixel);
    }
    result << R"(
}
void main(v2p p)"sv;
    if (retType->is_scalar() || retType->is_vector()) {
        result << ",out "sv;
        GetTypeName(*retType, result, Usage::READ);
        result << R"( o0:SV_TARGET0){
o0=pixel(p);
}
)"sv;
    } else if (retType->is_structure()) {
        size_t idx = 0;
        for (auto &&i : retType->members()) {
            result << ",out "sv;
            GetTypeName(*i, result, Usage::READ);
            auto num = vstd::to_string(idx);
            result << " o"sv << num << ":SV_TARGET"sv << num;
            ++idx;
        }
        result << "){\n"sv;
        GetTypeName(*retType, result, Usage::READ);
        result << " o=pixel(p);\n"sv;
        idx = 0;
        for (auto &&i : retType->members()) {
            auto num = vstd::to_string(idx);
            result << 'o' << num << "=o.v"sv << num << ";\n"sv;
            ++idx;
        }
        result << "}\n"sv;
    } else {
        LUISA_ERROR("Illegal pixel shader return type!");
    }

    // TODO
    // pixel return value
    // value assignment
}
namespace detail {
static bool IsCBuffer(Variable::Tag t) {
    switch (t) {
        case Variable::Tag::BUFFER:
        case Variable::Tag::TEXTURE:
        case Variable::Tag::BINDLESS_ARRAY:
        case Variable::Tag::ACCEL:
        case Variable::Tag::THREAD_ID:
        case Variable::Tag::BLOCK_ID:
        case Variable::Tag::DISPATCH_ID:
        case Variable::Tag::DISPATCH_SIZE:
            return false;
    }
    return true;
}
}// namespace detail
bool CodegenUtility::IsCBufferNonEmpty(std::initializer_list<vstd::IRange<Variable> *> fs) {
    for (auto &&f : fs) {
        for (auto &&i : *f) {
            if (detail::IsCBuffer(i.tag())) {
                return true;
            }
        }
    }
    return false;
}
bool CodegenUtility::IsCBufferNonEmpty(Function f) {
    for (auto &&i : f.arguments()) {
        if (detail::IsCBuffer(i.tag())) {
            return true;
        }
    }
    return false;
}
void CodegenUtility::GenerateCBuffer(
    std::initializer_list<vstd::IRange<Variable> *> fs,
    vstd::string &result) {
    result << "struct Args{\n"sv;
    size_t alignCount = 0;

    for (auto &&f : fs) {
        for (auto &&i : *f) {
            if (!detail::IsCBuffer(i.tag())) continue;
            GetTypeName(*i.type(), result, Usage::READ);
            result << ' ';
            GetVariableName(i, result);
            result << ";\n"sv;
        }
    }
    result << R"(};
StructuredBuffer<Args> _Global:register(t1);
)"sv;
}
#ifdef USE_SPIRV
void CodegenUtility::GenerateBindlessSpirv(
    vstd::string &str) {
    for (auto &&i : opt->bindlessBufferTypes) {
        str << "StructuredBuffer<"sv;
        if (i.first->is_matrix()) {
            auto n = i.first->dimension();
            str << luisa::format("WrappedFloat{}x{}", n, n);
        } else if (i.first->is_vector() && i.first->dimension() == 3) {
            str << "float4"sv;
        } else {
            GetTypeName(*i.first, str, Usage::READ);
        }
        vstd::string instName("bdls"sv);
        vstd::to_string(i.second, instName);
        str << "> " << instName << "[]:register(t0,space1);"sv;
    }
}
#endif
void CodegenUtility::GenerateBindless(
    CodegenResult::Properties &properties,
    vstd::string &str) {
    for (auto &&i : opt->bindlessBufferTypes) {
        str << "StructuredBuffer<"sv;
        if (i.first->is_matrix()) {
            auto n = i.first->dimension();
            str << luisa::format("WrappedFloat{}x{}", n, n);
        } else if (i.first->is_vector() && i.first->dimension() == 3) {
            str << "float4"sv;
        } else {
            GetTypeName(*i.first, str, Usage::READ);
        }
        vstd::string instName("bdls"sv);
        vstd::to_string(i.second, instName);
        str << "> " << instName << "[]:register(t0,space"sv;
        vstd::to_string(i.second + 3, str);
        str << ");\n"sv;

        properties.emplace_back(
            Property{
                ShaderVariableType::SRVDescriptorHeap,
                static_cast<uint>(i.second + 3u),
                0u, 0u});
    }
}

void CodegenUtility::PreprocessCodegenProperties(CodegenResult::Properties &properties, vstd::string &varData, vstd::array<uint, 3> &registerCount, bool cbufferNonEmpty) {
    registerCount = {0u, 0u, 1u};
    properties.emplace_back(
        Property{
            ShaderVariableType::SampDescriptorHeap,
            1u,
            0u,
            16u});
    properties.emplace_back(
        Property{
            ShaderVariableType::StructuredBuffer,
            0,
            0,
            0});
    if (cbufferNonEmpty) {
        registerCount[2] += 1;
        properties.emplace_back(
            Property{
                ShaderVariableType::StructuredBuffer,
                0,
                1,
                0});
    }
    properties.emplace_back(
        Property{
            ShaderVariableType::SRVDescriptorHeap,
            1,
            0,
            0});
    properties.emplace_back(
        Property{
            ShaderVariableType::SRVDescriptorHeap,
            2,
            0,
            0});

    GenerateBindless(properties, varData);
}
void CodegenUtility::PostprocessCodegenProperties(CodegenResult::Properties &properties, vstd::string &finalResult) {
    if (!opt->customStructVector.empty()) {
        vstd::vector<const StructGenerator *, VEngine_AllocType::VEngine, 8> structures(
            opt->customStructVector.begin(),
            opt->customStructVector.end());
        pdqsort(structures.begin(), structures.end(), [](auto lhs, auto rhs) noexcept {
            return lhs->GetType()->index() < rhs->GetType()->index();
        });
        for (auto v : structures) {
            finalResult << "struct " << v->GetStructName() << "{\n"
                        << v->GetStructDesc() << "};\n";
        }
    }
    for (auto &&kv : opt->sharedVariable) {
        auto &&i = kv.second;
        finalResult << "groupshared "sv;
        GetTypeName(*i.type()->element(), finalResult, Usage::READ);
        finalResult << ' ';
        GetVariableName(i, finalResult);
        finalResult << '[';
        vstd::to_string(i.type()->dimension(), finalResult);
        finalResult << "];\n"sv;
    }
}
void CodegenUtility::CodegenProperties(
    CodegenResult::Properties &properties,
    vstd::string &finalResult,
    vstd::string &varData,
    Function kernel,
    uint offset,
    vstd::array<uint, 3> &registerCount) {
    enum class RegisterType : vbyte {
        CBV,
        UAV,
        SRV
    };
    auto Writable = [&](Variable const &v) {
        return (static_cast<uint>(kernel.variable_usage(v.uid())) & static_cast<uint>(Usage::WRITE)) != 0;
    };
    auto args = kernel.arguments();
    for (auto &&i : vstd::ptr_range(args.data() + offset, args.size() - offset)) {
        auto print = [&] {
            GetTypeName(*i.type(), varData, kernel.variable_usage(i.uid()));
            varData << ' ';
            vstd::string varName;
            GetVariableName(i, varName);
            varData << varName;
        };
        auto printInstBuffer = [&]<bool writable>() {
            if constexpr (writable)
                varData << "RWStructuredBuffer<MeshInst> "sv;
            else
                varData << "StructuredBuffer<MeshInst> "sv;
            vstd::string varName;
            GetVariableName(i, varName);
            varName << "Inst"sv;
            varData << varName;
        };
        auto genArg = [&]<bool rtBuffer = false, bool writable = false>(RegisterType regisT, ShaderVariableType sT, char v) {
            auto &&r = registerCount[(vbyte)regisT];
            Property prop = {
                .type = sT,
                .spaceIndex = 0,
                .registerIndex = r,
                .arrSize = 0};
            if constexpr (rtBuffer) {
                printInstBuffer.operator()<writable>();
                properties.emplace_back(prop);

            } else {
                print();
                properties.emplace_back(prop);
            }
            varData << ":register("sv << v;
            vstd::to_string(r, varData);
            varData << ");\n"sv;
            r++;
        };

        switch (i.type()->tag()) {
            case Type::Tag::TEXTURE:
                if (Writable(i)) {
                    genArg(RegisterType::UAV, ShaderVariableType::UAVDescriptorHeap, 'u');
                } else {
                    genArg(RegisterType::SRV, ShaderVariableType::SRVDescriptorHeap, 't');
                }
                break;
            case Type::Tag::BUFFER: {
                if (Writable(i)) {
                    genArg(RegisterType::UAV, ShaderVariableType::RWStructuredBuffer, 'u');
                } else {
                    genArg(RegisterType::SRV, ShaderVariableType::StructuredBuffer, 't');
                }
            } break;
            case Type::Tag::BINDLESS_ARRAY:
                genArg(RegisterType::SRV, ShaderVariableType::StructuredBuffer, 't');
                break;
            case Type::Tag::ACCEL:
                if (Writable(i)) {
                    genArg.operator()<true, true>(RegisterType::UAV, ShaderVariableType::RWStructuredBuffer, 'u');
                } else {
                    genArg(RegisterType::SRV, ShaderVariableType::StructuredBuffer, 't');
                    genArg.operator()<true>(RegisterType::SRV, ShaderVariableType::StructuredBuffer, 't');
                }
                break;
        }
    }
}

CodegenResult CodegenUtility::Codegen(
    Function kernel, vstd::string_view internalDataPath) {
    assert(kernel.tag() == Function::Tag::KERNEL);
    opt = CodegenStackData::Allocate();
    // CodegenStackData::ThreadLocalSpirv() = false;
    opt->kernel = kernel;
    auto disposeOpt = vstd::scope_exit([&] {
        CodegenStackData::DeAllocate(std::move(opt));
    });
    bool nonEmptyCbuffer = IsCBufferNonEmpty(kernel);

    vstd::string codegenData;
    vstd::string varData;
    CodegenFunction(kernel, codegenData, nonEmptyCbuffer);
    vstd::string finalResult;
    finalResult.reserve(65500);

    finalResult << detail::HLSLHeader(internalDataPath);
    if (kernel.requires_raytracing()) {
        finalResult << detail::RayTracingHeader(internalDataPath);
    }
    opt->funcType = CodegenStackData::FuncType::Callable;
    auto argRange = vstd::RangeImpl(vstd::CacheEndRange(kernel.arguments()) | vstd::ValueRange{});
    if (nonEmptyCbuffer) {
        GenerateCBuffer({static_cast<vstd::IRange<Variable> *>(&argRange)}, varData);
    }
    varData << "StructuredBuffer<uint3> dsp_c:register(t0);\n"sv;
    CodegenResult::Properties properties;
    uint64 immutableHeaderSize = finalResult.size();
    vstd::array<uint, 3> registerCount;
    PreprocessCodegenProperties(properties, varData, registerCount, nonEmptyCbuffer);
    CodegenProperties(properties, finalResult, varData, kernel, 0, registerCount);
    PostprocessCodegenProperties(properties, finalResult);
    finalResult << varData << codegenData;
    return {
        std::move(finalResult),
        std::move(properties),
        opt->bindlessBufferCount,
        immutableHeaderSize};
}
CodegenResult CodegenUtility::RasterCodegen(
    MeshFormat const &meshFormat,
    Function vertFunc,
    Function pixelFunc,
    vstd::string_view internalDataPath) {
    opt = CodegenStackData::Allocate();
    // CodegenStackData::ThreadLocalSpirv() = false;
    opt->kernel = vertFunc;
    auto disposeOpt = vstd::scope_exit([&] {
        CodegenStackData::DeAllocate(std::move(opt));
    });
    vstd::string codegenData;
    vstd::string varData;
    vstd::string finalResult;
    finalResult.reserve(65500);
    // Vertex
    codegenData << "struct v2p{\n"sv;
    auto v2pType = vertFunc.return_type();
    if (v2pType->is_structure()) {
        size_t memberIdx = 0;
        for (auto &&i : v2pType->members()) {
            GetTypeName(*i, codegenData, Usage::READ);
            codegenData << " v"sv << vstd::to_string(memberIdx);
            if (memberIdx == 0) {
                codegenData << ":SV_POSITION;\n"sv;
            } else {
                codegenData << ":TEXCOORD"sv << vstd::to_string(memberIdx - 1) << ";\n"sv;
            }
            ++memberIdx;
        }
    } else if (v2pType->is_vector() && v2pType->dimension() == 4) {
        codegenData << "float4 v0:SV_POSITION;\n"sv;
    } else {
        LUISA_ERROR("Illegal vertex return type!");
    }
    codegenData << R"(};
uint obj_id:register(b0);
#ifdef VS
#define get_vert(tposition,tnormal,ttangent,tcolor,tuv0,tuv1,tuv2,tuv3,tvid,tiid){tvid=vt.vid;tiid=vt.iid;)"sv;
    std::bitset<kVertexAttributeCount> bits;
    bits.reset();
    auto vertexAttriName = {
        "position"sv,
        "normal"sv,
        "tangent"sv,
        "color"sv,
        "uv0"sv,
        "uv1"sv,
        "uv2"sv,
        "uv3"sv};
    auto semanticName = {
        "POSITION"sv,
        "NORMAL"sv,
        "TANGENT"sv,
        "COLOR"sv,
        "UV0"sv,
        "UV1"sv,
        "UV2"sv,
        "UV3"sv};
    auto semanticType = {
        "float3"sv,
        "float3"sv,
        "float4"sv,
        "float4"sv,
        "float2"sv,
        "float2"sv,
        "float2"sv,
        "float2"sv};
    for (auto i : vstd::range(meshFormat.vertex_stream_count())) {
        for (auto &&j : meshFormat.attributes(i)) {
            auto type = j.type;
            assert(!bits[static_cast<size_t>(type)]);
            bits[static_cast<size_t>(type)] = true;
            auto name = vertexAttriName.begin()[static_cast<size_t>(type)];
            codegenData << 't' << name << "=vt."sv << name << ';';
        }
    }
    for (auto i : vstd::range(bits.size())) {
        if (!bits[i]) {
            auto name = vertexAttriName.begin()[i];
            codegenData << 't' << name << "=0;"sv;
        }
    }
    codegenData << "}\n"sv;

    codegenData << "struct vertex{\n"sv;
    for (auto i : vstd::range(meshFormat.vertex_stream_count())) {
        for (auto &&j : meshFormat.attributes(i)) {
            auto type = j.type;
            auto idx = static_cast<size_t>(type);
            codegenData << semanticType.begin()[idx] << ' ' << vertexAttriName.begin()[idx] << ':' << semanticName.begin()[idx] << ";\n"sv;
        }
    }
    codegenData << R"(uint vid:SV_VERTEXID;
uint iid:SV_INSTANCEID;
};
)"sv;
    auto vertRange = vstd::RangeImpl(vstd::CacheEndRange(vertFunc.arguments()) | vstd::ValueRange{});
    auto pixelRange = vstd::RangeImpl(vstd::ite_range(pixelFunc.arguments().begin() + 1, pixelFunc.arguments().end()) | vstd::ValueRange{});
    std::initializer_list<vstd::IRange<Variable> *> funcs = {&vertRange, &pixelRange};

    bool nonEmptyCbuffer = IsCBufferNonEmpty(funcs);

    CodegenVertex(vertFunc, codegenData, nonEmptyCbuffer);
    // TODO: gen vertex data
    codegenData << "#elif defined(PS)\n"sv;
    // TODO: gen pixel data
    CodegenPixel(pixelFunc, codegenData, nonEmptyCbuffer);
    codegenData << "#endif\n"sv;
    finalResult << detail::HLSLHeader(internalDataPath);
    if (vertFunc.requires_raytracing() || pixelFunc.requires_raytracing()) {
        finalResult << detail::RayTracingHeader(internalDataPath);
    }
    opt->funcType = CodegenStackData::FuncType::Callable;
    if (nonEmptyCbuffer) {
        GenerateCBuffer(funcs, varData);
    }
    CodegenResult::Properties properties;
    uint64 immutableHeaderSize = finalResult.size();
    vstd::array<uint, 3> registerCount;
    PreprocessCodegenProperties(properties, varData, registerCount, nonEmptyCbuffer);
    CodegenProperties(properties, finalResult, varData, vertFunc, 0, registerCount);
    CodegenProperties(properties, finalResult, varData, pixelFunc, 1, registerCount);
    PostprocessCodegenProperties(properties, finalResult);
    finalResult << varData << codegenData;
    return {
        std::move(finalResult),
        std::move(properties),
        opt->bindlessBufferCount,
        immutableHeaderSize};
}
}// namespace toolhub::directx
