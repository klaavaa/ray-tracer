#pragma once
#include <glad/glad.h>

class EBO
{
public:
	EBO(const void* data, GLsizeiptr size);
	void Bind();
	void Unbind();
	void Delete();
public:
	unsigned int id;
};