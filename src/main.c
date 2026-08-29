// Main entry point for Litt GUI
// Uses C++ bridge for cleaner code

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stdlib.h>
#include "gui_bridge.h"

/* ── Globals ─────────────────────────────────────────────────────────────── */
static const int WIDTH  = 1280;
static const int HEIGHT = 720;

static GLFWwindow* window = nullptr;
static LittGuiBridge* bridge = nullptr;

/* ── Minimal Vulkan initialisation ─────────────────────────────────────── */
static VkAllocationCallbacks g_alloc = {nullptr, nullptr, nullptr, nullptr, nullptr};

static bool init_vulkan(GLFWwindow* win,
                        VkInstance* inst, VkPhysicalDevice* phys,
                        VkDevice* dev, VkQueue* queue,
                        VkSurfaceKHR* surface, VkRenderPass* rp,
                        VkDescriptorPool* pool) {
    /* ── Instance ───────────────────────────────────────────────────── */
    VkApplicationInfo app = {};
    app.sType          = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pApplicationName = "litt-gui";
    app.apiVersion       = VK_MAKE_VERSION(1, 3, 0);

    VkInstanceCreateInfo info = {};
    info.sType               = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    info.pApplicationInfo    = &app;

    uint32_t count = 0;
    const char** exts = glfwGetRequiredInstanceExtensions(&count);
    info.enabledExtensionCount    = count;
    info.ppEnabledExtensionNames  = exts;

    if (vkCreateInstance(&info, &g_alloc, inst) != VK_SUCCESS) {
        fprintf(stderr, "vkCreateInstance failed\n");
        return false;
    }

    /* ── Surface ────────────────────────────────────────────────────── */
    if (vkCreateWin32SurfaceKHR(*inst, nullptr, &g_alloc, surface) != VK_SUCCESS) {
        fprintf(stderr, "vkCreateWin32SurfaceKHR failed\n");
        return false;
    }

    /* ── Physical device ────────────────────────────────────────────── */
    VkPhysicalDevice phys_dev = VK_NULL_HANDLE;
    if (vkEnumeratePhysicalDevices(*inst, &count, nullptr) != VK_SUCCESS || count == 0) {
        fprintf(stderr, "no vulkan physical devices\n");
        return false;
    }
    vkEnumeratePhysicalDevices(*inst, &count, &phys_dev);
    *phys = phys_dev;

    /* ── Queue family ───────────────────────────────────────────────── */
    uint32_t qf_count = 0;
    VkQueueFamilyProperties* qfps = nullptr;
    vkGetPhysicalDeviceQueueFamilyProperties(phys_dev, &qf_count, nullptr);
    qfps = (VkQueueFamilyProperties*)malloc(qf_count * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(phys_dev, &qf_count, qfps);

    int qf_index = -1;
    for (uint32_t i = 0; i < qf_count; ++i) {
        if (qfps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            qf_index = (int)i;
            break;
        }
    }
    free(qfps);
    if (qf_index < 0) { fprintf(stderr, "no graphics queue family\n"); return false; }

    float q_priority = 0.f;
    VkDeviceQueueCreateInfo qci = {};
    qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    qci.queueFamilyIndex = (uint32_t)qf_index;
    qci.queueCount = 1;
    qci.pQueuePriorities = &q_priority;

    /* ── Device ─────────────────────────────────────────────────────── */
    const char* deps[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    VkDeviceCreateInfo di = {};
    di.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    di.queueCreateInfoCount = 1;
    di.pQueueCreateInfos = &qci;
    di.enabledExtensionCount = (uint32_t)sizeof(deps)/sizeof(deps[0]);
    di.ppEnabledExtensionNames = deps;

    if (vkCreateDevice(phys_dev, &di, &g_alloc, dev) != VK_SUCCESS) {
        fprintf(stderr, "vkCreateDevice failed\n");
        return false;
    }
    vkGetDeviceQueue(*dev, qf_index, 0, queue);

    /* ── Render pass (simplified single-subpass) ────────────────────── */
    VkAttachmentDescription att = {};
    att.format = VK_FORMAT_B8G8R8A8_UNORM;
    att.samples = VK_SAMPLE_COUNT_1_BIT;
    att.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    att.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference ref = {};
    ref.attachment = 0;
    ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription sub = {};
    sub.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sub.colorAttachmentCount = 1;
    sub.pColorAttachments = &ref;

    VkRenderPassCreateInfo rpi = {};
    rpi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpi.attachmentCount = 1;
    rpi.pAttachments = &att;
    rpi.subpassCount = 1;
    rpi.pSubpasses = &sub;
    if (vkCreateRenderPass(*dev, &rpi, &g_alloc, rp) != VK_SUCCESS) {
        fprintf(stderr, "vkCreateRenderPass failed\n");
        return false;
    }

    /* ── Descriptor pool ────────────────────────────────────────────── */
    VkDescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 10 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 10 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 10 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10 },
    };
    VkDescriptorPoolCreateInfo pdci = {};
    pdci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pdci.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pdci.poolSizeCount = (uint32_t)sizeof(pool_sizes)/sizeof(pool_sizes[0]);
    pdci.pPoolSizes = pool_sizes;
    pdci.maxSets = 1000;
    if (vkCreateDescriptorPool(*dev, &pdci, &g_alloc, pool) != VK_SUCCESS) {
        fprintf(stderr, "vkCreateDescriptorPool failed\n");
        return false;
    }

    return true;
}

/* ── Main ────────────────────────────────────────────────────────────────── */
int main(int argc, char** argv) {
    /* ── GLFW ─────────────────────────────────────────────────────────── */
    if (!glfwInit()) {
        fprintf(stderr, "glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(WIDTH, HEIGHT, "Litt Engine GUI", nullptr, nullptr);
    if (!window) {
        fprintf(stderr, "glfwCreateWindow failed\n");
        glfwTerminate();
        return 1;
    }

    /* ── Vulkan ───────────────────────────────────────────────────────── */
    VkInstance inst = VK_NULL_HANDLE;
    VkPhysicalDevice phys = VK_NULL_HANDLE;
    VkDevice dev = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkRenderPass rp = VK_NULL_HANDLE;
    VkDescriptorPool pool = VK_NULL_HANDLE;

    if (!init_vulkan(window, &inst, &phys, &dev, &queue, &surface, &rp, &pool)) {
        fprintf(stderr, "Vulkan init failed\n");
        return 1;
    }

    /* ── ImGui ────────────────────────────────────────────────────────── */
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = inst;
    init_info.PhysicalDevice = phys;
    init_info.Device = dev;
    init_info.Queue = queue;
    init_info.DescriptorPool = pool;
    init_info.RenderPass = rp;
    init_info.SuballocationTracker = nullptr;
    init_info.MinImageCount = 2;
    init_info.ImageCount = 2;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = &g_alloc;
    init_info.CheckVkResultFn = nullptr;
    ImGui_ImplVulkan_Init(&init_info);
    ImGui_ImplVulkan_CreateFontsTexture();

    /* ── Bridge ───────────────────────────────────────────────────────── */
    bridge = new LittGuiBridge();
    bridge->connect("127.0.0.1", 8080);
    bridge->loadWorld(nullptr, nullptr);

    /* ── Render loop ──────────────────────────────────────────────────── */
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Dashboard
        ImGui::Begin("Dashboard");
        ImGui::Text("Litt Engine GUI v%s", bridge->getVersion());
        ImGui::Separator();
        
        // State
        LittEngineState state = bridge->getState();
        const char* state_name = "";
        ImU32 state_color = IM_COL32(200,200,200,255);
        switch (state) {
            case LITT_ENGINE_STATE_DISCONNECTED: 
                state_name = "Disconnected"; 
                state_color = IM_COL32(220,50,50,255); 
                break;
            case LITT_ENGINE_STATE_CONNECTING: 
                state_name = "Connecting..."; 
                state_color = IM_COL32(220,220,50,255); 
                break;
            case LITT_ENGINE_STATE_CONNECTED: 
                state_name = "Connected"; 
                state_color = IM_COL32(50,200,50,255); 
                break;
            case LITT_ENGINE_STATE_RUNNING: 
                state_name = "Running"; 
                state_color = IM_COL32(50,150,220,255); 
                break;
            case LITT_ENGINE_STATE_PAUSED: 
                state_name = "Paused"; 
                state_color = IM_COL32(220,130,50,255); 
                break;
            case LITT_ENGINE_STATE_ERROR: 
                state_name = "Error"; 
                state_color = IM_COL32(220,50,50,255); 
                break;
        }
        ImGui::Text("State: ");
        ImGui::TextColored(ImColor(state_color), state_name);
        ImGui::SameLine();
        if (ImGui::Button("Connect")) {
            bridge->connect("127.0.0.1", 8080);
        }
        ImGui::SameLine();
        if (ImGui::Button("Disconnect")) {
            bridge->disconnect();
        }
        ImGui::End();

        // Entities
        ImGui::Begin("Entities");
        if (ImGui::Button("Create Entity")) {
            LittEntityDesc desc = {};
            snprintf(desc.name, sizeof(desc.name), "entity_%u", (unsigned)bridge->listEntities(nullptr, 0));
            desc.position.x = 0.0f;
            desc.position.y = 0.0f;
            desc.position.z = 0.0f;
            bridge->createEntity(&desc);
        }
        
        // List entities
        litt_entity_t ids[100];
        int count = bridge->listEntities(ids, 100);
        for (int i = 0; i < count; i++) {
            char label[128];
            snprintf(label, sizeof(label), "Entity %u", (unsigned)ids[i]);
            if (ImGui::Button(label)) {
                // Select entity
            }
        }
        ImGui::End();

        // Properties
        ImGui::Begin("Properties");
        LittCamera cam;
        bridge->getCamera(&cam);
        ImGui::SliderFloat("FOV", &cam.fov, 30.0f, 120.0f);
        ImGui::SliderFloat("Exposure", &cam.exposure, 0.1f, 3.0f);
        bridge->setCamera(&cam);
        ImGui::End();

        // Render
        ImGui::Render();
        VkRenderPassBeginInfo rpbi = {};
        rpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpbi.renderPass = rp;
        rpbi.renderArea.extent.width = WIDTH;
        rpbi.renderArea.extent.height = HEIGHT;
        rpbi.clearValueCount = 1;
        rpbi.pClearValues = nullptr;
        vkCmdBeginRenderPass(ImGui::GetDrawData()->CommandBuffers[0], &rpbi, VK_SUBPASS_CONTENTS_INLINE);
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData());
        vkCmdEndRenderPass(ImGui::GetDrawData()->CommandBuffers[0]);
        vkQueuePresentKHR(queue, nullptr);
    }

    /* ── Cleanup ──────────────────────────────────────────────────────── */
    bridge->disconnect();
    delete bridge;
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    vkDestroyDescriptorPool(dev, pool, &g_alloc);
    vkDestroyRenderPass(dev, rp, &g_alloc);
    vkDestroyDevice(dev, &g_alloc);
    vkDestroySurfaceKHR(inst, surface, &g_alloc);
    vkDestroyInstance(inst, &g_alloc);
    glfwTerminate();
    return 0;
}
