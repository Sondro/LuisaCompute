#include <py/py_stream.h>
#include <runtime/command_buffer.h>
namespace luisa::compute {
PyStream::PyStream(Device &device) noexcept
    : _data(new Data(device)) {}
PyStream::Data::Data(Device &device) noexcept
    : stream(device.create_stream()),
      buffer(stream.command_buffer()) {
}
PyStream::~PyStream() noexcept {
    if (!_data) return;
    if (!_data->buffer.empty()) {
        _data->buffer.commit();
    }
    _data->stream.synchronize();
}

void PyStream::add(Command *cmd) noexcept {
    _data->buffer << luisa::unique_ptr<Command>(cmd);
}
void PyStream::execute() noexcept {
    _data->buffer << [d = _data.get()] { d->readbackDisposer.clear(); };
    _data->uploadDisposer.clear();
}
void PyStream::execute_callback(luisa::function<void()> &&func) noexcept {
    _data->buffer << [func = std::move(func), d = _data.get()] {
        func();
        d->readbackDisposer.clear();
    };
}
void PyStream::sync() noexcept {
    _data->buffer.commit();
    _data->buffer.synchronize();
}

}// namespace luisa::compute