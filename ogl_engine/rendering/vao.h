#pragma once
#include <glad/glad.h>
#include "vbo.h"

class VAO
{
public:
	VAO();
	void LinkAttrib(VBO& vbo);
	void Bind();
	void Unbind();
	void Delete();
public:
	unsigned int id;
};