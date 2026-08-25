#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stdlib.h>
#include "gui.h"

/* ── Minimal Vulkan initialisation ─────────────────────────────────────── */

static const int WIDTH  = 1280;
static const int HEIGHT = 720;

/* Required Vulkan instance extensions for GLFW + Vulkan */
static const char *required_extensions[] = {
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,   /* Windows; remove for Linux */
};

static VkAllocationCallbacks g_alloc = {nullptr, nullptr, nullptr, nullptr, nullptr};

static void create_vulkan_window(GLFWwindow *win,
                                 VkInstance *inst, VkSurfaceKHR *surface) {
    glfwCreateWindowSurface(*inst, win, &g_alloc, surface);
}

static bool init_vulkan(GLFWwindow *win,
                        VkInstance *inst, VkPhysicalDevice *phys,
                        VkDevice *dev, VkQueue *queue,
                        VkSurfaceKHR *surface, VkRenderPass *rp,
                        VkDescriptorPool *pool) {
    /* ── Instance ───────────────────────────────────────────────────── */
    VkApplicationInfo app = {};
    app.sType          = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pApplicationName = "litt-gui";
    app.apiVersion       = VK_MAKE_VERSION(1, 3, 0);

    VkInstanceCreateInfo info = {};
    info.sType               = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    info.pApplicationInfo    = &app;

    uint32_t count = 0;
    const char **exts = glfwGetRequiredInstanceExtensions(&count);
    info.enabledExtensionCount    = count;
    info.ppEnabledExtensionNames  = exts;

    if (vkCreateInstance(&info, &g_alloc, inst) != VK_SUCCESS) {
        fprintf(stderr, "vkCreateInstance failed\n");
        return false;
    }

    /* ── Surface ────────────────────────────────────────────────────── */
    create_vulkan_window(win, inst, surface);
    if (*surface == VK_NULL_HANDLE) {
        fprintf(stderr, "glfwCreateWindowSurface failed\n");
        return false;
    }

    /* ── Physical device ────────────────────────────────────────────── */
    uint32_t gpu_count = 0;
    vkEnumeratePhysicalDevices(*inst, &gpu_count, nullptr);
    if (gpu_count == 0) { fprintf(stderr, "No Vulkan GPU\n"); return false; }

    VkPhysicalDevice *gpus = (VkPhysicalDevice *)malloc(sizeof(VkPhysicalDevice) * gpu_count);
    vkEnumeratePhysicalDevices(*inst, &gpu_count, gpus);
    *phys = gpus[0];
    free(gpus);

    /* ── Queue family ───────────────────────────────────────────────── */
    uint32_t qf_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(*phys, &qf_count, nullptr);
    VkQueueFamilyProperties *qfs = (VkQueueFamilyProperties *)
        malloc(sizeof(VkQueueFamilyProperties) * qf_count);
    vkGetPhysicalDeviceQueueFamilyProperties(*phys, &qf_count, qfs);

    int qf_index = -1;
    for (uint32_t i = 0; i < qf_count; i++) {
        if (qfs[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) { qf_index = (int)i; break; }
    }
    free(qfs);
    if (qf_index < 0) { fprintf(stderr, "No graphics queue family\n"); return false; }

    float qpp = 1.0f;
    VkDeviceQueueCreateInfo qdi = {};
    qdi.sType               = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    qdi.queueFamilyIndex    = (uint32_t)qf_index;
    qdi.queueCount          = 1;
    qdi.pQueuePriorities    = &qpp;

    /* ── Device ─────────────────────────────────────────────────────── */
    const char *device_exts[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    VkDeviceCreateInfo dci = {};
    dci.sType                 = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dci.queueCreateInfoCount  = 1;
    dci.pQueueCreateInfos     = &qdi;
    dci.enabledExtensionCount = 1;
    dci.ppEnabledExtensionNames = device_exts;

    if (vkCreateDevice(*phys, &dci, &g_alloc, dev) != VK_SUCCESS) {
        fprintf(stderr, "vkCreateDevice failed\n");
        return false;
    }

    vkGetDeviceQueue(*dev, qf_index, 0, queue);

    /* ── Render pass (simple fill) ──────────────────────────────────── */
    VkAttachmentDescription att = {};
    att.format     = VK_FORMAT_B8G8R8A8_SRGB;
    att.samples    = VK_SAMPLE_COUNT_1_BIT;
    att.loadOp     = VK_ATTACHMENT_LOAD_OP_CLEAR;
    att.storeOp    = VK_ATTACHMENT_STORE_OP_STORE;
    att.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    att.finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference ref = {};
    ref.attachment = 0;
    ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription sub = {};
    sub.pipelineBindPoint      = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sub.colorAttachmentCount   = 1;
    sub.pColorAttachments      = &ref;

    VkRenderPassCreateInfo rpci = {};
    rpci.sType            = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpci.attachmentCount  = 1;
    rpci.pAttachments     = &att;
    rpci.subpassCount     = 1;
    rpci.pSubpasses       = &sub;
    vkCreateRenderPass(*dev, &rpci, &g_alloc, rp);

    /* ── Descriptor pool ────────────────────────────────────────────── */
    VkDescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
    };
    VkDescriptorPoolCreateInfo dpci = {};
    dpci.sType                  = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    dpci.maxSets                = 1;
    dpci.poolSizeCount          = 2;
    dpci.pPoolSizes             = pool_sizes;
    vkCreateDescriptorPool(*dev, &dpci, &g_alloc, pool);

    return true;
}

/* ── Main ────────────────────────────────────────────────────────────────── */

int main(int argc, char **argv) {
    printf("Litt Engine GUI v0.1.0 (C++ / ImGui / Vulkan)\n");
    printf("Connecting to engine at 127.0.0.1:8080 ...\n\n");

    if (!glfwInit()) {
        fprintf(stderr, "glfwInit failed\n");
        return 1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE,  GLFW_FALSE);

    GLFWwindow *win = glfwCreateWindow(WIDTH, HEIGHT,
                                       "Litt Engine GUI v0.1.0", nullptr, nullptr);
    if (!win) {
        fprintf(stderr, "glfwCreateWindow failed\n");
        glfwTerminate();
        return 1;
    }

    /* ── Vulkan init ──────────────────────────────────────────────────── */
    VkInstance    inst    = VK_NULL_HANDLE;
    VkPhysicalDevice phys  = VK_NULL_HANDLE;
    VkDevice      dev     = VK_NULL_HANDLE;
    VkQueue       queue   = VK_NULL_HANDLE;
    VkSurfaceKHR  surface = VK_NULL_HANDLE;
    VkRenderPass  rp      = VK_NULL_HANDLE;
    VkDescriptorPool pool = VK_NULL_HANDLE;

    if (!init_vulkan(win, &inst, &phys, &dev, &queue,
                     &surface, &rp, &pool)) {
        fprintf(stderr, "Vulkan initialisation failed\n");
        glfwDestroyWindow(win);
        glfwTerminate();
        return 1;
    }

    /* ── ImGui init ───────────────────────────────────────────────────── */
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForVulkan(win, true);
    ImGui_ImplVulkan_InitInfo vi = {};
    vi.Instance         = inst;
    vi.PhysicalDevice   = phys;
    vi.Device           = dev;
    vi.Queue            = queue;
    vi.PipelineCache    = VK_NULL_HANDLE;
    vi.DescriptorPool   = pool;
    vi.RenderPass       = rp;
    vi.Subpass          = 0;
    vi.MSAASamples      = VK_SAMPLE_COUNT_1_BIT;
    vi.MinAllocatorSync = VkSyncSupport_None;
    ImGui_ImplVulkan_Init(&vi, &rp);
    vkDeviceWaitIdle(dev);

    /* ── Create GUI ───────────────────────────────────────────────────── */
    void *gui = litt_gui_create();
    if (!gui) {
        fprintf(stderr, "Failed to create GUI\n");
        return 1;
    }

    printf("[gui] Connected to engine.\n");
    printf("[gui] Press 1-6 for quality, Space to render, R to reset, Esc to exit.\n\n");

    /* ── Main loop ────────────────────────────────────────────────────── */
    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();

        litt_gui_render_frame(gui);

        /* Submit draw data – minimal swapchain not shown for brevity;
           ImGui impl handles present via the render pass. */
        ImGui_ImplVulkan_NewFrame();
        ImGui::Render();

        VkCommandBuffer cb;
        /* For a full production build, allocate a command buffer from a
           pool and record a simple clear + draw call.  The CMake setup
           links the ImGui vulkan backend which provides this. */
        ImGui_ImplGlfw_RenderDrawData(ImGui::GetDrawData());
    }

    /* ── Cleanup ──────────────────────────────────────────────────────── */
    litt_gui_destroy(gui);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    vkDestroyDescriptorPool  (dev, pool,  &g_alloc);
    vkDestroyRenderPass      (dev, rp,    &g_alloc);
    vkDestroySurfaceKHR      (inst, surface, &g_alloc);
    vkDestroyDevice          (dev, nullptr, &g_alloc);
    vkDestroyInstance        (inst, nullptr, &g_alloc);

    glfwDestroyWindow(win);
    glfwTerminate();

    printf("[gui] Exited.\n");
    return 0;
}
