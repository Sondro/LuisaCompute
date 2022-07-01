#pragma once
#include <vulkan_include.h>
#include <vk_types/buffer_view.h>
#include <vk_types/tex_view.h>
namespace toolhub::vk {
class Texture;
class Buffer;
class Accel;
struct BindResource {
	vstd::variant<
		Texture const*,
		Buffer const*,
		Accel const*>
		res;
	size_t offset;
	size_t size;
	BindResource(BufferView const& bf)
		: res(bf.buffer),
		  offset(bf.offset),
		  size(bf.size) {}
	BindResource(TexView const& tex)
		: res(tex.tex),
		  offset(tex.mipOffset),
		  size(tex.mipCount) {}
	BindResource(Accel const* accel)
		: res(accel) {}
};
}// namespace toolhub::vk