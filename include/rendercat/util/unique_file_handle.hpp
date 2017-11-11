#pragma once
#include "unique_handle.hpp"
#include <cstdio>

namespace rc {

struct FileDeleter
{
	void operator()(std::FILE* f) noexcept
	{
		std::fclose(f);
	}
};

using file_handle = rc::unique_handle<std::FILE*, FileDeleter>;

}

