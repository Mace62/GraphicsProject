#include "spaceship.hpp"

#include "cylinder.hpp"
#include "cone.hpp"
#include "triangle_prism.hpp"
#include "ovoid.hpp"
#include "simple_mesh.hpp"

#include <numbers>


SimpleMeshData create_spaceship(std::size_t aSubdivs, Vec3f aColorMainBody, Vec3f aColorWings, Mat44f aPreTransform, bool isTextureSupplied)
{
	// Expecting 4 integral components.
	// 1. Main body (cylinder)
	// 2. Nose of craft (cone)
	// 3. Flight control surfaces (not realistic but for the project shape req) (triangular prisms)
	// 4. Rocket nozzle (cut ovoid)

	SimpleMeshData rocketData{};

	// Create cylinder for main body
	auto mainBodyCylinder = make_cylinder(true, aSubdivs, aColorMainBody,
		make_scaling(4.f, 0.5f, 0.5f) * make_translation({ -0.5f, 0.f, 0.f })		// Centre cylinder first, then apply scaling
	);

	// Create cone for nose of spacecraft
	auto spaceshipNoseCone = make_cone(false, aSubdivs, aColorMainBody,
		make_translation({ 2.f, 0.f, 0.f }) * make_scaling(1.f, 0.5f, 0.5f)
	);

	// Create 2 wings as "flight control surfaces"
	auto wingTriangleBasedPrism = make_triangle_based_prism( true, 
		{1.5f, 0.f}, { 0.f, 0.f }, { 0.f, 1.f },
		0.05f, aColorWings,
		make_rotation_y(-90 *(std::numbers::pi_v<float> / 180.0)) * make_translation({0.f, 1.f, -0.5f}) * make_rotation_x(-90 * (std::numbers::pi_v<float> / 180.0))
	);

	rocketData = concatenate(std::move(mainBodyCylinder), spaceshipNoseCone);

	rocketData = concatenate(std::move(rocketData), wingTriangleBasedPrism);

	// Loop through the original positions and apply matrix
	for (auto& pos : wingTriangleBasedPrism.positions) {
		// Transform the position with the rotation matrix
		Vec4f transformedPos = make_rotation_x(std::numbers::pi_v<float>) * Vec4f{ pos.x, pos.y, pos.z, 1.f };

		// Replace the current position with the transformed position
		pos = Vec3f{ transformedPos.x, transformedPos.y, transformedPos.z };
	}
	rocketData = concatenate(std::move(rocketData), wingTriangleBasedPrism);



	// Create 4 rocket holder stands
	auto standTriangleBasedPrism = make_triangle_based_prism(true,
		{ 1.0f, 0.f }, { 0.f, 0.f }, { -1.f, 1.f },
		0.05f, aColorWings,
		make_rotation_y(-90 * (std::numbers::pi_v<float> / 180.0)) * make_translation({ 0.f, 1.f, 1.75f }) * make_rotation_x(-90 * (std::numbers::pi_v<float> / 180.0))
	);
	rocketData = concatenate(std::move(rocketData), standTriangleBasedPrism);

	// Other 3 created by matrix calculation
	for (int standNum = 0; standNum < 3; standNum++)
	{
		// Loop through the original positions and apply matrix
		for (auto& pos : standTriangleBasedPrism.positions) {
			// Transform the position with the rotation matrix
			Vec4f transformedPos = make_rotation_x(std::numbers::pi_v<float>/2.f) * Vec4f { pos.x, pos.y, pos.z, 1.f };

			// Replace the current position with the transformed position
			pos = Vec3f{ transformedPos.x, transformedPos.y, transformedPos.z };
		}
		rocketData = concatenate(std::move(rocketData), standTriangleBasedPrism);
	}
	
    // Example usage:
    SimpleMeshData nozzle = make_truncated_ovoid(
        32,     // circumference subdivisions
        16,     // height subdivisions
        2.0f,   // vertical scaling (makes it more elongated)
        0.6f,   // top cutoff (30% from top)
        0.15f,   // bottom cutoff (20% from bottom)
        Vec3f{ 0.8f, 0.8f, 0.8f },  // color (metallic gray)
        make_rotation_z(-90 * (std::numbers::pi_v<float> / 180.0)) * make_translation({ 0.f, -2.88f , 0.f }) * make_scaling(0.5f, 0.5f, 0.5f)
    );
	
    rocketData = concatenate(std::move(rocketData), nozzle);

	// Set tex coords to 0
	rocketData.texcoords.assign(rocketData.positions.size(), Vec2f{ 0.f, 0.f });

	// Set mins and diffs to zero
	rocketData.mins = Vec2f{ 0.f, 0.f };
	rocketData.diffs = Vec2f{ 0.f, 0.f };

    // Apply pretransform matrix
    // Transform positions by aPreTransform
    for (auto& p : rocketData.positions)
    {
        Vec4f p4{ p.x, p.y, p.z, 1.f };
        Vec4f t = aPreTransform * make_rotation_z(std::numbers::pi_v<float> / 2.f) * p4;
        t /= t.w;

        p = Vec3f{ t.x, t.y, t.z };
    }

    rocketData.isTextureSupplied = isTextureSupplied;

	return rocketData;
}
