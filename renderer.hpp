#include <vulkan/vulkan.hpp>
#include <ranges>
#include <functional>
#include <set>
#include <algorithm>
#include <map>

using namespace vk;

namespace terminal_display_vulkan_renderer {

	class renderer {
	public:
		enum class state {
			instance_extensions,
			surface,
			instance,
			physical_device,
			swapchain,
			device,
		};
		renderer() :
			m_instance{ nullptr },
			m_physical_device{ nullptr },
			m_device{ nullptr },
			m_surface{ nullptr },
			m_swapchain{ nullptr }
		{
			set_instance_states();
			set_physical_device_states();
			set_device_states();
			set_swapchain_states();

			set_state_changed(state::instance_extensions);
		}
		template<std::ranges::range T>
		void add_instance_extensions(T extensions) {
			std::ranges::for_each(extensions,
				[this](std::string extension) {
					m_instance_extensions.push_back(extension);
				});
			set_state_changed(state::instance_extensions);
		}
		Instance get_instance() {
			return m_instance;
		}
		void set_surface(SurfaceKHR surface) {
			m_surface = surface;
			set_state_changed(state::surface);
		}
		~renderer() {
			while (!is_all_destroyed()) {
				for (auto& [state, destroy_state] : m_destroy_functions) {
					bool all_depend_state_destroyed = true;
					for (auto [ite, end] = m_destroy_depend_states.equal_range(state);
						ite != end && all_depend_state_destroyed;
						++ite) {
						auto& [state, influence_state] = *ite;
						auto& is_destroyed = m_is_destroyed_functions[influence_state];
						all_depend_state_destroyed = is_destroyed(this);
					}
					if (all_depend_state_destroyed) {
						destroy_state(this);
					}
				}
			}
		}

		bool is_updated() {
			return m_changed_states.empty();
		}

		void update() {
			std::set<state> changed_states{};
			std::ranges::for_each(m_changed_states,
				[this, &changed_states](state changed_state) {
					for (auto [ite, end] = m_influence_states.equal_range(changed_state); ite != end; ++ite) {
						auto [s, influenced_state] = *ite;
						m_update_functions[influenced_state](this);
						changed_states.emplace(influenced_state);
					}
				});
			m_changed_states = std::move(changed_states);
		}

	private:
		bool is_all_destroyed() {
			return std::ranges::all_of(m_is_destroyed_functions,
				[this](auto& k_v) {
					auto& [state, is_destroyed] = k_v;
					return is_destroyed(this);
				});
		}
		void set_state_changed(state s) {
			m_changed_states.emplace(s);
		}
		void set_instance_states() {
			m_dependent_states.emplace(state::instance, state::instance_extensions);
			m_influence_states.emplace(state::instance_extensions, state::instance);
			m_update_functions.emplace(state::instance, &renderer::create_instance);
			m_destroy_functions.emplace(state::instance, &renderer::destroy_instance);
			m_is_destroyed_functions.emplace(state::instance, &renderer::instance_is_destroyed);
		}
		void create_instance() {
			InstanceCreateInfo info{};
			std::vector<const char*> instance_extensions(m_instance_extensions.size());
			std::ranges::transform(
				m_instance_extensions,
				instance_extensions.begin(),
				[](std::string& extension) {
					return extension.c_str();
				});
			info.setPEnabledExtensionNames(instance_extensions);
			m_instance = createInstance(info);
			set_state_changed(state::instance);
		}
		void destroy_instance() {
			if (m_instance != nullptr) {
				m_instance.destroy();
				m_instance = nullptr;
			}
		}
		bool instance_is_destroyed() const {
			return m_instance == nullptr;
		}
		void set_physical_device_states() {
			m_dependent_states.emplace(state::physical_device, state::instance);
			m_influence_states.emplace(state::instance, state::physical_device);
			m_update_functions.emplace(state::physical_device, &renderer::select_physical_device);
			m_destroy_functions.emplace(state::physical_device, &renderer::deselect_physical_device);
			m_is_destroyed_functions.emplace(state::physical_device, &renderer::physical_device_is_deselected);
		}
		void select_physical_device() {
			if (m_instance != nullptr) {
				auto physical_devices = m_instance.enumeratePhysicalDevices();
				auto ite = std::ranges::find_if(
					physical_devices,
					[](PhysicalDevice physical_device) {
						return physical_device.getProperties().deviceType == PhysicalDeviceType::eDiscreteGpu;
					});
				m_physical_device = *ite;
				set_state_changed(state::physical_device);
			}
		}
		void deselect_physical_device() {
			m_physical_device = nullptr;
		}
		bool physical_device_is_deselected() const {
			return m_physical_device == nullptr;
		}
		void set_device_states() {
			m_dependent_states.emplace(state::device, state::physical_device);
			m_influence_states.emplace(state::physical_device, state::device);
			m_update_functions.emplace(state::device, &renderer::create_device);
			m_destroy_functions.emplace(state::device, &renderer::destroy_device);
			m_is_destroyed_functions.emplace(state::device, &renderer::device_is_destroyed);
			m_destroy_depend_states.emplace(state::instance, state::device);
		}
		void create_device() {
			if (m_physical_device != nullptr) {
				DeviceCreateInfo info{};
				m_device = m_physical_device.createDevice(info);
				set_state_changed(state::device);
			}
		}
		void destroy_device() {
			if (m_device != nullptr) {
				m_device.destroy();
				m_device = nullptr;
			}
		}
		bool device_is_destroyed() const {
			return m_device == nullptr;
		}
		void set_swapchain_states() {
			m_dependent_states.emplace(state::swapchain, state::surface);
			m_influence_states.emplace(state::surface, state::swapchain);
			m_dependent_states.emplace(state::swapchain, state::device);
			m_influence_states.emplace(state::device, state::swapchain);
			m_update_functions.emplace(state::swapchain, &renderer::create_swapchain);
			m_destroy_functions.emplace(state::swapchain, &renderer::destroy_swapchain);
			m_is_destroyed_functions.emplace(state::swapchain, &renderer::swapchain_is_destroyed);
			m_destroy_depend_states.emplace(state::device, state::swapchain);
		}
		void create_swapchain() {
			if (m_surface != nullptr) {
				SwapchainCreateInfoKHR info{};
				info.setSurface(m_surface);
				m_swapchain = m_device.createSwapchainKHR(info);
				set_state_changed(state::swapchain);
			}
		}
		void destroy_swapchain() {
			if (m_swapchain != nullptr) {
				m_device.destroySwapchainKHR(m_swapchain);
				m_swapchain = nullptr;
			}
		}
		bool swapchain_is_destroyed() const {
			return m_swapchain == nullptr;
		}

		std::set<state> m_changed_states;
		std::multimap<state, state> m_dependent_states;
		std::multimap<state, state> m_influence_states;
		std::map<state, std::function<void(renderer*)>> m_update_functions;
		std::map<state, std::function<void(renderer*)>> m_destroy_functions;
		std::map<state, std::function<bool(const renderer*)>> m_is_destroyed_functions;
		std::multimap<state, state> m_destroy_depend_states;

		std::vector<std::string> m_instance_extensions;
		Instance m_instance;
		PhysicalDevice m_physical_device;
		Device m_device;
		SurfaceKHR m_surface;
		SwapchainKHR m_swapchain;
	};

}