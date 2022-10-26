#pragma once
#include <vstl/Common.h>
#include <vstl/functional.h>
#include <runtime/command_buffer.h>
#include <runtime/stream.h>
#include <runtime/device.h>
namespace luisa::compute {
class PyStream : public vstd::IOperatorNewBase {
    struct Disposer {
        void *ptr;
        vstd::funcPtr_t<void(void *ptr)> dtor;
        Disposer() {}
        Disposer(Disposer &&d) {
            ptr = d.ptr;
            d.ptr = nullptr;
            dtor = d.dtor;
        }
        ~Disposer() {
            if (!ptr) return;
            dtor(ptr);
            vengine_delete(ptr);
        }
    };

public:
    struct Data : public vstd::IOperatorNewBase {
        Stream stream;
        CommandBuffer buffer;
        vstd::vector<Disposer> uploadDisposer;
        vstd::vector<Disposer> readbackDisposer;
        Data(Device& device);
    };
    vstd::unique_ptr<Data> data;
    PyStream(PyStream &&) = default;
    PyStream(PyStream const &) = delete;
    PyStream(Device &device);
    ~PyStream();
    void Add(Command *cmd);
    template<typename T>
        requires(!std::is_reference_v<T>)
    void AddUploadRef(T &&t) {
        auto &disp = data->uploadDisposer.emplace_back();
        disp.ptr = vengine_malloc(sizeof(T));
        new(disp.ptr)T(std::move(t));
        disp.dtor = [](void *ptr) { reinterpret_cast<T *>(ptr)->~T(); };
    }
    template<typename T>
        requires(!std::is_reference_v<T>)
    void AddReadbackRef(T &&t) {
        auto &disp = data->readbackDisposer.emplace_back();
        disp.ptr = vengine_malloc(sizeof(T));
        new(disp.ptr)T(std::move(t));
        disp.dtor = [](void *ptr) { reinterpret_cast<T *>(ptr)->~T(); };
    }
    void Execute();
    void ExecuteCallback(std::function<void()> &&func);
};
}// namespace luisa::compute