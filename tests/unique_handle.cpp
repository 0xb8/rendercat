#include <rendercat/util/unique_handle.hpp>
#include <catch.hpp>
#include <utility>

namespace {

struct NullDeleter
{
	void operator()(int val) noexcept
	{

	}
};

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

		SECTION("Resetting valid handle results in deleter being called") {
			handle.reset();
			REQUIRE(!handle);
			REQUIRE(*handle == 0);
			REQUIRE(instance_count == 0);

			SECTION("Resetting empty handle does not call deleter") {
				handle.reset();
				REQUIRE(!handle);
				REQUIRE(*handle == 0);
				REQUIRE(instance_count == 0);
			}

		}

		SECTION("Reset() can be used to take ownership of new handle") {
			handle.reset(create_counted_handle<int>(1234));
			REQUIRE(handle);
			REQUIRE(*handle == 1234);
			REQUIRE(instance_count == 1);
		}

	}
	REQUIRE(instance_count == 0);
}

TEST_CASE("Handle Release") {

	SECTION("Preparing handle"){
		rc::unique_handle<int, CountedDeleter> handle(create_counted_handle<int>());
		REQUIRE(handle);
		REQUIRE(*handle == 42);
		REQUIRE(instance_count == 1);

		SECTION("Releasing handle will not run deleter") {
			auto val = handle.release();
			REQUIRE(val == 42);
			REQUIRE(!handle);
			REQUIRE(instance_count == 1);

			SECTION("Released handle can be reset") {
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

	SECTION("Handles can be swapped") {
		REQUIRE(*handle_a == 111);
		REQUIRE(*handle_b == 222);

		std::swap(handle_a, handle_b);

		REQUIRE(*handle_a == 222);
		REQUIRE(*handle_b == 111);
		REQUIRE(instance_count == 2);
	}

	SECTION("Handle can be move-constructed from another") {
		rc::unique_handle<int, CountedDeleter> target(std::move(handle_a));
		REQUIRE(target);
		REQUIRE(*target == 111);
		REQUIRE(!handle_a);
		REQUIRE(instance_count == 2);
	}

	SECTION("Handle can be move-assigned from another") {
		SECTION("Handle cannot be move-assigned from itself") {
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
