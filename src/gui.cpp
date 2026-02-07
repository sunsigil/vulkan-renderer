#include "gui.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"

#include "draw.h"

static TOS_context* context = nullptr;
static TOS_device* device = nullptr;
static TOS_swapchain* swapchain = nullptr;

static VkDescriptorPool imgui_descriptor_pool = VK_NULL_HANDLE;

void TOS_create_gui_context(TOS_context* _context, TOS_device* _device, TOS_swapchain* _swapchain)
{
	context = _context;
	device = _device;
	swapchain = _swapchain;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	[[maybe_unused]]
	ImGuiIO& io = ImGui::GetIO();
	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForVulkan(context->window_handle, true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = context->instance;
	init_info.PhysicalDevice = device->physical;
	init_info.Device = device->logical;
	init_info.QueueFamily = TOS_query_queue_families(context, device->physical).graphics.value();
	init_info.Queue = device->queues.graphics;
	init_info.PipelineCache = VK_NULL_HANDLE;

	VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE },
	};
	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 0;
	for (VkDescriptorPoolSize& pool_size : pool_sizes)
		pool_info.maxSets += pool_size.descriptorCount;
	pool_info.poolSizeCount = (uint32_t) IM_ARRAYSIZE(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;
	VkResult result = vkCreateDescriptorPool(device->logical, &pool_info, nullptr, &imgui_descriptor_pool);
	if(result != VK_SUCCESS)
		throw std::runtime_error("main: failed to create imgui descriptor pool");
	init_info.DescriptorPool = imgui_descriptor_pool;

	init_info.Allocator = nullptr;
	init_info.MinImageCount = 2;
	init_info.ImageCount = swapchain->images.size();
	init_info.CheckVkResultFn = nullptr;

	init_info.PipelineInfoMain.RenderPass = swapchain->render_pass;
	init_info.PipelineInfoMain.Subpass = 0;
	init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&init_info);
}

void TOS_destroy_gui_context()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	vkDestroyDescriptorPool(device->logical, imgui_descriptor_pool, nullptr);
}

void TOS_gui_begin_frame()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void TOS_gui_end_frame()
{
	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), TOS_get_command_buffer());
}

void TOS_gui_begin_overlay()
{
	int flags =
	ImGuiWindowFlags_NoSavedSettings |
	ImGuiWindowFlags_NoMove |
	ImGuiWindowFlags_NoResize |
	ImGuiWindowFlags_NoResize |
	ImGuiWindowFlags_NoCollapse |
	ImGuiWindowFlags_NoBackground |
	ImGuiWindowFlags_NoTitleBar |
	ImGuiWindowFlags_NoBringToFrontOnFocus;
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(context->window_width, context->window_height));
	ImGui::Begin("IMGUI Overlay", nullptr, flags);
}

void TOS_gui_end_overlay()
{
	ImGui::End();
}