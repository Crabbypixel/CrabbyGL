#pragma once

#include <glad/glad.h>

template<typename T = float>
class VertexBuffer
{
private:
	unsigned int m_VertexBufferID = 0;
	size_t m_BufferBytes = 0;
	size_t m_VertexCount = 0;
public:
	size_t typeSize = sizeof(T);

	VertexBuffer() = default;

	void generate(size_t vertexCount);

	void bind() const;

	void unbind() const;

	void setBuffer(size_t bytes, const void* data);

	size_t getBufferBytes() const;

	size_t getVertexCount() const;

	void free();

	const unsigned int getID() const;
};

template<typename T>
void VertexBuffer<T>::generate(size_t vertexCount)
{
	m_VertexCount = vertexCount;

	glGenBuffers(1, &m_VertexBufferID);
	glBindBuffer(GL_ARRAY_BUFFER, m_VertexBufferID);
}

template<typename T>
void VertexBuffer<T>::bind() const
{
	glBindBuffer(GL_ARRAY_BUFFER, m_VertexBufferID);
}

template<typename T>
void VertexBuffer<T>::unbind() const
{
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

template<typename T>
void VertexBuffer<T>::setBuffer(size_t bytes, const void* data)
{
	m_BufferBytes = bytes;
	glBufferData(GL_ARRAY_BUFFER, bytes, data, GL_STATIC_DRAW);
}

template<typename T>
size_t VertexBuffer<T>::getBufferBytes() const
{
	return m_BufferBytes;
}

template<typename T>
size_t VertexBuffer<T>::getVertexCount() const
{
	return m_VertexCount;
}

template<typename T>
void VertexBuffer<T>::free()
{
	glDeleteBuffers(1, &m_VertexBufferID);
}

template<typename T>
const unsigned int VertexBuffer<T>::getID() const
{
	return m_VertexBufferID;
}