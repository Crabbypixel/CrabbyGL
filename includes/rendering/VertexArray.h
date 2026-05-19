#pragma once

#include <glad/glad.h>

class VertexArray
{
private:
	unsigned int m_VertexArrayID = 0;

public:
	VertexArray() = default;

	void generate() noexcept;

	void bind() const noexcept;

	void unbind() const noexcept;

	~VertexArray() noexcept;
};