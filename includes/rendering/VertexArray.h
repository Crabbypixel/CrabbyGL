#pragma once

#include <glad/glad.h>

class VertexArray
{
private:
	unsigned int m_VertexArrayID = 0;

public:
	VertexArray() = default;

	void generate();

	void bind() const;

	void unbind() const;

	void free() const;
};