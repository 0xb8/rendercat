#include <rendercat/util/gl_unique_handle.hpp>
#include <glbinding/gl45core/functions.h>

using namespace gl45core;

void rc::BufferDeleter::operator()(uint32_t h) noexcept
{
	glDeleteBuffers(1, &h);
}

void rc::FrameBufferDeleter::operator()(uint32_t h) noexcept
{
	glDeleteFramebuffers(1, &h);
}

void rc::ProgramDeleter::operator()(uint32_t h) noexcept
{
	glDeleteProgram(h);
}

void rc::QueryDeleter::operator()(uint32_t h) noexcept
{
	glDeleteQueries(1, &h);
}

void rc::ShaderDeleter::operator()(uint32_t h) noexcept
{
	glDeleteShader(h);
}

void rc::TextureDeleter::operator()(uint32_t h) noexcept
{
	glDeleteTextures(1, &h);
}

void rc::VertexArrayDeleter::operator()(uint32_t h) noexcept
{
	glDeleteVertexArrays(1, &h);
}


// -----------------------------------------------------------------------------
#include <doctest/doctest.h>
#ifndef DOCTEST_CONFIG_DISABLE
namespace {

static int instance_count;
struct CountedDeleter
{
	void operator()(int) noexcept
	{
		--instance_count;
	}
};

template<typename T>
static T create_counted_handle(T val = T{42})
{
	++instance_count;
	return val;
}

}


TEST_CASE("Handle construction") {
	struct NullDeleter
	{
		void operator()(int) noexcept { }
	};
	rc::unique_handle<int, NullDeleter> handle;

	REQUIRE(!handle);
	REQUIRE(*handle == 0);
	REQUIRE(handle.get() != nullptr);
	*handle = 123;
	REQUIRE(handle);
	REQUIRE(*handle == 123);
	REQUIRE(*handle.get() == 123);
}

TEST_CASE("Handle Destruction") {
	{
		rc::unique_handle<int, CountedDeleter> handle(create_counted_handle<int>());

		REQUIRE(handle);
		REQUIRE(*handle == 42);
		REQUIRE(instance_count == 1);
	}
	REQUIRE(instance_count == 0);
}

TEST_CASE("Handle Reset") {
	{
		rc::unique_handle<int, CountedDeleter> handle(create_counted_handle<int>());
		REQUIRE(handle);
		REQUIRE(*handle == 42);
		REQUIRE(instance_count == 1);

		SUBCASE("Resetting valid handle results in deleter being called") {
			handle.reset();
			REQUIRE(!handle);
			REQUIRE(*handle == 0);
			REQUIRE(instance_count == 0);

			SUBCASE("Resetting empty handle does not call deleter") {
				handle.reset();
				REQUIRE(!handle);
				REQUIRE(*handle == 0);
				REQUIRE(instance_count == 0);
			}

		}

		SUBCASE("Reset() can be used to take ownership of new handle") {
			handle.reset(create_counted_handle<int>(1234));
			REQUIRE(handle);
			REQUIRE(*handle == 1234);
			REQUIRE(instance_count == 1);
		}

	}
	REQUIRE(instance_count == 0);
}

TEST_CASE("Handle Release") {

	SUBCASE("Preparing handle"){
		rc::unique_handle<int, CountedDeleter> handle(create_counted_handle<int>());
		REQUIRE(handle);
		REQUIRE(*handle == 42);
		REQUIRE(instance_count == 1);

		SUBCASE("Releasing handle will not run deleter") {
			auto val = handle.release();
			REQUIRE(val == 42);
			REQUIRE(!handle);
			REQUIRE(instance_count == 1);

			SUBCASE("Released handle can be reset") {
				handle.reset(create_counted_handle<int>(3456));
				REQUIRE(handle);
				REQUIRE(*handle == 3456);
				REQUIRE(instance_count == 2);
			}
		}
	}

	REQUIRE(instance_count == 1);
	instance_count = 0;
}

TEST_CASE("Handle move/swap/assign") {

	rc::unique_handle<int, CountedDeleter> handle_a(create_counted_handle<int>(111));
	rc::unique_handle<int, CountedDeleter> handle_b(create_counted_handle<int>(222));
	REQUIRE(handle_a);
	REQUIRE(handle_b);
	REQUIRE(instance_count == 2);

	SUBCASE("Handles can be swapped") {
		REQUIRE(*handle_a == 111);
		REQUIRE(*handle_b == 222);

		std::swap(handle_a, handle_b);

		REQUIRE(*handle_a == 222);
		REQUIRE(*handle_b == 111);
		REQUIRE(instance_count == 2);
	}

	SUBCASE("Handle can be move-constructed from another") {
		rc::unique_handle<int, CountedDeleter> target(std::move(handle_a));
		REQUIRE(target);
		REQUIRE(*target == 111);
		REQUIRE(!handle_a);
		REQUIRE(instance_count == 2);
	}

	SUBCASE("Handle can be move-assigned from another") {
		SUBCASE("Handle cannot be move-assigned from itself") {
			handle_a = std::move(handle_a);
			REQUIRE(handle_a);
			REQUIRE(*handle_a == 111);
			REQUIRE(instance_count == 2);
		}

		handle_a = std::move(handle_b);
		REQUIRE(handle_a);
		REQUIRE(!handle_b);
		REQUIRE(*handle_a == 222);
		REQUIRE(instance_count == 1);
	}
}

TEST_CASE("Handle comparison") {
	rc::unique_handle<int, CountedDeleter> handle_a(create_counted_handle<int>(123)),
	                                       handle_b(create_counted_handle<int>(345));
	REQUIRE(handle_a);
	REQUIRE(handle_b);
	REQUIRE(instance_count == 2);

	REQUIRE(handle_a != handle_b);
	REQUIRE(handle_a <= handle_b);
	REQUIRE_FALSE(handle_a == handle_b);
	REQUIRE_FALSE(handle_a > handle_b);

	*handle_a = 42;
	*handle_b = 42;

	REQUIRE(handle_a == handle_b);
	REQUIRE_FALSE(handle_a != handle_b);

	REQUIRE(handle_a <= handle_b);
	REQUIRE(handle_a >= handle_b);
	REQUIRE(handle_b <= handle_a);
	REQUIRE(handle_b >= handle_a);
	REQUIRE_FALSE(handle_a < handle_b);
	REQUIRE_FALSE(handle_b < handle_a);
}
#endif // DOCTEST_CONFIG_DISABLE
