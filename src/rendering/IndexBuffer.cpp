#include "rendering/IndexBuffer.h"

void IndexBuffer::generate() noexcept
{
	glGenBuffers(1, &m_IndexBufferID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexBufferID);
}

void IndexBuffer::bind() const noexcept
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexBufferID);
}

void IndexBuffer::unbind() const noexcept
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void IndexBuffer::setBuffer(size_t bytes, const void* data) const noexcept
{
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, bytes, data, GL_STATIC_DRAW);
}

unsigned int IndexBuffer::getID() const noexcept
{
	return m_IndexBufferID;
}

void IndexBuffer::free() const noexcept
{
	if (m_IndexBufferID != 0)
		glDeleteBuffers(1, &m_IndexBufferID);
}
