#ifndef SPACESHIP_LOADER_HPP
#define SPACESHIP_LOADER_HPP

#include <vector>

#include <cstdlib>

#include "simple_mesh.hpp"

#include "../vmlib/vec3.hpp"
#include "../vmlib/mat44.hpp"


SimpleMeshData create_spaceship(std::size_t aSubdivs = 32, Vec3f aColorMainBody = {1.f, 1.f, 1.f}, 
	Vec3f aColorWings = { 0.f, 0.f, 0.f }, Mat44f aPreTransform = kIdentity44f,
	bool isTextureSupplied = false
);

#endif // SPACESHIP_LOADER_HPP