#include <rendercat/core/bbox.hpp>
#include <catch.hpp>

TEST_CASE("BBox construction") {

	rc::bbox2 bbox2;
	rc::bbox3 bbox3;

	SECTION("Default constructed bbox is null") {
		REQUIRE(bbox2.is_null());
		REQUIRE(bbox3.is_null());
	}

	SECTION("Default constructed bbox's center and diagnonal are zero-vectors") {
		REQUIRE(bbox2.center() == glm::vec2(0.0));
		REQUIRE(bbox3.center() == glm::vec3(0.0));

		REQUIRE(bbox2.diagonal() == glm::vec2(0.0));
		REQUIRE(bbox3.diagonal() == glm::vec3(0.0));
	}

	SECTION("Default constructed bbox does not intersect with anything, even itself") {
		REQUIRE(rc::bbox2::intersects_sphere(bbox2, glm::vec2(0.0f), 10.0f) == rc::bbox2::Intersection::Outside);
		REQUIRE(rc::bbox3::intersects_sphere(bbox3, glm::vec3(0.0f), 10.0f) == rc::bbox3::Intersection::Outside);

		REQUIRE(rc::bbox2::intersects(bbox2, bbox2) == rc::bbox2::Intersection::Outside);
		REQUIRE(rc::bbox3::intersects(bbox3, bbox3) == rc::bbox3::Intersection::Outside);

		// includes a circle/sphere at origin with radius 10.0
		rc::bbox2 bbox2a;
		bbox2a.include_circle(glm::vec2(0.0f), 10.0f);
		rc::bbox3 bbox3a;
		bbox3a.include_sphere(glm::vec3(0.0f), 10.0f);

		REQUIRE(rc::bbox2::intersects(bbox2, bbox2a) == rc::bbox2::Intersection::Outside);
		REQUIRE(rc::bbox3::intersects(bbox3, bbox3a) == rc::bbox3::Intersection::Outside);
	}
}

TEST_CASE("BBox include") {

	rc::bbox2 bbox2;
	rc::bbox3 bbox3;

	SECTION("single point in null bbox") {
		REQUIRE(bbox2.is_null());

		const auto point2 = glm::vec2(3.0f, -2.0f);
		const auto point3 = glm::vec3(3.0f, -2.0f, 1.0f);

		bbox2.include(point2);
		bbox3.include(point3);

		SECTION("Bbox with a point included is not null") {
			REQUIRE(!bbox2.is_null());
			REQUIRE(!bbox3.is_null());
		}

		SECTION("Bbox with a single point has min and max equal to that point") {
			REQUIRE(bbox2.min() == point2);
			REQUIRE(bbox2.max() == point2);
			REQUIRE(bbox3.min() == point3);
			REQUIRE(bbox3.max() == point3);
		}

		SECTION("Bbox with a single point has ranges equal to that point") {
			REQUIRE(bbox2.x_range().min == point2.x);
			REQUIRE(bbox2.x_range().max == point2.x);
			REQUIRE(bbox2.y_range().min == point2.y);
			REQUIRE(bbox2.y_range().max == point2.y);
			REQUIRE(bbox3.x_range().min == point3.x);
			REQUIRE(bbox3.x_range().max == point3.x);
			REQUIRE(bbox3.y_range().min == point3.y);
			REQUIRE(bbox3.y_range().max == point3.y);
			REQUIRE(bbox3.z_range().min == point3.z);
			REQUIRE(bbox3.z_range().max == point3.z);
		}

		SECTION("Bbox with a single point has zero diagonal") {
			REQUIRE(bbox2.diagonal() == glm::vec2(0.0));
			REQUIRE(bbox3.diagonal() == glm::vec3(0.0));
		}

		SECTION("Bbox with a single point has center in that point") {
			REQUIRE(bbox2.center() == point2);
			REQUIRE(bbox3.center() == point3);
		}

		SECTION("Closest point on bbox with a single point is that point") {
			REQUIRE(bbox2.closest_point(glm::vec2(123.4f, 567.8f)) == point2);
			REQUIRE(bbox3.closest_point(glm::vec3(123.4f, 567.8f, 910.11f)) == point3);
		}
	}

	SECTION("multiple points") {

		SECTION("of 2D BBox") {
			rc::bbox2 bbox;

			glm::vec2 a(0.0f), b(1.0f), c(-1.0f, 10.0f), d(0.5f, -0.5f), e(-3.0f, 3.0f);

			SECTION("Normal include") {
				bbox.include(a);
				bbox.include(b);
				bbox.include(c);
				bbox.include(d);
				bbox.include(e);

				REQUIRE(!bbox.is_null());
				REQUIRE(bbox.min() == glm::vec2(-3.0f, -0.5f));
				REQUIRE(bbox.max() == glm::vec2(1.0f, 10.0f));

			}

			SECTION("Variadic include") {

				bbox.include(d);
				bbox.include(a, b, c, e);

				REQUIRE(!bbox.is_null());
				REQUIRE(bbox.min() == glm::vec2(-3.0f, -0.5f));
				REQUIRE(bbox.max() == glm::vec2(1.0f, 10.0f));
			}

		}

		SECTION("of 3D BBox") {
			rc::bbox3 bbox;

			glm::vec3 a(0.0f), b(-1.0f, -2.0f, -3.0f), c(5.0f, 3.0f, 4.0f), d(0.1f, -1.0f, 2.0f), e(-24.0f, 0.0f, 1.045f);

			SECTION("Normal include") {

				bbox.include(a);
				bbox.include(b);
				bbox.include(c);
				bbox.include(d);
				bbox.include(e);

				REQUIRE(!bbox.is_null());
				REQUIRE(bbox.min() == glm::vec3(-24.0f, -2.0f, -3.0f));
				REQUIRE(bbox.max() == glm::vec3(5.0f, 3.0f, 4.0f));
			}

			SECTION("Variadic include") {

				bbox.include(c);
				bbox.include(a, b, d, e);

				REQUIRE(!bbox.is_null());
				REQUIRE(bbox.min() == glm::vec3(-24.0f, -2.0f, -3.0f));
				REQUIRE(bbox.max() == glm::vec3(5.0f, 3.0f, 4.0f));
			}
		}
	}
}
