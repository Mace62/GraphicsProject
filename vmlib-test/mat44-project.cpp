#include <catch2/catch_amalgamated.hpp>

#include <numbers>

#include "../vmlib/mat44.hpp"

// See mat44-rotation.cpp first.

TEST_CASE( "Perspective projection", "[mat44]" )
{
	static constexpr float kEps_ = 1e-6f;

	using namespace Catch::Matchers;

	// "Standard" projection matrix presented in the exercises. Assumes
	// standard window size (e.g., 1280x720).
	//
	// Field of view (FOV) = 60 degrees
	// Window size is 1280x720 and we defined the aspect ratio as w/h
	// Near plane at 0.1 and far at 100
	SECTION( "Standard" )
	{
		auto const proj = make_perspective_projection(
			60.f * std::numbers::pi_v<float> / 180.f,
			1280/float(720),
			0.1f, 100.f
		);

		REQUIRE_THAT( proj(0,0), WithinAbs( 0.974279, kEps_ ) );
		REQUIRE_THAT( proj(0,1), WithinAbs( 0.f, kEps_ ) );
		REQUIRE_THAT( proj(0,2), WithinAbs( 0.f, kEps_ ) );
		REQUIRE_THAT( proj(0,3), WithinAbs( 0.f, kEps_ ) );

		REQUIRE_THAT( proj(1,0), WithinAbs( 0.f, kEps_ ) );
		REQUIRE_THAT( proj(1,1), WithinAbs( 1.732051f, kEps_ ) );
		REQUIRE_THAT( proj(1,2), WithinAbs( 0.f, kEps_ ) );
		REQUIRE_THAT( proj(1,3), WithinAbs( 0.f, kEps_ ) );

		REQUIRE_THAT( proj(2,0), WithinAbs( 0.f, kEps_ ) );
		REQUIRE_THAT( proj(2,1), WithinAbs( 0.f, kEps_ ) );
		REQUIRE_THAT( proj(2,2), WithinAbs( -1.002002f, kEps_ ) );
		REQUIRE_THAT( proj(2,3), WithinAbs( -0.200200f, kEps_ ) );

		REQUIRE_THAT( proj(3,0), WithinAbs( 0.f, kEps_ ) );
		REQUIRE_THAT( proj(3,1), WithinAbs( 0.f, kEps_ ) );
		REQUIRE_THAT( proj(3,2), WithinAbs( -1.f, kEps_ ) );
		REQUIRE_THAT( proj(3,3), WithinAbs( 0.f, kEps_ ) );
	}


	SECTION("Narrow FOV (30 degrees)")
	{
		auto const proj = make_perspective_projection(
			30.f * std::numbers::pi_v<float> / 180.f,  // 30 degrees FOV
			1280.f / 720.f,  // Aspect ratio
			0.1f, 100.f      // Near and far planes
		);

		REQUIRE_THAT(proj(0, 0), WithinAbs(2.099279, kEps_));
		REQUIRE_THAT(proj(0, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(proj(0, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(proj(0, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(proj(1, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(proj(1, 1), WithinAbs(3.732051f, kEps_));
		REQUIRE_THAT(proj(1, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(proj(1, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(proj(2, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(proj(2, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(proj(2, 2), WithinAbs(-1.002002f, kEps_));
		REQUIRE_THAT(proj(2, 3), WithinAbs(-0.200200f, kEps_));

		REQUIRE_THAT(proj(3, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(proj(3, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(proj(3, 2), WithinAbs(-1.f, kEps_));
		REQUIRE_THAT(proj(3, 3), WithinAbs(0.f, kEps_));
	}

	SECTION("Wide FOV (120 degrees)")
	{
		auto const proj = make_perspective_projection(
			120.f * std::numbers::pi_v<float> / 180.f,  // 120 degrees FOV
			1280.f / 720.f,  // Aspect ratio
			0.1f, 100.f      // Near and far planes
		);

		REQUIRE_THAT(proj(0, 0), WithinAbs(0.324760, kEps_));
		REQUIRE_THAT(proj(0, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(proj(0, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(proj(0, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(proj(1, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(proj(1, 1), WithinAbs(0.577350f, kEps_));
		REQUIRE_THAT(proj(1, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(proj(1, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(proj(2, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(proj(2, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(proj(2, 2), WithinAbs(-1.002002f, kEps_));
		REQUIRE_THAT(proj(2, 3), WithinAbs(-0.200200f, kEps_));

		REQUIRE_THAT(proj(3, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(proj(3, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(proj(3, 2), WithinAbs(-1.f, kEps_));
		REQUIRE_THAT(proj(3, 3), WithinAbs(0.f, kEps_));
	}

	SECTION("Aspect Ratio 4:3")
	{
		auto const proj = make_perspective_projection(
			60.f * std::numbers::pi_v<float> / 180.f,  // 60 degrees FOV
			4.f / 3.f,  // Aspect ratio 4:3
			0.1f, 100.f  // Near and far planes
		);

		// Check the relevant entries in the projection matrix
		REQUIRE_THAT(proj(0, 0), WithinAbs(1.299038f, kEps_));  // Adjusted for 4:3 aspect ratio
		REQUIRE_THAT(proj(0, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(proj(0, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(proj(0, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(proj(1, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(proj(1, 1), WithinAbs(1.732051f, kEps_));
		REQUIRE_THAT(proj(1, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(proj(1, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(proj(2, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(proj(2, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(proj(2, 2), WithinAbs(-1.002002f, kEps_));
		REQUIRE_THAT(proj(2, 3), WithinAbs(-0.200200f, kEps_));

		REQUIRE_THAT(proj(3, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(proj(3, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(proj(3, 2), WithinAbs(-1.f, kEps_));
		REQUIRE_THAT(proj(3, 3), WithinAbs(0.f, kEps_));
	}

	SECTION("Aspect Ratio 1:1")
	{
		auto const proj = make_perspective_projection(
			60.f * std::numbers::pi_v<float> / 180.f,  // 60 degrees FOV
			1.f,  // Aspect ratio 1:1
			0.1f, 100.f  // Near and far planes
		);

		// Check the relevant entries in the projection matrix
		REQUIRE_THAT(proj(0, 0), WithinAbs(1.732051, kEps_));
		REQUIRE_THAT(proj(0, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(proj(0, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(proj(0, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(proj(1, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(proj(1, 1), WithinAbs(1.732051f, kEps_));
		REQUIRE_THAT(proj(1, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(proj(1, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(proj(2, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(proj(2, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(proj(2, 2), WithinAbs(-1.002002f, kEps_));
		REQUIRE_THAT(proj(2, 3), WithinAbs(-0.200200f, kEps_));

		REQUIRE_THAT(proj(3, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(proj(3, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(proj(3, 2), WithinAbs(-1.f, kEps_));
		REQUIRE_THAT(proj(3, 3), WithinAbs(0.f, kEps_));
	}

	SECTION("Small Near Plane (0.01f)")
	{
		auto const proj = make_perspective_projection(
			60.f * std::numbers::pi_v<float> / 180.f,  // 60 degrees FOV
			1280.f / 720.f,  // Aspect ratio
			0.01f, 100.f     // Near plane = 0.01, Far plane = 100
		);

		// Check the relevant entries in the projection matrix
		REQUIRE_THAT(proj(2, 2), WithinAbs(-1.000200f, kEps_));  // Small change due to near/far ratio
		REQUIRE_THAT(proj(2, 3), WithinAbs(-0.020002f, kEps_));  // Small change due to near/far ratio
	}

	SECTION("Large Far Plane (1000.f)")
	{
		auto const proj = make_perspective_projection(
			60.f * std::numbers::pi_v<float> / 180.f,  // 60 degrees FOV
			1280.f / 720.f,  // Aspect ratio
			0.1f, 1000.f     // Near plane = 0.1, Far plane = 1000
		);

		// Check the relevant entries in the projection matrix
		REQUIRE_THAT(proj(2, 2), WithinAbs(-1.0002f, kEps_));  // Small change due to near/far ratio
		REQUIRE_THAT(proj(2, 3), WithinAbs(-0.200020f, kEps_));  // Small change due to near/far ratio
	}
}
