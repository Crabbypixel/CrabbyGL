#include "rendering/IndexBuffer.h"

void IndexBuffer::generate()
{
	glGenBuffers(1, &m_IndexBufferID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexBufferID);
}

void IndexBuffer::bind() const
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexBufferID);
}

void IndexBuffer::unbind() const
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void IndexBuffer::setBuffer(size_t bytes, const void* data) const
{
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, bytes, data, GL_STATIC_DRAW);
}

void IndexBuffer::free() const
{
	glDeleteBuffers(1, &m_IndexBufferID);
}

const unsigned int IndexBuffer::getID() const
{
	return m_IndexBufferID;
}