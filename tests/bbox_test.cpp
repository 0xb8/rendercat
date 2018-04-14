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
		REQUIRE(rc::bbox2::intersects_sphere(bbox2, glm::vec2(0.0f), 10.0f) == rc::Intersection::Outside);
		REQUIRE(rc::bbox3::intersects_sphere(bbox3, glm::vec3(0.0f), 10.0f) == rc::Intersection::Outside);

		REQUIRE(rc::bbox2::intersects(bbox2, bbox2) == rc::Intersection::Outside);
		REQUIRE(rc::bbox3::intersects(bbox3, bbox3) == rc::Intersection::Outside);

		// includes a circle/sphere at origin with radius 10.0
		rc::bbox2 bbox2a;
		bbox2a.include_circle(glm::vec2(0.0f), 10.0f);
		rc::bbox3 bbox3a;
		bbox3a.include_sphere(glm::vec3(0.0f), 10.0f);

		REQUIRE(rc::bbox2::intersects(bbox2, bbox2a) == rc::Intersection::Outside);
		REQUIRE(rc::bbox3::intersects(bbox3, bbox3a) == rc::Intersection::Outside);
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

		rc::bbox2 bbox2;
		rc::bbox3 bbox3;

		glm::vec2 a2(0.0f), b2(1.0f), c2(-1.0f, 10.0f), d2(0.5f, -0.5f), e2(-3.0f, 3.0f);
		glm::vec3 a3(0.0f), b3(-1.0f, -2.0f, -3.0f), c3(5.0f, 3.0f, 4.0f), d3(0.1f, -1.0f, 2.0f), e3(-24.0f, 0.0f, 1.045f);

		SECTION("Normal include") {
			bbox2.include(a2);
			bbox2.include(b2);
			bbox2.include(c2);
			bbox2.include(d2);
			bbox2.include(e2);

			bbox3.include(a3);
			bbox3.include(b3);
			bbox3.include(c3);
			bbox3.include(d3);
			bbox3.include(e3);

			REQUIRE(!bbox2.is_null());
			REQUIRE(bbox2.min() == glm::vec2(-3.0f, -0.5f));
			REQUIRE(bbox2.max() == glm::vec2(1.0f, 10.0f));

			REQUIRE(!bbox3.is_null());
			REQUIRE(bbox3.min() == glm::vec3(-24.0f, -2.0f, -3.0f));
			REQUIRE(bbox3.max() == glm::vec3(5.0f, 3.0f, 4.0f));

		}

		SECTION("Variadic include") {

			bbox2.include(d2);
			bbox2.include(a2, b2, c2, e2);

			bbox3.include(c3);
			bbox3.include(a3, b3, d3, e3);

			REQUIRE(!bbox2.is_null());
			REQUIRE(bbox2.min() == glm::vec2(-3.0f, -0.5f));
			REQUIRE(bbox2.max() == glm::vec2(1.0f, 10.0f));

			REQUIRE(!bbox3.is_null());
			REQUIRE(bbox3.min() == glm::vec3(-24.0f, -2.0f, -3.0f));
			REQUIRE(bbox3.max() == glm::vec3(5.0f, 3.0f, 4.0f));
		}
	}

	SECTION("Include NaN") {

		rc::bbox2 bbox2;
		rc::bbox3 bbox3;

		SECTION("Null bbox should stay null") {
			bbox2.include(glm::vec2(NAN));
			bbox3.include(glm::vec3(NAN));

			REQUIRE(bbox2.is_null());
			REQUIRE(bbox3.is_null());
		}

		glm::vec2 a2(0.0f), b2(1.0f), c2(-1.0f, 10.0f), d2(0.5f, -0.5f), e2(-3.0f, 3.0f);
		glm::vec3 a3(0.0f), b3(-1.0f, -2.0f, -3.0f), c3(5.0f, 3.0f, 4.0f), d3(0.1f, -1.0f, 2.0f), e3(-24.0f, 0.0f, 1.045f);
		bbox2.include(a2, b2, c2, d2, e2);
		bbox3.include(a3, b3, c3, d3, e3);

		auto bbox2_nan = bbox2;
		auto bbox3_nan = bbox3;


		SECTION("Single point") {
			bbox2_nan.include(glm::vec2(NAN, 1.1f));
			bbox2_nan.include(glm::vec2(0.1, NAN));
			bbox2_nan.include(glm::vec2(0.1, std::nanf("1234")));
			bbox2_nan.include(glm::vec2(std::nanf("5678"), 0.1));

			bbox3_nan.include(glm::vec3(NAN, 1.1f, 0.0f));
			bbox3_nan.include(glm::vec3(0.1, NAN, 0.1f));
			bbox3_nan.include(glm::vec3(0.1, 0.1f, NAN));
			bbox3_nan.include(glm::vec3(0.1, std::nanf("1234"), NAN));
			bbox3_nan.include(glm::vec3(std::nanf("5678"), 0.1, NAN));

			REQUIRE(!bbox2_nan.is_null());
			REQUIRE(bbox2_nan.min() == bbox2.min());
			REQUIRE(bbox2_nan.max() == bbox2.max());

			REQUIRE(!bbox3_nan.is_null());
			REQUIRE(bbox3_nan.min() == bbox3.min());
			REQUIRE(bbox3_nan.max() == bbox3.max());
		}

		SECTION("Variadic include") {
			bbox2_nan.include(glm::vec2(0.0f, 1.1f), glm::vec2(NAN, 1.0f), glm::vec2(1.0f, NAN), glm::vec2(NAN, NAN));
			bbox2_nan.include(glm::vec2(NAN, NAN), glm::vec2(NAN, 1.0f), glm::vec2(1.0f, NAN), glm::vec2(0.0f, 1.1f));

			bbox3_nan.include(glm::vec3(std::nanf("5678"), 0.1, NAN),
			                  glm::vec3( NAN, 1.1f, 0.0f),
			                  glm::vec3(0.1, NAN, 0.1f),
			                  glm::vec3(0.1, std::nanf("1234"), NAN),
			                  glm::vec3(0.1, 0.1f, NAN));

			REQUIRE(!bbox2_nan.is_null());
			REQUIRE(bbox2_nan.min() == bbox2.min());
			REQUIRE(bbox2_nan.max() == bbox2.max());

			REQUIRE(!bbox3_nan.is_null());
			REQUIRE(bbox3_nan.min() == bbox3.min());
			REQUIRE(bbox3_nan.max() == bbox3.max());
		}

		SECTION("Bbox") {
			// we can't actually create NaN bbox, so instead we use fake one
			struct NanBbox2
			{
				glm::vec2 min;
				glm::vec2 max;
			};

			struct NanBbox3
			{
				glm::vec3 min;
				glm::vec3 max;
			};

			NanBbox2 fakebbox2{glm::vec2(NAN, 0.0f), glm::vec2(0.0f, NAN)};
			NanBbox2 fakebbox22{glm::vec2(0.0f, NAN), glm::vec2(NAN, 0.0f)};
			NanBbox2 fakebbox23{glm::vec2(NAN), glm::vec2(NAN)};
			NanBbox3 fakebbox3{glm::vec3(NAN, 0.0f, NAN), glm::vec3(0.0f, NAN, 0.0f)};
			NanBbox3 fakebbox32{glm::vec3(0.0f, NAN, NAN), glm::vec3(NAN, NAN, 0.0f)};
			NanBbox3 fakebbox33{glm::vec3(NAN, 0.0f, 0.0), glm::vec3(NAN, NAN, NAN)};

			// dirty hack, if tests fail thats why
			bbox2_nan.include(*reinterpret_cast<rc::bbox2*>(&fakebbox2));
			bbox2_nan.include(*reinterpret_cast<rc::bbox2*>(&fakebbox22));
			bbox2_nan.include(*reinterpret_cast<rc::bbox2*>(&fakebbox23));

			bbox3_nan.include(*reinterpret_cast<rc::bbox3*>(&fakebbox3));
			bbox3_nan.include(*reinterpret_cast<rc::bbox3*>(&fakebbox32));
			bbox3_nan.include(*reinterpret_cast<rc::bbox3*>(&fakebbox33));

			SECTION("Nan bbox should become null") {
				REQUIRE(reinterpret_cast<rc::bbox2*>(&fakebbox2)->is_null());
				REQUIRE(reinterpret_cast<rc::bbox2*>(&fakebbox22)->is_null());
				REQUIRE(reinterpret_cast<rc::bbox2*>(&fakebbox23)->is_null());

				REQUIRE(reinterpret_cast<rc::bbox3*>(&fakebbox3)->is_null());
				REQUIRE(reinterpret_cast<rc::bbox3*>(&fakebbox32)->is_null());
				REQUIRE(reinterpret_cast<rc::bbox3*>(&fakebbox33)->is_null());

				SECTION("And stay null") {
					auto bbox2 = *reinterpret_cast<rc::bbox2*>(&fakebbox2);
					bbox2.include(glm::vec2(1213.f, 32242.f), glm::vec2(-223.f,-54.32f));

					auto bbox3 = *reinterpret_cast<rc::bbox3*>(&fakebbox3);
					bbox3.include(glm::vec3(1213.f, 32242.f, 12312.f), glm::vec3(-223.f,-54.32f, -232.f));

					REQUIRE(bbox2.is_null());
					REQUIRE(bbox3.is_null());
				}
			}

			REQUIRE(!bbox2_nan.is_null());
			REQUIRE(bbox2_nan.min() == bbox2.min());
			REQUIRE(bbox2_nan.max() == bbox2.max());

			REQUIRE(!bbox3_nan.is_null());
			REQUIRE(bbox3_nan.min() == bbox3.min());
			REQUIRE(bbox3_nan.max() == bbox3.max());

		}

		SECTION("Circle") {

			bbox2_nan.include_circle(glm::vec2(12.0f), NAN);
			bbox2_nan.include_circle(glm::vec2(43.0f, NAN), NAN);
			bbox2_nan.include_circle(glm::vec2(NAN, 1.0f), NAN);
			bbox2_nan.include_circle(glm::vec2(NAN), NAN);

			REQUIRE(!bbox2_nan.is_null());
			REQUIRE(bbox2_nan.min() == bbox2.min());
			REQUIRE(bbox2_nan.max() == bbox2.max());
		}
	}
}

TEST_CASE("BBox extend") {

		rc::bbox2 bbox2;
		bbox2.include(glm::vec2(10.0f, -10.0f));
		bbox2.include(glm::vec2(0.0f, 10.0f));

		rc::bbox3 bbox3;
		bbox3.include(glm::vec3(10.0f, -10.0f, 0.0f));
		bbox3.include(glm::vec3(0.0f, 10.0f, -10.0f));

		rc::bbox2 bbox2_ex = bbox2;
		rc::bbox3 bbox3_ex = bbox3;

		SECTION("Zero") {

			bbox2_ex.extend(0.0f);
			bbox3_ex.extend(0.0f);

			REQUIRE(bbox2_ex.min() == bbox2.min());
			REQUIRE(bbox2_ex.max() == bbox2.max());

			REQUIRE(bbox3_ex.min() == bbox3.min());
			REQUIRE(bbox3_ex.max() == bbox3.max());
		}

		SECTION("Positive") {
			bbox2_ex.extend(10.0f);
			bbox3_ex.extend(10.0f);

			REQUIRE(bbox2_ex.min() == glm::vec2(-10.0f, -20.0f));
			REQUIRE(bbox2_ex.max() == glm::vec2(20.0f, 20.0f));

			REQUIRE(bbox3_ex.min() == glm::vec3(-10.0f, -20.0f, -20.0f));
			REQUIRE(bbox3_ex.max() == glm::vec3(20.0f, 20.0f, 10.0f));
		}

		SECTION("Negative") {
			bbox2_ex.extend(-5.0f);
			bbox3_ex.extend(-5.0f);

			REQUIRE(bbox2_ex.min() == glm::vec2(5.0f, -5.0f));
			REQUIRE(bbox2_ex.max() == glm::vec2(5.0f, 5.0f));

			REQUIRE(bbox3_ex.min() == glm::vec3(5.0f, -5.0f, -5.0f));
			REQUIRE(bbox3_ex.max() == glm::vec3(5.0f, 5.0f, -5.0f));
		}
}
