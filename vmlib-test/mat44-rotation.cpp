#include <catch2/catch_amalgamated.hpp>

#include <numbers>

#include "../vmlib/mat44.hpp"

TEST_CASE( "4x4 rotation around Z axis", "[rotation][mat44]" )
{
	static constexpr float kEps_ = 1e-6f;

	using namespace Catch::Matchers;

	// Simple check: rotating zero degrees should yield an idenity matrix
	SECTION( "Identity" )
	{
		auto const identity = make_rotation_z( 0.f );

		REQUIRE_THAT( identity(0,0), WithinAbs( 1.f, kEps_ ) );
		REQUIRE_THAT( identity(0,1), WithinAbs( 0.f, kEps_ ) );
		REQUIRE_THAT( identity(0,2), WithinAbs( 0.f, kEps_ ) );
		REQUIRE_THAT( identity(0,3), WithinAbs( 0.f, kEps_ ) );

		REQUIRE_THAT( identity(1,0), WithinAbs( 0.f, kEps_ ) );
		REQUIRE_THAT( identity(1,1), WithinAbs( 1.f, kEps_ ) );
		REQUIRE_THAT( identity(1,2), WithinAbs( 0.f, kEps_ ) );
		REQUIRE_THAT( identity(1,3), WithinAbs( 0.f, kEps_ ) );

		REQUIRE_THAT( identity(2,0), WithinAbs( 0.f, kEps_ ) );
		REQUIRE_THAT( identity(2,1), WithinAbs( 0.f, kEps_ ) );
		REQUIRE_THAT( identity(2,2), WithinAbs( 1.f, kEps_ ) );
		REQUIRE_THAT( identity(2,3), WithinAbs( 0.f, kEps_ ) );

		REQUIRE_THAT( identity(3,0), WithinAbs( 0.f, kEps_ ) );
		REQUIRE_THAT( identity(3,1), WithinAbs( 0.f, kEps_ ) );
		REQUIRE_THAT( identity(3,2), WithinAbs( 0.f, kEps_ ) );
		REQUIRE_THAT( identity(3,3), WithinAbs( 1.f, kEps_ ) );
	}

	// Rotating 90 degrees = pi/2 radians.
	SECTION( "90 degrees" )
	{
		auto const right = make_rotation_z( std::numbers::pi_v<float>/2.f );

		REQUIRE_THAT( right(0,0), WithinAbs( 0.f, kEps_ ) );
		REQUIRE_THAT( right(0,1), WithinAbs( -1.f, kEps_ ) );
		REQUIRE_THAT( right(0,2), WithinAbs( 0.f, kEps_ ) );
		REQUIRE_THAT( right(0,3), WithinAbs( 0.f, kEps_ ) );

		REQUIRE_THAT( right(1,0), WithinAbs( 1.f, kEps_ ) );
		REQUIRE_THAT( right(1,1), WithinAbs( 0.f, kEps_ ) );
		REQUIRE_THAT( right(1,2), WithinAbs( 0.f, kEps_ ) );
		REQUIRE_THAT( right(1,3), WithinAbs( 0.f, kEps_ ) );

		REQUIRE_THAT( right(2,0), WithinAbs( 0.f, kEps_ ) );
		REQUIRE_THAT( right(2,1), WithinAbs( 0.f, kEps_ ) );
		REQUIRE_THAT( right(2,2), WithinAbs( 1.f, kEps_ ) );
		REQUIRE_THAT( right(2,3), WithinAbs( 0.f, kEps_ ) );

		REQUIRE_THAT( right(3,0), WithinAbs( 0.f, kEps_ ) );
		REQUIRE_THAT( right(3,1), WithinAbs( 0.f, kEps_ ) );
		REQUIRE_THAT( right(3,2), WithinAbs( 0.f, kEps_ ) );
		REQUIRE_THAT( right(3,3), WithinAbs( 1.f, kEps_ ) );
	}

	// Rotating -90 degrees = -pi/2 radians.
	SECTION( "-90 degrees" )
	{
		auto const right = make_rotation_z( -std::numbers::pi_v<float>/2.f );

		REQUIRE_THAT( right(0,0), WithinAbs( 0.f, kEps_ ) );
		REQUIRE_THAT( right(0,1), WithinAbs( 1.f, kEps_ ) );
		REQUIRE_THAT( right(0,2), WithinAbs( 0.f, kEps_ ) );
		REQUIRE_THAT( right(0,3), WithinAbs( 0.f, kEps_ ) );

		REQUIRE_THAT( right(1,0), WithinAbs( -1.f, kEps_ ) );
		REQUIRE_THAT( right(1,1), WithinAbs( 0.f, kEps_ ) );
		REQUIRE_THAT( right(1,2), WithinAbs( 0.f, kEps_ ) );
		REQUIRE_THAT( right(1,3), WithinAbs( 0.f, kEps_ ) );

		REQUIRE_THAT( right(2,0), WithinAbs( 0.f, kEps_ ) );
		REQUIRE_THAT( right(2,1), WithinAbs( 0.f, kEps_ ) );
		REQUIRE_THAT( right(2,2), WithinAbs( 1.f, kEps_ ) );
		REQUIRE_THAT( right(2,3), WithinAbs( 0.f, kEps_ ) );

		REQUIRE_THAT( right(3,0), WithinAbs( 0.f, kEps_ ) );
		REQUIRE_THAT( right(3,1), WithinAbs( 0.f, kEps_ ) );
		REQUIRE_THAT( right(3,2), WithinAbs( 0.f, kEps_ ) );
		REQUIRE_THAT( right(3,3), WithinAbs( 1.f, kEps_ ) );
	}

	// Rotating 180 degrees = pi radians.
	SECTION("180 degrees")
	{
		auto const flip = make_rotation_z(std::numbers::pi_v<float>);

		REQUIRE_THAT(flip(0, 0), WithinAbs(-1.f, kEps_));
		REQUIRE_THAT(flip(0, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(flip(0, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(flip(0, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(flip(1, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(flip(1, 1), WithinAbs(-1.f, kEps_));
		REQUIRE_THAT(flip(1, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(flip(1, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(flip(2, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(flip(2, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(flip(2, 2), WithinAbs(1.f, kEps_));
		REQUIRE_THAT(flip(2, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(flip(3, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(flip(3, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(flip(3, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(flip(3, 3), WithinAbs(1.f, kEps_));
	}

	// Rotating 360 degrees = 2*pi radians.
	// Matching agaisnt Identity matrix
	SECTION("360 degrees")
	{
		auto const full_circle = make_rotation_z(2.f * std::numbers::pi_v<float>);

		REQUIRE_THAT(full_circle(0, 0), WithinAbs(1.f, kEps_));
		REQUIRE_THAT(full_circle(0, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(full_circle(0, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(full_circle(0, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(full_circle(1, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(full_circle(1, 1), WithinAbs(1.f, kEps_));
		REQUIRE_THAT(full_circle(1, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(full_circle(1, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(full_circle(2, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(full_circle(2, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(full_circle(2, 2), WithinAbs(1.f, kEps_));
		REQUIRE_THAT(full_circle(2, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(full_circle(3, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(full_circle(3, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(full_circle(3, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(full_circle(3, 3), WithinAbs(1.f, kEps_));
	}

	SECTION("270 degrees and -90 degrees equivalence")
	{
		// 270 degrees = 3*pi/2 radians
		auto const rot270 = make_rotation_z(3.f * std::numbers::pi_v<float> / 2.f);

		// -90 degrees = -pi/2 radians
		auto const rotNeg90 = make_rotation_z(-std::numbers::pi_v<float> / 2.f);

		// Verify equivalence of 270 degrees and -90 degrees
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				REQUIRE_THAT(rot270(i, j), WithinAbs(rotNeg90(i, j), kEps_));
			}
		}
	}
}

TEST_CASE("4x4 rotation around X axis", "[rotation][mat44]")
{
	static constexpr float kEps_ = 1e-6f;

	using namespace Catch::Matchers;

	// Simple check: rotating zero degrees should yield an identity matrix
	SECTION("Identity")
	{
		auto const identity = make_rotation_x(0.f);

		REQUIRE_THAT(identity(0, 0), WithinAbs(1.f, kEps_));
		REQUIRE_THAT(identity(0, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(identity(0, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(identity(0, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(identity(1, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(identity(1, 1), WithinAbs(1.f, kEps_));
		REQUIRE_THAT(identity(1, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(identity(1, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(identity(2, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(identity(2, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(identity(2, 2), WithinAbs(1.f, kEps_));
		REQUIRE_THAT(identity(2, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(identity(3, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(identity(3, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(identity(3, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(identity(3, 3), WithinAbs(1.f, kEps_));
	}

	// Rotating 90 degrees = pi/2 radians.
	SECTION("90 degrees")
	{
		auto const right = make_rotation_x(std::numbers::pi_v<float> / 2.f);

		REQUIRE_THAT(right(0, 0), WithinAbs(1.f, kEps_));
		REQUIRE_THAT(right(0, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(right(0, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(right(0, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(right(1, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(right(1, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(right(1, 2), WithinAbs(-1.f, kEps_));
		REQUIRE_THAT(right(1, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(right(2, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(right(2, 1), WithinAbs(1.f, kEps_));
		REQUIRE_THAT(right(2, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(right(2, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(right(3, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(right(3, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(right(3, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(right(3, 3), WithinAbs(1.f, kEps_));
	}

	// Rotating -90 degrees = -pi/2 radians.
	SECTION("-90 degrees")
	{
		auto const right = make_rotation_x(-std::numbers::pi_v<float> / 2.f);

		REQUIRE_THAT(right(0, 0), WithinAbs(1.f, kEps_));
		REQUIRE_THAT(right(0, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(right(0, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(right(0, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(right(1, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(right(1, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(right(1, 2), WithinAbs(1.f, kEps_));
		REQUIRE_THAT(right(1, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(right(2, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(right(2, 1), WithinAbs(-1.f, kEps_));
		REQUIRE_THAT(right(2, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(right(2, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(right(3, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(right(3, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(right(3, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(right(3, 3), WithinAbs(1.f, kEps_));
	}

	// Rotating 180 degrees = pi radians.
	SECTION("180 degrees")
	{
		auto const flip = make_rotation_x(std::numbers::pi_v<float>);

		REQUIRE_THAT(flip(0, 0), WithinAbs(1.f, kEps_));
		REQUIRE_THAT(flip(0, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(flip(0, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(flip(0, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(flip(1, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(flip(1, 1), WithinAbs(-1.f, kEps_));
		REQUIRE_THAT(flip(1, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(flip(1, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(flip(2, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(flip(2, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(flip(2, 2), WithinAbs(-1.f, kEps_));
		REQUIRE_THAT(flip(2, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(flip(3, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(flip(3, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(flip(3, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(flip(3, 3), WithinAbs(1.f, kEps_));
	}

	// Rotating 360 degrees = 2*pi radians.
	SECTION("360 degrees")
	{
		auto const full_circle = make_rotation_x(2.f * std::numbers::pi_v<float>);

		REQUIRE_THAT(full_circle(0, 0), WithinAbs(1.f, kEps_));
		REQUIRE_THAT(full_circle(0, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(full_circle(0, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(full_circle(0, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(full_circle(1, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(full_circle(1, 1), WithinAbs(1.f, kEps_));
		REQUIRE_THAT(full_circle(1, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(full_circle(1, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(full_circle(2, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(full_circle(2, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(full_circle(2, 2), WithinAbs(1.f, kEps_));
		REQUIRE_THAT(full_circle(2, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(full_circle(3, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(full_circle(3, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(full_circle(3, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(full_circle(3, 3), WithinAbs(1.f, kEps_));
	}

	SECTION("270 degrees and -90 degrees equivalence")
	{
		// 270 degrees = 3*pi/2 radians
		auto const rot270 = make_rotation_x(3.f * std::numbers::pi_v<float> / 2.f);

		// -90 degrees = -pi/2 radians
		auto const rotNeg90 = make_rotation_x(-std::numbers::pi_v<float> / 2.f);

		// Verify equivalence of 270 degrees and -90 degrees
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				REQUIRE_THAT(rot270(i, j), WithinAbs(rotNeg90(i, j), kEps_));
			}
		}
	}
}


TEST_CASE("4x4 rotation around Y axis", "[rotation][mat44]")
{
	static constexpr float kEps_ = 1e-6f;

	using namespace Catch::Matchers;

	// Simple check: rotating zero degrees should yield an identity matrix
	SECTION("Identity")
	{
		auto const identity = make_rotation_y(0.f);

		REQUIRE_THAT(identity(0, 0), WithinAbs(1.f, kEps_));
		REQUIRE_THAT(identity(0, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(identity(0, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(identity(0, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(identity(1, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(identity(1, 1), WithinAbs(1.f, kEps_));
		REQUIRE_THAT(identity(1, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(identity(1, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(identity(2, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(identity(2, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(identity(2, 2), WithinAbs(1.f, kEps_));
		REQUIRE_THAT(identity(2, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(identity(3, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(identity(3, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(identity(3, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(identity(3, 3), WithinAbs(1.f, kEps_));
	}

	// Rotating 90 degrees = pi/2 radians.
	SECTION("90 degrees")
	{
		auto const right = make_rotation_y(std::numbers::pi_v<float> / 2.f);

		REQUIRE_THAT(right(0, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(right(0, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(right(0, 2), WithinAbs(1.f, kEps_));
		REQUIRE_THAT(right(0, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(right(1, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(right(1, 1), WithinAbs(1.f, kEps_));
		REQUIRE_THAT(right(1, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(right(1, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(right(2, 0), WithinAbs(-1.f, kEps_));
		REQUIRE_THAT(right(2, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(right(2, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(right(2, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(right(3, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(right(3, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(right(3, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(right(3, 3), WithinAbs(1.f, kEps_));
	}

	// Rotating -90 degrees = -pi/2 radians.
	SECTION("-90 degrees")
	{
		auto const right = make_rotation_y(-std::numbers::pi_v<float> / 2.f);

		REQUIRE_THAT(right(0, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(right(0, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(right(0, 2), WithinAbs(-1.f, kEps_));
		REQUIRE_THAT(right(0, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(right(1, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(right(1, 1), WithinAbs(1.f, kEps_));
		REQUIRE_THAT(right(1, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(right(1, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(right(2, 0), WithinAbs(1.f, kEps_));
		REQUIRE_THAT(right(2, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(right(2, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(right(2, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(right(3, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(right(3, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(right(3, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(right(3, 3), WithinAbs(1.f, kEps_));
	}

	// Rotating 180 degrees = pi radians.
	SECTION("180 degrees")
	{
		auto const flip = make_rotation_y(std::numbers::pi_v<float>);

		REQUIRE_THAT(flip(0, 0), WithinAbs(-1.f, kEps_));
		REQUIRE_THAT(flip(0, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(flip(0, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(flip(0, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(flip(1, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(flip(1, 1), WithinAbs(1.f, kEps_));
		REQUIRE_THAT(flip(1, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(flip(1, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(flip(2, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(flip(2, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(flip(2, 2), WithinAbs(-1.f, kEps_));
		REQUIRE_THAT(flip(2, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(flip(3, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(flip(3, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(flip(3, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(flip(3, 3), WithinAbs(1.f, kEps_));
	}

	// Rotating 360 degrees = 2*pi radians.
	SECTION("360 degrees")
	{
		auto const full_circle = make_rotation_y(2.f * std::numbers::pi_v<float>);

		REQUIRE_THAT(full_circle(0, 0), WithinAbs(1.f, kEps_));
		REQUIRE_THAT(full_circle(0, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(full_circle(0, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(full_circle(0, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(full_circle(1, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(full_circle(1, 1), WithinAbs(1.f, kEps_));
		REQUIRE_THAT(full_circle(1, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(full_circle(1, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(full_circle(2, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(full_circle(2, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(full_circle(2, 2), WithinAbs(1.f, kEps_));
		REQUIRE_THAT(full_circle(2, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(full_circle(3, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(full_circle(3, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(full_circle(3, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(full_circle(3, 3), WithinAbs(1.f, kEps_));
	}

	SECTION("270 degrees and -90 degrees equivalence")
	{
		// 270 degrees = 3*pi/2 radians
		auto const rot270 = make_rotation_y(3.f * std::numbers::pi_v<float> / 2.f);

		// -90 degrees = -pi/2 radians
		auto const rotNeg90 = make_rotation_y(-std::numbers::pi_v<float> / 2.f);

		// Verify equivalence of 270 degrees and -90 degrees
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				REQUIRE_THAT(rot270(i, j), WithinAbs(rotNeg90(i, j), kEps_));
			}
		}
	}
}


