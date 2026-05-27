#include "core/OpenGL_3D.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

void OpenGL_3D::RendererThread()
{
	// We want the window to be in the renderer thread context (important!)
	glfwMakeContextCurrent(window);

	float fAccumulatedTime = 0.0f;
	int iFrameCount = 0;

	if (!Setup())
		m_bIsRunning = false;

	// Update projection matrix
	UpdateProjectionMatrix();

	auto dt1 = std::chrono::system_clock::now();
	auto dt2 = std::chrono::system_clock::now();

	// Run as fast as possible
	while (m_bIsRunning)
	{
		// FPS calculation
		dt2 = std::chrono::system_clock::now();
		std::chrono::duration<float> elapsedTime = dt2 - dt1;
		dt1 = dt2;

		float fElapsedTime = elapsedTime.count();
		fTimeSinceStart += fElapsedTime;

		// Capture scroll atomic once per frame into a stable snapshot
		// This also resets the atomic to 0, so the main thread can update it for the next frame without worrying about synchronization
		m_mouseScrollFrame = m_mouseScroll.exchange(0, std::memory_order_relaxed);

		if (m_keySwapReady.exchange(false, std::memory_order_acquire))
			std::swap(m_keyRaw, m_keyRawPending);		// Swap the raw state buffers when the main thread signals a fresh snapshot is ready

		// Update key and mouse states on each frame, they may be used later by the programmer
		// 1. Update key states
		for (int i = 0; i < MAX_KEYS; ++i)
		{
			m_keyNewState[i] = m_keyRaw[i];		// Use the raw state captured by the main thread callback

			m_keys[i].bPressed = false;
			m_keys[i].bReleased = false;

			if (m_keyNewState[i] != m_keyOldState[i])
			{
				if (m_keyNewState[i])
				{
					m_keys[i].bPressed = !m_keys[i].bHeld;
					m_keys[i].bHeld = true;
				}
				else
				{
					m_keys[i].bReleased = true;
					m_keys[i].bHeld = false;
				}
			}

			m_keyOldState[i] = m_keyNewState[i];
		}

		// 2. Update mouse states
		for (int i = 0; i < MAX_MOUSE_BUTTONS; ++i)
		{
			// Drain the atomic button held states into the new state array for processing
			m_mouseNewState[i] = m_bMouseButtonHeld[i].load(std::memory_order_relaxed);

			m_mouse[i].bPressed = false;
			m_mouse[i].bReleased = false;

			if (m_mouseNewState[i] != m_mouseOldState[i])
			{
				if (m_mouseNewState[i])
				{
					m_mouse[i].bPressed = !m_mouse[i].bHeld;
					m_mouse[i].bHeld = true;
				}
				else
				{
					m_mouse[i].bReleased = true;
					m_mouse[i].bHeld = false;
				}
				m_mouseOldState[i] = m_mouseNewState[i];
			}
		}

		// Pause/resume the renderer
		if (GetKey(GLFW_KEY_P).bPressed)
		{
			bIsPaused = !bIsPaused;

			if (bIsPaused)
			{
				shouldUpdateCamera = false;

				std::cout << "Engine: paused\n";
			}
			else
			{
				shouldUpdateCamera = true;

				camera.fLastX = (float)GetMousePosX();
				camera.fLastY = (float)GetMousePosY();
				std::cout << "Engine: unpaused\n";
			}
		}

		if (!Update(fElapsedTime))
		{
			m_bIsRunning = false;
		}

		// Control inputs - Change Projection and View matrices & handle keyboard inputs
		UpdateCameraControls(fElapsedTime);

		// FPS calculation
		++iFrameCount;
		fAccumulatedTime += fElapsedTime;

		// Update FPS every 0.5 seconds
		if (fAccumulatedTime >= 0.5f)
		{
			int fps = (int)(iFrameCount / fAccumulatedTime);

			if (window)
			{
				char s[32];
				snprintf(s, 32, "%s : %d FPS", m_sAppName.c_str(), fps);
				glfwSetWindowTitle(window, s);
			}

			fAccumulatedTime = 0.0f;
			iFrameCount = 0;
		}

		// Swap buffers
		glfwSwapBuffers(window);
	}

	Destroy();

	// Give the window context back to the main thread
	glfwMakeContextCurrent(nullptr);
}

void OpenGL_3D::UpdateCameraControls(float fElapsedTime)
{
	if (shouldUpdateCamera)
	{
		if (GetKey('C').bHeld)
		{
			if (fFov > 10.0f)
				fFov -= fElapsedTime * 200.0f;
			UpdateProjectionMatrix();
		}

		// Zoom out if the 'C' key is released
		else if (GetKey('C').bReleased)
		{
			fFov = 80.0f;
			UpdateProjectionMatrix();
		}

		if (GetKey(GLFW_KEY_HOME).bPressed)
			camera.Init(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, -1.0f), ScreenWidth(), ScreenHeight());

		/* ------------------------------------------ - Mouse Control - ------------------------------------------ */
		camera.ProcessMouse(GetMousePosX(), GetMousePosY(), ScreenWidth(), ScreenHeight(), bFirstMouse);
	}

	if (!shouldUpdateCamera && !bIsPaused)
		camera.ProcessMouse(camera.fLastX, camera.fLastY, ScreenWidth(), ScreenHeight(), bFirstMouse);

	UpdateViewMatrix();
}

void OpenGL_3D::UpdateProjectionMatrix()
{
	matProjection = glm::perspective(fFov * pi / 180.0f, (float)ScreenWidth() / (float)ScreenHeight(), 0.1f, 1000.0f);
	glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(matProjection));
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void OpenGL_3D::UpdateViewMatrix()
{
	glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(camera.getLookAt()));
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void OpenGL_3D::ConstructWindow(int width, int height, std::string windowName)
{
	m_sAppName = windowName;
	m_width = width;
	m_height = height;

	// Initialize GLFW and initialize OpenGL to version 3.3
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Make the window non-resizable
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	glfwWindowHint(GLFW_SAMPLES, 4);			// MSAA
	//glfwWindowHint(GLFW_DEPTH_BITS, 24);		// Request a 32-bit depth buffer

	// Create a window
	window = glfwCreateWindow(m_width, m_height, m_sAppName.c_str(), NULL, NULL);
	if (window == NULL)
		Error("Failed to create window.");

	// Set window position on screen
	glfwSetWindowPos(window, 360, 75);

	// Make the window to in the current context
	glfwMakeContextCurrent(window);

	// Disable cursor
	glfwSetCursorPos(window, m_width / 2.0f, m_height / 2.0f);

	// Disable V-Sync (to achieve 60+ fps)
	// Comment this out to get 60 fps (max)
	glfwSwapInterval(0);

	// Load all function pointers using GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		Error("Failed to initialize GLAD");

	// Set viewport and callback function when window gets resized 
	glViewport(0, 0, m_width, m_height);

	// Enable z-buffer
	glEnable(GL_DEPTH_TEST);          // Enable depth testing
	glDepthFunc(GL_LESS);

	// Enable multi-sampling (usually enabled, good to enable it ourselves anyways)
	glEnable(GL_MULTISAMPLE);

	// Make the image loading library flip textures on load by default since 
	// OpenGL's texture coordinate system has the y-axis going 
	// upwards, while images usually have it downwards
	stbi_set_flip_vertically_on_load(true);

	// Display GPU info
	DisplayGPU();
}

void OpenGL_3D::Start()
{
	m_bIsRunning = true;

	glfwSetWindowUserPointer(window, this);

	// Set callbacks
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	// Make the window context null before calling the renderer thread
	glfwMakeContextCurrent(nullptr);

	// Start the renderer
	std::thread rendererThread = std::thread(&OpenGL_3D::RendererThread, this);

	// While the renderer thread is running, the main thread handles poll events
	while (m_bIsRunning)
	{
		// Check for "esc" key press
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		{
			m_bIsRunning = false;
			glfwSetWindowShouldClose(window, true);
		}

		// Set cursor mode
		glfwSetInputMode(window, GLFW_CURSOR, m_cursorVisible.load(std::memory_order_relaxed) ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
		
		// Initiate shutdown when window is closed
		if (glfwWindowShouldClose(window))
			m_bIsRunning = false;

		glfwPollEvents();
		PollKeys();
	}

	// Wait until the renderer thread exits
	if (rendererThread.joinable())
		rendererThread.join();

	// Cleanup functions
	glfwDestroyWindow(window);
	glfwTerminate();
}

void OpenGL_3D::ErrorLog(const std::string& str)
{
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR)
	{
		std::cerr << "OpenGL error - main: " << err << std::endl;
		std::cerr << "in: " << str << '\n';
	}
}

void OpenGL_3D::Error(const std::string& message)
{
	std::cerr << "Error: " << message << std::endl;
	Destroy();
	glfwTerminate();
	exit(EXIT_FAILURE);
}

void OpenGL_3D::PollKeys()
{
	// Called from main thread - defined behavior
	for (int i = 0; i < MAX_KEYS; ++i)
		m_keyRawPending[i] = (glfwGetKey(window, i) == GLFW_PRESS);

	// Signal renderer that a fresh snapshot is ready for swapping
	m_keySwapReady.store(true, std::memory_order_release);
}

void OpenGL_3D::mouse_callback(GLFWwindow* window, double xPos, double yPos)
{
	OpenGL_3D* instance = static_cast<OpenGL_3D*>(glfwGetWindowUserPointer(window));

	instance->m_mousePosX = (float)xPos;
	instance->m_mousePosY = (float)yPos;
	instance->bFirstMouse = false;
}

void OpenGL_3D::scroll_callback(GLFWwindow* window, double xOffset, double yOffset)
{
	OpenGL_3D* instance = static_cast<OpenGL_3D*>(glfwGetWindowUserPointer(window));

	if ((int)yOffset == 1)
		instance->m_mouseScroll.store((int)Mouse::SCROLL_UP, std::memory_order_relaxed);
	else if ((int)yOffset == -1)
		instance->m_mouseScroll.store((int)Mouse::SCROLL_DOWN, std::memory_order_relaxed);
}

void OpenGL_3D::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	OpenGL_3D* instance = static_cast<OpenGL_3D*>(glfwGetWindowUserPointer(window));
	if (button < MAX_MOUSE_BUTTONS)
		instance->m_bMouseButtonHeld[button].store(action != GLFW_RELEASE, std::memory_order_relaxed);
}

void OpenGL_3D::DisplayGPU()
{
	const char* renderer = (const char*)glGetString(GL_RENDERER);
	const char* vendor = (const char*)glGetString(GL_VENDOR);
	const char* version = (const char*)glGetString(GL_VERSION);
	const char* glslVersion = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);

	std::cout << "========== GPU INFORMATION ==========\n\n";

	// Basic Info
	std::cout << "[Core]\n";
	std::cout << "Renderer        : " << renderer << '\n';
	std::cout << "Vendor          : " << vendor << '\n';
	std::cout << "OpenGL Version  : " << version << '\n';
	std::cout << "GLSL Version    : " << glslVersion << "\n\n";

	// Check if OpenGL context exists
	if (!renderer)
	{
		std::cout << "[ERROR] OpenGL not initialized!\n";
		return;
	}

	// Limits
	GLint maxTextureSize;
	GLint maxVertexAttribs;
	GLint maxUniforms;
	GLint maxDrawBuffers;

	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);
	glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &maxUniforms);
	glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);

	std::cout << "[Limits]\n";
	std::cout << "Max Texture Size       : " << maxTextureSize << '\n';
	std::cout << "Max Vertex Attribs     : " << maxVertexAttribs << '\n';
	std::cout << "Max Vertex Uniforms    : " << maxUniforms << '\n';
	std::cout << "Max Draw Buffers       : " << maxDrawBuffers << "\n\n";

	// Compute Shader Support
	GLint major = 0, minor = 0;
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);

	bool computeSupported = (major > 4 || (major == 4 && minor >= 3));

	std::cout << "[Features]\n";
	std::cout << "Compute Shader Support : " << (computeSupported ? "YES" : "NO") << '\n';

	GLint workGroupCount[3];
	GLint workGroupSize[3];

	// VRAM (NVIDIA)
#ifdef GL_NVX_gpu_memory_info
	GLint totalMemKB = 0;
	GLint availMemKB = 0;

	glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &totalMemKB);
	glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &availMemKB);

	std::cout << "[Memory - NVIDIA]\n";
	std::cout << "Total VRAM (MB)        : " << totalMemKB / 1024 << '\n';
	std::cout << "Available VRAM (MB)    : " << availMemKB / 1024 << "\n\n";
#endif

#ifdef GL_ATI_meminfo
	GLint memInfo[4];
	glGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, memInfo);

	std::cout << "[Memory - AMD]\n";
	std::cout << "Free Texture Memory MB : " << memInfo[0] / 1024 << "\n\n";
#endif

	// Extensions count
	GLint numExtensions = 0;
	glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);

	std::cout << "[Extensions]\n";
	std::cout << "Total Extensions       : " << numExtensions << '\n';

	std::cout << "\n=====================================\n\n";
}

OpenGL_3D::~OpenGL_3D()
{
	glDeleteBuffers(1, &uboMatrices);
	std::cout << "Destructor called" << std::endl;
}