#pragma once
#include <glad/glad.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader
{
public:
    Shader(const char* vertexPath, const char* fragmentPath);
    void Bind();
    void Unbind();
    void CheckCompileErrors(unsigned int shader, std::string type);
    void SetUInt(const std::string& name, uint32_t value) const;
    void SetInt(const std::string& name, int value) const;
    void SetInt2(const std::string& name, glm::ivec2 value) const;
    void SetInt3(const std::string& name, glm::ivec3 value) const;
    void SetFloat(const std::string& name, float value) const;
    void SetFloat2(const std::string& name, glm::vec2 value) const;
    void SetFloat3(const std::string& name, glm::vec3 value) const;
    void SetFloat4(const std::string& name, glm::vec4 value) const;
private:
	uint32_t id;
};