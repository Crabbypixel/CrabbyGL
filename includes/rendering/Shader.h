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

	void load(const std::string& shaderPath);

	Shader& operator=(const Shader&) = delete;

	void use();
	unsigned int getID() { return id; }

	void setBool(const std::string& name, bool value);
	void setInt(const std::string& name, int value);
	void setFloat(const std::string& name, float value);
	void setMat4(const std::string& name, const glm::mat4& mat);
	void setVec3(const std::string& name, const float& f1, const float& f2, const float& f3);
	void setVec3(const std::string& name, const glm::vec3& vec);
	void setIvec3(const std::string& name, const glm::ivec3& ivec);
	void setIvec3(const std::string& name, const int& i1, const int& i2, const int& i3);
	void setVec2(const std::string& name, const float& f1, const float& f2);
	void setVec2(const std::string& name, const glm::vec2& vec);
	void setIvec2(const std::string& name, const glm::ivec2& ivec);
	void setIvec2(const std::string& name, const int& i1, const int& i2);
	void setVec4(const std::string& name, const glm::vec4& vec);
	void setVec4(const std::string& name, const float& f1, const float& f2, const float& f3, const float& f4);
	void setIvec4(const std::string& name, const glm::ivec4& vec);
	void setIvec4(const std::string& name, const int& f1, const int& f2, const int& f3, const int& f4);

private:
	unsigned int id;
	unsigned int CompileShader(unsigned int type, const std::string& source, const std::string& shaderPath);
};