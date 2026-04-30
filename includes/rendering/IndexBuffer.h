#pragma once

#include <glad/glad.h>

class IndexBuffer
{
private:
	unsigned int m_IndexBufferID = 0;

public:
	IndexBuffer() = default;

	void generate();

	void bind() const;

	void unbind() const;

	void setBuffer(size_t bytes, const void* data) const;

	void free() const;

	const unsigned int getID() const;
};