#pragma once
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <string>
#include <fstream>

#include <sstream>

#include "Object.h"

template<class T>
T base_name(T const& path, T const& delims = "/\\")
{
	return path.substr(path.find_last_of(delims) + 1);
}
template<class T>
T remove_extension(T const& filename)
{
	typename T::size_type const p(filename.find_last_of('.'));
	return p > 0 && p != T::npos ? filename.substr(0, p) : filename;
}

inline bool parse_obj(const std::string& filename, std::vector<glm::vec3>& vertices, std::vector<glm::ivec3>& indices)
{


	std::cout << "loading " << filename << "...\n";

	std::ifstream file;
	file.open(filename);

	if (!file.is_open())
		return false;
	
	std::string line;

	glm::vec3 v;
	glm::ivec3 i;



	while (std::getline(file, line))
	{
		std::stringstream s;
		char junk;
		

		if (line[0] == 'v')
		{
			if (line[1] != ' ') continue;

			s << line;
			s >> junk >> v.x >> v.y >> v.z;
			vertices.push_back(v);
		}

		if (line[0] == 'f')
		{
			std::string new_line = "";
			bool skip = false;
			for (auto c : line)
			{
				if (c == '/')
					skip = true;
				
				if (c == ' ')
					skip = false;

				if (skip)
					continue;
				
				new_line += c;
			}

			s << new_line;

			s >> junk;
			std::vector<int> is;
			int indice = 0;
			while (s >> indice)
			{
				is.push_back(indice - 1);
			}

			for (size_t j = 0; j < is.size() - 2; j++)
			{
				i.x = is[0];
				i.y = is[j + 1];
				i.z = is[j + 2];
				indices.push_back(i);
			}

			
			
		}
	}


	std::cout << "finished loading " << filename << "\n";
	return true;
}

inline TriMesh parse_obj(const std::string& filename)
{

	std::cout << "loading " << filename << "...\n";

	std::ifstream file;
	file.open(filename);

	TriMesh trimesh;

	std::vector<glm::vec3> vertices;
	std::vector<glm::ivec3> indices;

	if (!file.is_open())
	{
		std::cout << "failed to load " << filename << "\n";
		return trimesh;
	}
	std::string line;

	glm::vec3 v;
	glm::ivec3 i;



	while (std::getline(file, line))
	{
		std::stringstream s;
		char junk;


		if (line[0] == 'v')
		{
			if (line[1] != ' ') continue;

			s << line;
			s >> junk >> v.x >> v.y >> v.z;
			vertices.push_back(v);
		}

		if (line[0] == 'f')
		{
			std::string new_line = "";
			bool skip = false;
			for (auto c : line)
			{
				if (c == '/')
					skip = true;

				if (c == ' ')
					skip = false;

				if (skip)
					continue;

				new_line += c;
			}

			s << new_line;

			s >> junk;
			std::vector<int> is;
			int indice = 0;
			while (s >> indice)
			{
				is.push_back(indice - 1);
			}

			for (size_t j = 0; j < is.size() - 2; j++)
			{
				i.x = is[0];
				i.y = is[j + 1];
				i.z = is[j + 2];
				indices.push_back(i);
			}



		}
	}

	trimesh.vertices = vertices;
	trimesh.indices = indices;
	trimesh.name = remove_extension(base_name(filename));
	trimesh.filename = filename;
	std::cout << "finished loading " << filename << "\n";
	return trimesh;
}

inline void save_scene(std::string filename, std::stringstream& save)
{

	std::ofstream file;
	std::cout << "saving scene " + filename + "...\n";
	file.open("saves\\" + filename);

	if (!file.is_open())
	{
		std::cout << "error saving scene " + filename + "\n";
		return;
	}
	file.write(save.str().c_str(), save.str().size());

}



inline void load_scene(const std::string& filename, glm::vec3& cam_pos, glm::vec2& cam_rot, glm::vec3& sky_color, glm::vec3& horizont_color,
	std::vector<Sphere>& spheres, std::vector<TriMesh>& trimeshes)
{
	std::ifstream file;
	std::cout << "loading scene " + filename + "...\n";
	file.open(filename);

	spheres.clear();
	trimeshes.clear();

	if (!file.is_open())
	{
		std::cout << "error loading scene " + filename + "\n";
		return;
	}

	std::string line;

	while (std::getline(file, line))
	{
		std::stringstream s;
		
		s << line;
		std::string type;
		s >> type;
		
		if (type == "cam_pos")
		{
			s >> cam_pos.x >> cam_pos.y >> cam_pos.z;
		}
		else if (type == "cam_rot")
		{
			s >> cam_rot.x >> cam_rot.y;
		}
		else if (type == "sky_color")
		{
			s >> sky_color.x >> sky_color.y >> sky_color.z;
		}
		else if (type == "horizont_color")
		{
			s >> horizont_color.x >> horizont_color.y >> horizont_color.z;
		}
		else if (type == "sphere")
		{
			
			Sphere sphere;
	
			s >> sphere.center.x >> sphere.center.y >> sphere.center.z >> sphere.radius >>
				sphere.material.color.r >> sphere.material.color.g >> sphere.material.color.b >>
				sphere.material.emission.x >> sphere.material.emission.y >> sphere.material.emission.z >> sphere.material.emission.w >>
				sphere.material.reflection;

			spheres.push_back(sphere);
		}

		else if (type == "trimesh")
		{
			TriMesh trimesh;

			s >> trimesh.filename >> trimesh.translation.x >> trimesh.translation.y >> trimesh.translation.z >>
				trimesh.rotation.x >> trimesh.rotation.y >> trimesh.rotation.z >>
				trimesh.scale.x >> trimesh.scale.y >> trimesh.scale.z >>
				trimesh.material.color.r >> trimesh.material.color.g >> trimesh.material.color.b >>
				trimesh.material.emission.x >> trimesh.material.emission.y >> trimesh.material.emission.z >> trimesh.material.emission.w;

			parse_obj(trimesh.filename, trimesh.vertices, trimesh.indices);

			trimeshes.push_back(trimesh);

		
		}
	}

}