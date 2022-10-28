//
// Created by Mike Smith on 2021/6/24.
//

#pragma once

#include <core/dll_export.h>
#include <core/basic_types.h>
#include <dsl/expr.h>
#include <rtx/ray.h>
#include <rtx/hit.h>
#include <rtx/mesh.h>

namespace luisa::compute {

class LC_RUNTIME_API Accel final : public Resource {

public:
    using UsageHint = AccelUsageHint;
    using BuildRequest = AccelBuildRequest;
    using Modification = AccelBuildCommand::Modification;

private:
    luisa::unordered_map<size_t, Modification> _modifications;
    luisa::vector<uint64_t> _mesh_handles;

private:
    friend class Device;
    friend class Mesh;
    explicit Accel(DeviceInterface *device, UsageHint hint = UsageHint::FAST_TRACE, bool allow_compact = false, bool allow_update = true) noexcept;

public:
    Accel() noexcept = default;
    Accel(Accel &&) noexcept = default;
    Accel(Accel const&) = delete;
    using Resource::operator bool;
    [[nodiscard]] auto size() const noexcept { return _mesh_handles.size(); }

    // host interfaces
    void emplace_back(Mesh const &mesh, float4x4 transform = make_float4x4(1.f), bool visible = true) noexcept;
    void emplace_back_handle(uint64_t mesh_handle, float4x4 transform, bool visible) noexcept;
    void set(size_t index, const Mesh &mesh, float4x4 transform = make_float4x4(1.f), bool visible = true) noexcept;
    void pop_back() noexcept;
    void set_transform_on_update(size_t index, float4x4 transform) noexcept;
    void set_visibility_on_update(size_t index, bool visible) noexcept;
    [[nodiscard]] luisa::unique_ptr<Command> build(BuildRequest request = BuildRequest::PREFER_UPDATE) noexcept;

    // shader functions
    [[nodiscard]] Var<Hit> trace_closest(Expr<Ray> ray) const noexcept;
    [[nodiscard]] Var<bool> trace_any(Expr<Ray> ray) const noexcept;
    [[nodiscard]] Var<float4x4> instance_transform(Expr<int> instance_id) const noexcept;
    [[nodiscard]] Var<float4x4> instance_transform(Expr<uint> instance_id) const noexcept;
    void set_instance_transform(Expr<int> instance_id, Expr<float4x4> mat) const noexcept;
    void set_instance_transform(Expr<uint> instance_id, Expr<float4x4> mat) const noexcept;
    void set_instance_visibility(Expr<int> instance_id, Expr<bool> vis) const noexcept;
    void set_instance_visibility(Expr<uint> instance_id, Expr<bool> vis) const noexcept;
};

template<>
struct LC_RUNTIME_API Expr<Accel> {

private:
    const RefExpr *_expression{nullptr};

public:
    explicit Expr(const RefExpr *expr) noexcept;
    Expr(const Accel &accel) noexcept;
    [[nodiscard]] auto expression() const noexcept { return _expression; }
    [[nodiscard]] Var<Hit> trace_closest(Expr<Ray> ray) const noexcept;
    [[nodiscard]] Var<bool> trace_any(Expr<Ray> ray) const noexcept;
    [[nodiscard]] Var<float4x4> instance_transform(Expr<uint> instance_id) const noexcept;
    [[nodiscard]] Var<float4x4> instance_transform(Expr<int> instance_id) const noexcept;
    void set_instance_transform(Expr<int> instance_id, Expr<float4x4> mat) const noexcept;
    void set_instance_visibility(Expr<int> instance_id, Expr<bool> vis) const noexcept;
    void set_instance_transform(Expr<uint> instance_id, Expr<float4x4> mat) const noexcept;
    void set_instance_visibility(Expr<uint> instance_id, Expr<bool> vis) const noexcept;
};

template<>
struct Var<Accel> : public Expr<Accel> {
    explicit Var(detail::ArgumentCreation) noexcept
        : Expr<Accel>{detail::FunctionBuilder::current()->accel()} {}
    Var(Var &&) noexcept = default;
    Var(const Var &) noexcept = delete;
    Var &operator=(Var &&) noexcept = delete;
    Var &operator=(const Var &) noexcept = delete;
};

using AccelVar = Var<Accel>;

}// namespace luisa::compute
