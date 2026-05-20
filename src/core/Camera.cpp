#include "core/Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

void Camera::Init(glm::vec3 vPos, glm::vec3 vFront, int screenWidth, int screenHeight)
{
	position = vPos;
	fLastX = screenWidth / 2.0f;
	fLastY = screenHeight / 2.0f;

	front = vFront;
	matView = glm::lookAt(position, position + front, up);
}

const glm::mat4& Camera::getLookAt()
{
	return matView;
}

// We essentially change vCamPos using vCamFront and vCamUp
void Camera::ProcessKeyboard(CameraMovement movement, float fDeltaTime)
{
	float fDistance = fCameraSpeed * fDeltaTime;
	switch (movement)
	{
	case CameraMovement::FORWARD:
		position += front * fDistance;
		break;

	case CameraMovement::BACKWARD:
		position -= front * fDistance;
		break;

	case CameraMovement::LEFT:
		position -= glm::normalize(glm::cross(front, up)) * fDistance;
		break;

	case CameraMovement::RIGHT:
		position += glm::normalize(glm::cross(front, up)) * fDistance;
		break;

	case CameraMovement::UP:
		position += up * fDistance;
		break;

	case CameraMovement::DOWN:
		position -= up * fDistance;
		break;
	}

	matView = glm::lookAt(position, position + front, up);
}

// We essentially change vCamDir using pitch and yaw values
void Camera::ProcessMouse(float fMousePosX, float fMousePosY, int screenWidth, int screenHeight, bool bFirstMouse)
{
	if (bFirstMouse)
	{
		fLastX = screenWidth / 2.0f;
		fLastY = screenHeight / 2.0f;
	}

	else
	{
		float fOffsetX = fMousePosX - fLastX;
		float fOffsetY = fLastY - fMousePosY;

		fLastX = fMousePosX;
		fLastY = fMousePosY;

		float fSensitivity = 0.2f;
		fOffsetX *= fSensitivity;
		fOffsetY *= fSensitivity;

		fYaw += fOffsetX;
		fPitch += fOffsetY;

		if (fPitch > 89.9f)
			fPitch = 89.9f;
		else if (fPitch < -89.9f)
			fPitch = -89.9;

		glm::vec3 vDirection;
		vDirection.x = cosf(glm::radians(fYaw)) * cosf(glm::radians(fPitch));
		vDirection.y = sinf(glm::radians(fPitch));
		vDirection.z = sinf(glm::radians(fYaw)) * cosf(glm::radians(fPitch));
		front = glm::normalize(vDirection);
	}

	matView = glm::lookAt(position, position + front, up);
}

void Camera::SetCameraPos(glm::vec3 vPos)
{
	position = vPos;
}