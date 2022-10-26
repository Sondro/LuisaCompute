#include <py/py_stream.h>
#include <runtime/command_buffer.h>
namespace luisa::compute {
PyStream::PyStream(Device &device)
    : data(new Data(device)) {}
PyStream::Data::Data(Device &device)
    : stream(device.create_stream()),
      buffer(stream.command_buffer()) {
}
PyStream::~PyStream() {
    if(!data) return;
    if (!data->buffer.empty()) {
        data->buffer.commit();
    }
    data->stream.synchronize();
}

void PyStream::Add(Command *cmd) {
    data->buffer << luisa::unique_ptr<Command>(cmd);
}
void PyStream::Execute() {
    data->buffer << [d = data.get()] { d->readbackDisposer.clear(); };
    data->uploadDisposer.clear();
}
void PyStream::ExecuteCallback(std::function<void()> &&func) {
    data->buffer << [func = std::move(func), d = data.get()] {
        func();
        d->readbackDisposer.clear();
    };
}
}// namespace luisa::compute