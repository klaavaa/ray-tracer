#pragma once
#include <glad/glad.h>

class VBO
{
public:
	VBO(const void* data, GLsizeiptr size);
	void Bind();
	void Unbind();
	void Delete();
public:
	unsigned int id;
};