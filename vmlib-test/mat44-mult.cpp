#include <catch2/catch_amalgamated.hpp>

#include "../vmlib/mat44.hpp"

#include <numbers>

TEST_CASE( "4x4 matrix by matrix multiplication", "[mat44]" )
{
    SECTION("Multiplying with identity matrix")
    {
        Mat44f const mat = { {
            1.f, 2.f, 3.f, 4.f,
            5.f, 6.f, 7.f, 8.f,
            9.f, 10.f, 11.f, 12.f,
            13.f, 14.f, 15.f, 16.f
        } };
        auto const identity = kIdentity44f;

        auto const result = mat * identity;

        // Verify the result is equal to mat
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                REQUIRE(result(i, j) == mat(i, j));
            }
        }
    }

    SECTION("Multiplying two matrices")
    {
        Mat44f const mat1 = { {
            1.f, 2.f, 3.f, 4.f,
            5.f, 6.f, 7.f, 8.f,
            9.f, 10.f, -11.f, 12.f,
            13.f, 14.f, 15.f, 16.f
        } };

        Mat44f const mat2 = { {
            2.f, 3.f, 4.f, 5.f, 
            6.f, -7.f, 8.f, 9.f, 
            10.f, 11.f, 12.f, 13.f,
            14.f, 15.f, 16.f, 17.f
        } };

        Mat44f const true_result = { {
            100.f, 82.f, 120.f, 130.f,
            228.f, 170.f, 280.f, 306.f,
            136.f, 16.f, 176.f, 196.f,
            484.f, 346.f, 600.f, 658.f
        } };

        auto const result = mat1 * mat2;

        // Manually or with another library, compute expected results and compare
        // Use Eigen/GLM or simple row-column multiplication
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                REQUIRE(result(i, j) == true_result(i, j));
            }
        }
    }

    SECTION("Commutative property for diagonal matrices")
    {
        Mat44f const mat1 = { {
            1.f, 0.f, 0.f, 0.f,
            0.f, 4.f, 0.f, 0.f,
            0.f, 0.f, 8.f, 0.f,
            0.f, 0.f, 0.f, 16.f
        } };

        Mat44f const mat2 = { {
            11.f, 0.f, 0.f, 0.f,
            0.f, 41.f, 0.f, 0.f,
            0.f, 0.f, 81.f, 0.f,
            0.f, 0.f, 0.f, 126.f
        } };

        auto const result1 = mat1 * mat2;
        auto const result2 = mat2 * mat1;

        // Verify if mat1 * mat2 == mat2 * mat1
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                REQUIRE(result1(i, j) == result2(i, j));
            }
        }
    }

    SECTION("Associative property")
    {
        Mat44f const mat1 = { {
            2.f, 3.f, 4.f, 5.f,
            6.f, 7.f, 8.f, 9.f,
            10.f, 11.f, 12.f, 13.f,
            14.f, 15.f, 16.f, 17.f
        } };

        Mat44f const mat2 = { {
            100.f, 110.f, 120.f, 130.f,
            228.f, 254.f, 280.f, 306.f,
            356.f, 398.f, 440.f, 482.f,
            484.f, 542.f, 600.f, 658.f
        } };

        Mat44f const mat3 = { {
            11.f, 0.f, 0.f, 0.f,
            0.f, 41.f, 0.f, 0.f,
            0.f, 0.f, 81.f, 0.f,
            0.f, 0.f, 0.f, 126.f
        } };

        auto const result1 = (mat1 * mat2) * mat3;
        auto const result2 = mat1 * (mat2 * mat3);

        // Verify if (mat1 * mat2) * mat3 == mat1 * (mat2 * mat3)
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                REQUIRE(result1(i, j) == result2(i, j));
            }
        }
    }

    SECTION("Multiplying by zero matrix")
    {
        Mat44f const mat = { {
            100.f, 110.f, 120.f, 130.f,
            228.f, 254.f, 280.f, 306.f,
            356.f, 398.f, 440.f, 482.f,
            484.f, 542.f, 600.f, 1.f
        } };

        Mat44f const zero = { {} };

        auto const result = mat * zero;

        // Verify that the result is a zero matrix
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                REQUIRE(result(i, j) == 0.f);
            }
        }
    }
}








TEST_CASE( "4x4 matrix by vector multiplication", "[mat44][vec4]" )
{
    static constexpr float kEps_ = 1e-6f;

    using namespace Catch::Matchers;

    SECTION("Identity matrix multiplication")
    {
        Mat44f identity = kIdentity44f; // Assume identity matrix is defined
        Vec4f v = { 1.f, 2.f, 3.f, 4.f };
        Vec4f result = identity * v;

        REQUIRE(result.x == 1.f);
        REQUIRE(result.y == 2.f);
        REQUIRE(result.z == 3.f);
        REQUIRE(result.w == 4.f);
    }

    SECTION("Zero matrix multiplication")
    {
        Mat44f zeroMatrix = {};  // Matrix with all elements set to 0
        Vec4f v = { 1.f, 2.f, 3.f, 4.f };
        Vec4f result = zeroMatrix * v;

        REQUIRE(result.x == 0.f);
        REQUIRE(result.y == 0.f);
        REQUIRE(result.z == 0.f);
        REQUIRE(result.w == 0.f);
    }

    SECTION("Rotation by 90 degrees around X-axis")
    {
        Mat44f rotation = make_rotation_x(std::numbers::pi_v<float> / 2.f);
        Vec4f v = { 1.f, 0.f, 0.f, 1.f };
        Vec4f result = rotation * v;

        REQUIRE_THAT(result.x, WithinAbs(1.f, kEps_));
        REQUIRE_THAT(result.y, WithinAbs(0.f, kEps_));
        REQUIRE_THAT(result.z, WithinAbs(0.f, kEps_));
        REQUIRE_THAT(result.w, WithinAbs(1.f, kEps_));
    }

    SECTION("Rotation by 90 degrees around Y-axis")
    {
        Mat44f rotation = make_rotation_y(std::numbers::pi_v<float> / 2.f);
        Vec4f v = { 1.f, 0.f, 0.f, 1.f };
        Vec4f result = rotation * v;

        REQUIRE_THAT(result.x, WithinAbs(0.f, kEps_));
        REQUIRE_THAT(result.y, WithinAbs(0.f, kEps_));
        REQUIRE_THAT(result.z, WithinAbs(-1.f, kEps_));
        REQUIRE_THAT(result.w, WithinAbs(1.f, kEps_));
    }

    SECTION("Rotation by 90 degrees around Z-axis")
    {
        Mat44f rotation = make_rotation_z(std::numbers::pi_v<float> / 2.f);
        Vec4f v = { 1.f, 0.f, 0.f, 1.f };
        Vec4f result = rotation * v;

        REQUIRE_THAT(result.x, WithinAbs(0.f, kEps_));
        REQUIRE_THAT(result.y, WithinAbs(1.f, kEps_));
        REQUIRE_THAT(result.x, WithinAbs(0.f, kEps_));
        REQUIRE_THAT(result.y, WithinAbs(1.f, kEps_));
    }

    SECTION("Rotation by 180 degrees around X-axis")
    {
        Mat44f rotation = make_rotation_x(std::numbers::pi_v<float>);  // pi radians = 180 degrees
        Vec4f v = { 1.f, 0.f, 0.f, 1.f };
        Vec4f result = rotation * v;

        REQUIRE_THAT(result.x, WithinAbs(1.f, kEps_));
        REQUIRE_THAT(result.y, WithinAbs(0.f, kEps_));
        REQUIRE_THAT(result.z, WithinAbs(0.f, kEps_));
        REQUIRE_THAT(result.w, WithinAbs(1.f, kEps_));
    }

    SECTION("Rotation by 180 degrees around Y-axis")
    {
        Mat44f rotation = make_rotation_y(std::numbers::pi_v<float>);  // pi radians = 180 degrees
        Vec4f v = { 1.f, 0.f, 0.f, 1.f };
        Vec4f result = rotation * v;

        REQUIRE_THAT(result.x, WithinAbs(-1.f, kEps_));
        REQUIRE_THAT(result.y, WithinAbs(0.f, kEps_));
        REQUIRE_THAT(result.z, WithinAbs(0.f, kEps_));
        REQUIRE_THAT(result.w, WithinAbs(1.f, kEps_));
    }

    SECTION("Rotation by 180 degrees around Z-axis")
    {
        Mat44f rotation = make_rotation_z(std::numbers::pi_v<float>);  // pi radians = 180 degrees
        Vec4f v = { 1.f, 0.f, 0.f, 1.f };
        Vec4f result = rotation * v;

        REQUIRE_THAT(result.x, WithinAbs(-1.f, kEps_));
        REQUIRE_THAT(result.y, WithinAbs(0.f, kEps_));
        REQUIRE_THAT(result.z, WithinAbs(0.f, kEps_));
        REQUIRE_THAT(result.w, WithinAbs(1.f, kEps_));
    }

    SECTION("Rotation by 270 degrees around X-axis")
    {
        Mat44f rotation = make_rotation_x(3.f * std::numbers::pi_v<float> / 2.f);  // 3 * pi / 2 radians = 270 degrees
        Vec4f v = { 1.f, 0.f, 0.f, 1.f };
        Vec4f result = rotation * v;

        REQUIRE_THAT(result.x, WithinAbs(1.f, kEps_));
        REQUIRE_THAT(result.y, WithinAbs(0.f, kEps_));
        REQUIRE_THAT(result.z, WithinAbs(0.f, kEps_));
        REQUIRE_THAT(result.w, WithinAbs(1.f, kEps_));
    }

    SECTION("Rotation by 270 degrees around Y-axis")
    {
        Mat44f rotation = make_rotation_y(3.f * std::numbers::pi_v<float> / 2.f);  // 3 * pi / 2 radians = 270 degrees
        Vec4f v = { 1.f, 0.f, 0.f, 1.f };
        Vec4f result = rotation * v;

        REQUIRE_THAT(result.x, WithinAbs(0.f, kEps_));
        REQUIRE_THAT(result.y, WithinAbs(0.f, kEps_));
        REQUIRE_THAT(result.z, WithinAbs(1.f, kEps_));
        REQUIRE_THAT(result.w, WithinAbs(1.f, kEps_));
    }

    SECTION("Rotation by 270 degrees around Z-axis")
    {
        Mat44f rotation = make_rotation_z(3.f * std::numbers::pi_v<float> / 2.f);  // 3 * pi / 2 radians = 270 degrees
        Vec4f v = { 1.f, 0.f, 0.f, 1.f };
        Vec4f result = rotation * v;

        REQUIRE_THAT(result.x, WithinAbs(0.f, kEps_));
        REQUIRE_THAT(result.y, WithinAbs(-1.f, kEps_));
        REQUIRE_THAT(result.z, WithinAbs(0.f, kEps_));
        REQUIRE_THAT(result.w, WithinAbs(1.f, kEps_));
    }

    SECTION("Scaling matrix multiplication")
    {
        Mat44f scaleMatrix = make_scaling(2.f, 2.f, 1.f);  // Scaling X and Y by 2
        Vec4f v = { 1.f, 1.f, 1.f, 1.f };
        Vec4f result = scaleMatrix * v;

        REQUIRE(result.x == 2.f);
        REQUIRE(result.y == 2.f);
        REQUIRE(result.z == 1.f);
        REQUIRE(result.w == 1.f);
    }

    SECTION("Translation matrix multiplication")
    {
        Vec3f translation = { 3.f, 5.f, 7.f };
        Mat44f translationMatrix = make_translation(translation);
        Vec4f v = { 1.f, 1.f, 1.f, 1.f };
        Vec4f result = translationMatrix * v;

        REQUIRE(result.x == 4.f);  // 1 + 3
        REQUIRE(result.y == 6.f);  // 1 + 5
        REQUIRE(result.z == 8.f);  // 1 + 7
        REQUIRE(result.w == 1.f); 
    }

    SECTION("Perspective projection matrix multiplication")
    {
        Mat44f perspective = make_perspective_projection(std::numbers::pi_v<float> / 4.f, 1.33f, 0.1f, 100.f);
        Vec4f v = { 1.f, 1.f, 1.f, 1.f };
        Vec4f result = perspective * v;

        REQUIRE(result.x != v.x);
        REQUIRE(result.y != v.y);
        REQUIRE(result.z != v.z);
    }


}
