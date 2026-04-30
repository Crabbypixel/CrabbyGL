#include "rendering/Shader.h"

void Shader::load(const std::string& shaderPath)
{
	bool isGeometryShaderPresent = false;
	std::ifstream stream(shaderPath);

	enum class ShaderType
	{
		NONE = -1,
		VERTEX = 0,
		FRAGMENT = 1,
		GEOMETRY = 2
	};

	std::string line;
	ShaderType type = ShaderType::NONE;
	std::stringstream ss[3];

	while (getline(stream, line))
	{
		if (line.find("SHADER") != std::string::npos)
		{
			if (line.find("VERTEX") != std::string::npos)
				type = ShaderType::VERTEX;
			else if (line.find("FRAGMENT") != std::string::npos)
				type = ShaderType::FRAGMENT;
			else if (line.find("GEOMETRY") != std::string::npos)
			{
				type = ShaderType::GEOMETRY;
				isGeometryShaderPresent = true;
			}
		}

		else
		{
			ss[(int)type] << line << '\n';
		}
	}

	std::string vertexShader = "#version 330 core\n #define SHADER_VERTEX\n #ifdef SHADER_VERTEX\n" + ss[(int)ShaderType::VERTEX].str();
	std::string fragmentShader = "#version 330 core\n #define SHADER_FRAGMENT\n #ifdef SHADER_FRAGMENT\n" + ss[(int)ShaderType::FRAGMENT].str();
	std::string geometryShader;

	if (isGeometryShaderPresent)
		geometryShader = "#version 330 core\n #define SHADER_GEOMETRY\n #ifdef SHADER_GEOMETRY\n" + ss[(int)ShaderType::GEOMETRY].str();

	// Compile shaders
	unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader, shaderPath);
	unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader, shaderPath);
	unsigned int gs;

	if (isGeometryShaderPresent)
		gs = CompileShader(GL_GEOMETRY_SHADER, geometryShader, shaderPath);

	// Link shaders
	id = glCreateProgram();
	glAttachShader(id, vs);
	glAttachShader(id, fs);
	if (isGeometryShaderPresent) glAttachShader(id, gs);

	glLinkProgram(id);
	glValidateProgram(id);

	// Check for linking errors
	int result;
	glGetProgramiv(id, GL_LINK_STATUS, &result);

	if (!result)
	{
		int length;
		glGetProgramiv(id, GL_INFO_LOG_LENGTH, &length);

		char* message = (char*)alloca(length * sizeof(char));

		glGetShaderInfoLog(id, length, &length, message);

		std::cout << "[OpenGL Error] Linking error in \'" << shaderPath << "\'" << std::endl;
		std::cout << "Log: " << message << std::endl;

		glDeleteShader(vs);
		glDeleteShader(fs);

		if (isGeometryShaderPresent)
			glDeleteShader(gs);

		glfwTerminate();
		exit(0);
	}

	glDeleteShader(vs);
	glDeleteShader(fs);

	if (isGeometryShaderPresent)
		glDeleteShader(gs);
}

void Shader::use()
{
	glUseProgram(id);
}

void Shader::setBool(const std::string& name, bool value)
{
	glUniform1i(glGetUniformLocation(id, name.c_str()), (int)value);
}

void Shader::setInt(const std::string& name, int value)
{
	glUniform1i(glGetUniformLocation(id, name.c_str()), value);
}

void Shader::setFloat(const std::string& name, float value)
{
	glUniform1f(glGetUniformLocation(id, name.c_str()), value);
}

void Shader::setMat4(const std::string& name, const glm::mat4& mat)
{
	glUniformMatrix4fv(glGetUniformLocation(id, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::setVec3(const std::string& name, const float& f1, const float& f2, const float& f3)
{
	glUniform3f(glGetUniformLocation(id, name.c_str()), f1, f2, f3);
}

void Shader::setVec3(const std::string& name, const glm::vec3& vec)
{
	glUniform3f(glGetUniformLocation(id, name.c_str()), vec.x, vec.y, vec.z);
}

void Shader::setIvec3(const std::string& name, const glm::ivec3& ivec)
{
	glUniform3i(glGetUniformLocation(id, name.c_str()), ivec.x, ivec.y, ivec.z);
}

void Shader::setIvec3(const std::string& name, const int& i1, const int& i2, const int& i3)
{
	glUniform3i(glGetUniformLocation(id, name.c_str()), i1, i2, i3);
}

void Shader::setVec4(const std::string& name, const glm::vec4& vec)
{
	glUniform4f(glGetUniformLocation(id, name.c_str()), vec.x, vec.y, vec.z, vec.w);
}

void Shader::setVec4(const std::string& name, const float& f1, const float& f2, const float& f3, const float& f4)
{
	glUniform4f(glGetUniformLocation(id, name.c_str()), f1, f2, f3, f4);
}

void Shader::setIvec4(const std::string& name, const glm::ivec4& vec)
{
	glUniform4f(glGetUniformLocation(id, name.c_str()), vec.x, vec.y, vec.z, vec.w);
}

void Shader::setIvec4(const std::string& name, const int& f1, const int& f2, const int& f3, const int& f4)
{
	glUniform4f(glGetUniformLocation(id, name.c_str()), f1, f2, f3, f4);
}

void Shader::setVec2(const std::string& name, const float& f1, const float& f2)
{
	glUniform2f(glGetUniformLocation(id, name.c_str()), f1, f2);
}

void Shader::setVec2(const std::string& name, const glm::vec2& vec)
{
	glUniform2f(glGetUniformLocation(id, name.c_str()), vec.x, vec.y);
}

void Shader::setIvec2(const std::string& name, const glm::ivec2& ivec)
{
	glUniform2i(glGetUniformLocation(id, name.c_str()), ivec.x, ivec.y);
}

void Shader::setIvec2(const std::string& name, const int& i1, const int& i2)
{
	glUniform2i(glGetUniformLocation(id, name.c_str()), i1, i2);
}

// Private utility function - to compile vertex and fragment shader
unsigned int Shader::CompileShader(unsigned int type, const std::string& source, const std::string& shaderPath)
{
	unsigned int shader = glCreateShader(type);
	const char* shaderSource = source.c_str();
	glShaderSource(shader, 1, &shaderSource, NULL);
	glCompileShader(shader);

	int result;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &result);

	if (!result)
	{
		int length;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

		// Generate info log
		char* message = (char*)alloca(length * sizeof(char));
		glGetShaderInfoLog(shader, length, &length, message);

		std::cout << "[OpenGL Error] Failed to compile ";

		if (type == GL_VERTEX_SHADER)
			std::cout << "vertex";
		else if (type == GL_FRAGMENT_SHADER)
			std::cout << "fragment";
		else if (type == GL_GEOMETRY_SHADER)
			std::cout << "geometry";

		std::cout << " shader in \'" << shaderPath << "\'" << std::endl;
		std::cout << "Log: " << message << std::endl;

		glDeleteShader(shader);

		glfwTerminate();
		exit(0);
	}

	return shader;
}