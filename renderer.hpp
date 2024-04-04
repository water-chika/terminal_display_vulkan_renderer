#include <vulkan/vulkan.hpp>
#include <ranges>
#include <functional>
#include <set>
#include <algorithm>
#include <map>

using namespace vk;

class terminal_display_vulkan_renderer {
public:
	enum class state {
		instance_extensions,
		surface,
		instance,
		physical_device,
		swapchain,
		device,
	};
	terminal_display_vulkan_renderer() : 
		m_instance{ nullptr },
		m_physical_device{nullptr},
		m_device{nullptr},
		m_surface{nullptr},
		m_swapchain{nullptr}
	{
		set_instance_dependences();
		set_physical_device_dependences();
		set_device_dependences();
		set_surface_dependences();
		set_swapchain_dependences();
	}
	void add_instance_extensions(std::set<std::string> extensions) {
		std::ranges::for_each(extensions,
			[this](std::string extension) {
				m_instance_extensions.emplace(extension);
			});
		set_state_changed(state::instance_extensions);
	}
	void set_surface_dependences() {}
	Instance get_instance() {
		return m_instance;
	}
	void set_surface(SurfaceKHR surface) {
		m_surface = surface;
		set_state_changed(state::surface);
	}
	~terminal_display_vulkan_renderer() {
		while (!m_destroy_calls.empty()) {
			m_destroy_calls.back()(this);
			m_destroy_calls.pop_back();
		}
	}

	void update() {

	}

private:
	void set_state_changed(state s) {
		m_changed_states.emplace(s);
	}
	void set_instance_dependences() {
		m_dependent_states.emplace(state::instance, state::instance_extensions);
	}
	void create_instance() {
		InstanceCreateInfo info{};
		std::vector<const char*> instance_extensions(m_instance_extensions.size());
		std::ranges::transform(
			m_instance_extensions,
			instance_extensions.begin(),
			[](std::string extension) {
				return extension.c_str();
			});
		info.setPEnabledExtensionNames(instance_extensions);
		m_instance = createInstance(info);
	}
	void destroy_instance() {
		m_instance.destroy();
	}
	void set_physical_device_dependences() {
		m_dependent_states.emplace(state::physical_device, state::instance);
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
	void set_device_dependences() {
		m_dependent_states.emplace(state::device, state::physical_device);
	}
	void create_device() {
		DeviceCreateInfo info{};
		m_device = m_physical_device.createDevice(info);
	}
	void destroy_device() {
		m_device.destroy();
	}
	void set_swapchain_dependences() {
		m_dependent_states.emplace(state::swapchain, state::surface);
	}
	void create_swapchain() {
		SwapchainCreateInfoKHR info{};
		info.setSurface(m_surface);
		m_swapchain = m_device.createSwapchainKHR(info);
	}

	std::set<state> m_changed_states;
	std::multimap<state, state> m_dependent_states;
	std::vector<std::function<void(terminal_display_vulkan_renderer*)>> m_destroy_calls;

	std::set<std::string> m_instance_extensions;
	Instance m_instance;
	PhysicalDevice m_physical_device;
	Device m_device;
	SurfaceKHR m_surface;
	SwapchainKHR m_swapchain;
};