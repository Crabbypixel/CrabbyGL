#pragma once

// IMPORTANT: GLAD must be included BEFORE GLFW
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// GLM (needed for matProjection)
#include <glm/glm.hpp>

#include "core/Camera.h"

#include <string>
#include <atomic>

// Constants
static constexpr int MAX_KEYS = GLFW_KEY_LAST;
static constexpr int MAX_MOUSE_BUTTONS = 3;
constexpr float pi = 3.14159f;

class OpenGL_3D
{
private:
	// Window width and height
	int m_width;
	int m_height;

	// Window title name
	std::string m_sAppName;

	// Arrays to store key states
	short m_keyNewState[MAX_KEYS] = { 0 };
	short m_keyOldState[MAX_KEYS] = { 0 };
	short m_mouseOldState[MAX_MOUSE_BUTTONS] = { 0 };
	short m_mouseNewState[MAX_MOUSE_BUTTONS] = { 0 };

	struct sKeyState
	{
		bool bPressed;
		bool bReleased;
		bool bHeld;
	} m_keys[MAX_KEYS] = {}, m_mouse[MAX_MOUSE_BUTTONS] = {};

	// Mouse variables
	float m_mousePosX;
	float m_mousePosY;
	int m_mouseScroll;
	bool m_bMouseButtonHeld[MAX_MOUSE_BUTTONS] = { false };

	// Atomic variable for running console
	static std::atomic<bool> m_bIsRunning;

protected:
	GLFWwindow* window;

	enum class Mouse
	{
		LEFT = 0,
		RIGHT = 1,
		MIDDLE = 2,
		SCROLL_UP = 3,
		SCROLL_DOWN = 4
	};

	bool bIsPaused = false;

private:
	// Main renderer thread which constantly renders to the screen
	void RendererThread();

	// Update key and mouse states and update camera parameters
	void HandleInputs(float fElapsedTime);

	// Update projection matrix UBOs
	void UpdateProjectionMatrix();

	// Update view matrix using UBOs
	void UpdateViewMatrix();

public:
	float fTimeSinceStart = 0.0f;
	bool bFirstMouse = true;

	Camera camera;
	glm::mat4 matProjection;
	float fFov = 80.0f;

	// Using a Uniform Buffer Object(UBO) to store the projection & view matrices 
	// in VRAM allows multiple shaders to access this matrix directly, 
	// eliminating the need for repeated CPU - GPU calls each time
	// The actual definition of uboMatrices is defined in Main.cpp
	unsigned int uboMatrices;

	int ScreenWidth() const { return m_width; }
	int ScreenHeight() const { return m_height; }
	float GetMousePosX() const { return m_mousePosX; }
	float GetMousePosY() const { return m_mousePosY; }
	Mouse GetMouseScroll() const { return (Mouse)m_mouseScroll; }
	sKeyState GetMouseButton(Mouse button) const { return m_mouse[(int)button]; }
	sKeyState GetKey(int nKeyID) const { return m_keys[nKeyID]; }

	OpenGL_3D() : window(nullptr), m_width(0), m_height(0) {}

	~OpenGL_3D();

	void ConstructWindow(int width, int height, std::string windowName);

	void Start();

	void ErrorLog(const std::string& str = "");

// Virtual functions
protected:
	// Has to be overridden by subclasses
	virtual bool Setup() = 0;
	virtual bool Update(float fElapsedTime) = 0;

	// Optional to override
	virtual void Destroy() {}

// Private functions
private:
	void Error(const std::string& message);
	void DisplayGPU();

	// Callback functions used by GLFW
	static void mouse_callback(GLFWwindow* window, double xPos, double yPos);
	static void scroll_callback(GLFWwindow* window, double xOffset, double yOffset);
	static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
};