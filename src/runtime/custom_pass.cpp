#include <runtime/custom_pass.h>
#include <vstl/MetaLib.h>
namespace luisa::compute {
CustomPass::CustomPass(
    luisa::string &&name,
    StreamTag stream_tag,
    size_t capacity)
    : _name(std::move(name)),
      _stream_tag(stream_tag) {
    _bindings.reserve(capacity);
}
CustomPass::~CustomPass() noexcept {
}
luisa::unique_ptr<Command> CustomPass::build() &noexcept {
    auto cmd = std::move(*this).build();
    vstd::reset(_name);
    vstd::reset(_bindings);
    return std::move(cmd);
}
luisa::unique_ptr<Command> CustomPass::build() &&noexcept {
    return CustomCommand::create(
        std::move(_bindings),
        std::move(_name),
        _stream_tag);
}
}// namespace luisa::compute