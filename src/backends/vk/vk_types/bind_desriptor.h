#pragma once
#include <vulkan_include.h>
#include <vk_types/buffer_view.h>
#include <vk_types/tex_view.h>
namespace toolhub::vk {
class Accel;
using BindResource = vstd::variant<
	TexView,
	BufferView,
	Accel const*>;
}// namespace toolhub::vk