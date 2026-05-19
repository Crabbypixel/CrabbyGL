#include "rendering/VertexArray.h"

void VertexArray::generate() noexcept
{
	glGenVertexArrays(1, &m_VertexArrayID);
	glBindVertexArray(m_VertexArrayID);
}

void VertexArray::bind() const noexcept
{
	glBindVertexArray(m_VertexArrayID);
}

void VertexArray::unbind() const noexcept
{
	glBindVertexArray(0);
}

VertexArray::~VertexArray() noexcept
{
	if (m_VertexArrayID != 0)
		glDeleteVertexArrays(1, &m_VertexArrayID);
}