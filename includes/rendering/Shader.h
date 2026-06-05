#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

class Shader
{
public:
	Shader() = default;

	// Non-copyable, non-movable
	Shader(const Shader&) = delete;
	Shader(Shader&&) = delete;
	Shader& operator=(const Shader&) = delete;
	Shader& operator=(Shader&&) = delete;

	void load(const std::string& shaderPath);

	void use() const noexcept;

	[[nodiscard]]
	unsigned int getID() const noexcept { return m_id; }

	void setBool(const std::string& name, bool value) noexcept;
	void setInt(const std::string& name, int value) noexcept;
	void setFloat(const std::string& name, float value) noexcept;
	void setMat4(const std::string& name, const glm::mat4& mat) noexcept;
	void setVec3(const std::string& name, const float& f1, const float& f2, const float& f3) noexcept;
	void setVec3(const std::string& name, const glm::vec3& vec) noexcept;
	void setIvec3(const std::string& name, const glm::ivec3& ivec) noexcept;
	void setIvec3(const std::string& name, const int& i1, const int& i2, const int& i3) noexcept;
	void setVec2(const std::string& name, const float& f1, const float& f2) noexcept;
	void setVec2(const std::string& name, const glm::vec2& vec) noexcept;
	void setIvec2(const std::string& name, const glm::ivec2& ivec) noexcept;
	void setIvec2(const std::string& name, const int& i1, const int& i2) noexcept;
	void setVec4(const std::string& name, const glm::vec4& vec) noexcept;
	void setVec4(const std::string& name, const float& f1, const float& f2, const float& f3, const float& f4) noexcept;
	void setIvec4(const std::string& name, const glm::ivec4& vec) noexcept;
	void setIvec4(const std::string& name, const int& f1, const int& f2, const int& f3, const int& f4) noexcept;

	~Shader();

private:
	unsigned int m_id = 0;
	std::unordered_map<std::string, int> m_uniformLocationCache;
	unsigned int GetUniformLocation(const std::string& name) noexcept;
	unsigned int CompileShader(unsigned int type, const std::string& source, const std::string& shaderPath);
};