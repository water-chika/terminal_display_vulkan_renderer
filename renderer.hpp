#include <vulkan/vulkan.hpp>
#include <ranges>
#include <functional>

using namespace vk;

class terminal_display_vulkan_renderer {
public:
	terminal_display_vulkan_renderer() : 
		m_instance{ nullptr },
		m_physical_device{nullptr},
		m_device{nullptr},
		m_surface{nullptr},
		m_swapchain{nullptr}
	{
		create_instance();
		m_destroy_calls.push_back(destroy_instance);
		select_physical_device();
		create_device();
		m_destroy_calls.push_back(destroy_device);
	}
	~terminal_display_vulkan_renderer() {
		std::ranges::for_each(m_destroy_calls,
			[this](auto destroy_call) {
				destroy_call(this);
			});
	}
private:
	void create_instance() {
		InstanceCreateInfo info{};
		m_instance = createInstance(info);
	}
	void destroy_instance() {
		m_instance.destroy();
	}
	void select_physical_device() {
		auto physical_devices = m_instance.enumeratePhysicalDevices();
		auto ite = std::ranges::find_if(
			physical_devices,
			[](PhysicalDevice physical_device) {
				return physical_device.getProperties().deviceType == PhysicalDeviceType::eDiscreteGpu;
			});
		m_physical_device = *ite;
	}
	void create_device() {
		DeviceCreateInfo info{};
		m_device = m_physical_device.createDevice(info);
	}
	void destroy_device() {
		m_device.destroy();
	}
	void set_surface(SurfaceKHR surface) {
		m_surface = surface;
	}
	void create_swapchain() {
		SwapchainCreateInfoKHR info{};
		info.setSurface(m_surface);
		m_swapchain = m_device.createSwapchainKHR(info);
	}

	std::vector<std::function<void(terminal_display_vulkan_renderer*)>> m_destroy_calls;

	Instance m_instance;
	PhysicalDevice m_physical_device;
	Device m_device;
	SurfaceKHR m_surface;
	SwapchainKHR m_swapchain;
};