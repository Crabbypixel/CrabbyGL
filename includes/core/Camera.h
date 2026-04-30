#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "rendering/Shader.h"

class Camera
{
public:
	enum class CameraMovement
	{
		FORWARD,
		BACKWARD,
		LEFT,
		RIGHT,
		UP,
		DOWN
	};

	glm::vec3 position;
	glm::vec3 front;
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
	float fLastX;
	float fLastY;

	static float CAMERA_FAST_SPEED;
	static float CAMERA_NORMAL_SPEED;

private:
	float fPitch = 0.0f;
	float fYaw = -90.0f;
	float fFov = 80.0f;

	glm::mat4 matView;

public:
	float fCameraSpeed = 5.0f;

	Camera() = default;	

	void init(glm::vec3 vPos, glm::vec3 vFront, int screenWidth, int screenHeight);
	const glm::mat4& getLookAt();

	// We essentially change vCamPos using vCamFront and vCamUp
	void ProcessKeyboard(CameraMovement movement, float fDeltaTime);

	// We essentially change vCamDir using pitch and yaw values
	void ProcessMouse(float fMousePosX, float fMousePosY, int screenWidth, int screenHeight, bool bFirstMouse);

	void SetCameraPos(glm::vec3 vPos);

	void UpdateView(Shader shader, const std::string& viewMat4ID);
};