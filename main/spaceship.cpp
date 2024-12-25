#include "spaceship.hpp"

#include "cylinder.hpp"
#include "cone.hpp"
#include "triangle_prism.hpp"
#include "simple_mesh.hpp"

#include <numbers>


SimpleMeshData make_truncated_ovoid(
    std::size_t aCircleSubdivs,    // Number of subdivisions around circumference
    std::size_t aHeightSubdivs,    // Number of subdivisions along height
    float verticalScale,           // Scaling factor for height (elongation)
    float topCutoff,              // How much to cut from top (0.0 - 1.0)
    float bottomCutoff,           // How much to cut from bottom (0.0 - 1.0)
    Vec3f aColor,
    Mat44f aPreTransform
) {
    SimpleMeshData data{};

    // Precompute the normal matrix
    Mat33f const N = mat44_to_mat33(transpose(invert(aPreTransform)));

    // Calculate phi (vertical) angles with cutoffs
    float phiStart = bottomCutoff * std::numbers::pi_v<float>;
    float phiEnd = (1.0f - topCutoff) * std::numbers::pi_v<float>;
    float phiStep = (phiEnd - phiStart) / float(aHeightSubdivs);

    // Calculate theta (horizontal) step
    float thetaStep = 2.0f * std::numbers::pi_v<float> / float(aCircleSubdivs);

    // Generate vertices and triangles
    for (std::size_t phi_idx = 0; phi_idx < aHeightSubdivs; ++phi_idx) {
        float phi1 = phiStart + phi_idx * phiStep;
        float phi2 = phi1 + phiStep;

        for (std::size_t theta_idx = 0; theta_idx < aCircleSubdivs; ++theta_idx) {
            float theta1 = theta_idx * thetaStep;
            float theta2 = theta1 + thetaStep;

            // Calculate vertices for current quad
            // Note: y is scaled by verticalScale to create elongation
            auto calcVertex = [verticalScale](float phi, float theta) -> Vec3f {
                return Vec3f{
                    std::sin(phi) * std::cos(theta),
                    verticalScale * std::cos(phi),
                    std::sin(phi) * std::sin(theta)
                };
                };

            auto calcNormal = [verticalScale, &N](float phi, float theta) -> Vec3f {
                // For an ovoid, we need to adjust the normal based on the vertical scaling
                Vec3f normal{
                    std::sin(phi) * std::cos(theta),
                    std::cos(phi) / verticalScale, // Adjust normal for vertical scaling
                    std::sin(phi) * std::sin(theta)
                };
                return normalize(N * normalize(normal));
                };

            // Calculate four corners of the quad
            Vec3f v1 = calcVertex(phi1, theta1);
            Vec3f v2 = calcVertex(phi1, theta2);
            Vec3f v3 = calcVertex(phi2, theta1);
            Vec3f v4 = calcVertex(phi2, theta2);

            // Calculate normals for each vertex
            Vec3f n1 = calcNormal(phi1, theta1);
            Vec3f n2 = calcNormal(phi1, theta2);
            Vec3f n3 = calcNormal(phi2, theta1);
            Vec3f n4 = calcNormal(phi2, theta2);

            // First triangle of quad
            data.positions.insert(data.positions.end(), { v1, v2, v3 });
            data.normals.insert(data.normals.end(), { n1, n2, n3 });

            // Second triangle of quad
            data.positions.insert(data.positions.end(), { v2, v4, v3 });
            data.normals.insert(data.normals.end(), { n2, n4, n3 });
        }
    }

    // Transform positions
    for (auto& p : data.positions) {
        Vec4f p4{ p.x, p.y, p.z, 1.f };
        Vec4f t = aPreTransform * p4;
        t /= t.w;
        p = Vec3f{ t.x, t.y, t.z };
    }

    // Add colors
    data.colors.assign(data.positions.size(), aColor);

    return data;
}

SimpleMeshData create_spaceship(std::size_t aSubdivs, Vec3f aColorMainBody, Vec3f aColorWings, Mat44f aPreTransform)
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
		make_translation({ 2.f, 0.f, 0.f }) * make_scaling(2.f, 0.5f, 0.5f)
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

	// Loop through and set each vector to zero
	rocketData.texcoords.assign(rocketData.positions.size(), Vec2f{ 0.f, 0.f });
	rocketData.Ka.assign(rocketData.positions.size(), Vec3f{ 0.f, 0.f, 0.f });
	rocketData.Kd.assign(rocketData.positions.size(), Vec3f{ 0.f, 0.f, 0.f });
	rocketData.Ks.assign(rocketData.positions.size(), Vec3f{ 0.f, 0.f, 0.f });
	rocketData.Ns.assign(rocketData.positions.size(), 0.f);
	rocketData.Ke.assign(rocketData.positions.size(), Vec3f{ 0.f, 0.f, 0.f });

	// Set mins and diffs to zero
	rocketData.mins = Vec2f{ 0.f, 0.f };
	rocketData.diffs = Vec2f{ 0.f, 0.f };


	return rocketData;
}
