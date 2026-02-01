#pragma once
#include <glad/glad.h>

class Framebuffer
{
public:
	Framebuffer(int screenWidth, int screenHeight);
	~Framebuffer();
	void Bind();
	void Unbind();
	void Delete();
public:
	unsigned int fb_id;
	unsigned int rb_id;
	unsigned int fbTex;
};