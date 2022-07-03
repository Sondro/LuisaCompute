#pragma once
#include "frame_resource.h"
#include "command_pool.h"
#include "command_buffer.h"
#include "res_state_tracker.h"
namespace toolhub::vk {
class RenderPipeline : public Resource {
	RenderPipeline(RenderPipeline const&) = delete;
	RenderPipeline(RenderPipeline&&) = delete;
	vstd::LockFreeArrayQueue<vstd::variant<
		size_t,
		vstd::move_only_func<void()>,
		std::pair<Event*, size_t>>>
		executingList;
	std::mutex callbackMtx;
	std::mutex syncMtx;
	std::condition_variable callbackCv;
	std::condition_variable syncCv;
    std::atomic_uint64_t callbackPosition = 0;
    std::atomic_uint64_t mainThreadPosition = 0;
	void CallbackThreadMain();

	struct FrameResLck {
		std::mutex mtx;
		std::condition_variable cv;
		bool executing = false;
	};
	static constexpr size_t FRAME_BUFFER = 3;
	ResStateTracker stateTracker;
	CommandPool pool;
	vstd::array<FrameResource, FRAME_BUFFER> frameRes;
	vstd::array<FrameResLck, FRAME_BUFFER> frameResLock;
	//the executing frame
	FrameResource* lastExecuteFrame = nullptr;
	//the preparing frame
	size_t executeFrameIndex = FRAME_BUFFER - 1;
	bool enabled = true;
	// thread
	std::thread callbackThread;

public:
	VkCommandPool Pool() const { return pool.Pool(); }
	ResStateTracker& StateTracker() { return stateTracker; }
	FrameResource* PreparingFrame() { return &frameRes[executeFrameIndex]; }
	RenderPipeline(Device const* device);
	~RenderPipeline();
	// get next frame, wait if not finished
	FrameResource* BeginPrepareFrame();
	void EndPrepareFrame();
	void Complete();
	void ForceSyncInRendering();
	void AddEvtSync(Event* evt);
	void AddCallback(vstd::move_only_func<void()>&& func);
	template<typename... Args>
	void AddTask(Args&&... args);
};
}// namespace toolhub::vk