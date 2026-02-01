#pragma once

#include <glm/glm.hpp>



struct Material
{
	glm::vec3 color = { 0, 0, 0 };
	glm::vec4 emission = { 0, 0, 0, 0 };
	float reflection = 0.0f;
};

struct AxisAllignedBox
{
	glm::vec3 p1;
	glm::vec3 p2;
};

struct TriMesh
{
	std::string name = "trimesh";
	std::string filename = "";
	std::vector<glm::vec3> vertices;
	std::vector<glm::ivec3> indices;
	std::vector<glm::vec3> transformed_vertices;
	Material material;

	glm::mat4 transform = glm::identity<glm::mat4>();
	glm::vec3 translation = { 0, 0, 0 };
	glm::vec3 rotation = { 0, 0, 0 };
	glm::vec3 scale = { 1, 1, 1 };

	AxisAllignedBox box;
};

struct Sphere
{
	std::string name = "sphere";
	glm::vec3 center = { 0, 0, 0 };
	float radius = 1.0f;
	
	Material material;

};