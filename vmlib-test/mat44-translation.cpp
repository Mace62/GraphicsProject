#include <catch2/catch_amalgamated.hpp>

#include "../vmlib/mat44.hpp"

#include <numbers>

TEST_CASE("4x4 matrix translation", "[translation][mat44]")
{
	SECTION( "Basic translation" )
	{
        Mat44f translation = make_translation({ 3.0f, -4.0f, 5.0f });  // Translate by (3, -4, 5)
        Vec4f v = { 1.0f, 1.0f, 1.0f, 1.0f };  // Point at (1, 1, 1)
        Vec4f result = translation * v;

        REQUIRE(result.x == 4.0f);  // 1 + 3 = 4
        REQUIRE(result.y == -3.0f);  // 1 - 4 = -3
        REQUIRE(result.z == 6.0f);  // 1 + 5 = 6
	}

    SECTION("Zero translation")
    {
        Mat44f translation = make_translation({ 0.0f, 0.0f, 0.0f });  // No translation
        Vec4f v = { 1.0f, 1.0f, 1.0f, 1.0f };  // Point at (1, 1, 1)
        Vec4f result = translation * v;

        REQUIRE(result.x == 1.0f);  // 1 + 0 = 1
        REQUIRE(result.y == 1.0f);  // 1 + 0 = 1
        REQUIRE(result.z == 1.0f);  // 1 + 0 = 1
    }

    SECTION("Inverse translation")
    {
        Mat44f translation = make_translation({ 3.0f, -2.0f, 1.0f });  // Translate by (3, -2, 1)
        Mat44f inverse_translation = invert(translation);  // Inverse translation should negate the translation
        
        Vec4f v = { 1.0f, 1.0f, 1.0f, 1.0f };  // Point at (1, 1, 1)
        Vec4f result = inverse_translation * (translation * v);  // Apply translation and then its inverse

        REQUIRE(result.x == 1.0f);
        REQUIRE(result.y == 1.0f);
        REQUIRE(result.z == 1.0f);
    }

    SECTION("Translation on identity")
    {
        Mat44f translation = make_translation({ 5.0f, -3.0f, 2.0f });  // Translate by (5, -3, 2)
        Vec4f v = { 0.0f, 0.0f, 0.0f, 1.0f };  // Identity vector (origin)
        Vec4f result = translation * v;

        REQUIRE(result.x == 5.0f);
        REQUIRE(result.y == -3.0f);
        REQUIRE(result.z == 2.0f);
    }

    SECTION("Negative translation")
    {
        Mat44f translation = make_translation({ -2.0f, -4.0f, -6.0f });  // Translate by (-2, -4, -6)
        Vec4f v = { 1.0f, 1.0f, 1.0f, 1.0f };  // Point at (1, 1, 1)
        Vec4f result = translation * v;

        REQUIRE(result.x == -1.0f);
        REQUIRE(result.y == -3.0f);
        REQUIRE(result.z == -5.0f);
    }
}