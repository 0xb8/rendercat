#include <atomic>
#include <glbinding-aux/ContextInfo.h>
#include <glbinding-aux/Meta.h>
#include <rendercat/util/gl_meta.hpp>
#include <rendercat/util/unique_file_handle.hpp>
#include <fmt/core.h>

static std::set<gl::GLextension> extensions;
static std::atomic_flag extensions_ready;

std::string rc::glmeta::renderer()
{
	return glbinding::aux::ContextInfo::renderer();
}

std::string rc::glmeta::vendor()
{
	return glbinding::aux::ContextInfo::vendor();
}

bool rc::glmeta::extension_supported(gl::GLextension ext)
{
	if(!extensions_ready.test_and_set()) {
		extensions = glbinding::aux::ContextInfo::extensions();
	}
	return extensions.find(ext) != extensions.end();
}

std::string rc::glmeta::supported_extensions()
{
	if(!extensions_ready.test_and_set()) {
		extensions = glbinding::aux::ContextInfo::extensions();
	}
	std::string res;
	res.reserve(extensions.size() * 96);
	for(const auto& ext : extensions) {
		res.append(glbinding::aux::Meta::getString(ext));
		res.push_back('\n');
	}
	return res;
}

void rc::glmeta::log_all_supported_extensions(const std::string_view filename)
{
	rc::file_handle file(std::fopen(filename.data(), "w"));
	if(!file) return;

	auto str = supported_extensions();

	auto write_string = [](const std::string& str, rc::file_handle& file)
	{
		assert(file);
		std::fwrite(str.data(), sizeof(char), str.size(), *file);
	};

	write_string(fmt::format("renderer:   {}\n", renderer()), file);
	write_string(fmt::format("vendor:     {}\n", vendor()), file);
	write_string(fmt::format("extensions: {} known extensions\n\n{}\n", extensions.size(), str), file);
	std::fflush(*file);
}

void rc::glmeta::require_extension(gl::GLextension ext)
{
	if(!extension_supported(ext)) {
		throw std::runtime_error(
			fmt::format("required OpenGL extension is not supported by {} ({}): {}\n",
				renderer(), vendor(), glbinding::aux::Meta::getString(ext)));
	}
}
