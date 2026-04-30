#include "rendering/VertexArray.h"

void VertexArray::generate()
{
	glGenVertexArrays(1, &m_VertexArrayID);
	glBindVertexArray(m_VertexArrayID);
}

void VertexArray::bind() const
{
	glBindVertexArray(m_VertexArrayID);
}

void VertexArray::unbind() const
{
	glBindVertexArray(0);
}

void VertexArray::free() const
{
	glDeleteVertexArrays(1, &m_VertexArrayID);
}