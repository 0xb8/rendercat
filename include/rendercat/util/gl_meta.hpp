#pragma once
#include <string>
#include <string_view>
#include <glbinding/gl/extension.h>

namespace rc { namespace glmeta {

	std::string renderer();
	std::string vendor();
	std::string supported_extensions();

	bool extension_supported(gl::GLextension);
	void require_extension(gl::GLextension);

	void log_all_supported_extensions(const std::string_view file);

}}
