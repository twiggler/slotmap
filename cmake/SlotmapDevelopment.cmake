# Settings for slotmap's own targets (tests, demo, benchmarks): strict warnings and optional clang-tidy.
# Third-party code pulled in with FetchContent is not affected.

if(SLOTMAP_CLANG_TIDY)
	find_program(SLOTMAP_CLANG_TIDY_EXE NAMES clang-tidy REQUIRED)
endif()

function(slotmap_configure_target target)
	if(MSVC)
		target_compile_options(${target} PRIVATE /W4 /permissive- /WX /Zc:__cplusplus /utf-8)
	else()
		target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Werror)
		if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
			# Narrowing integer conversions: what MSVC reports at /W4 (C4244, C4267), so they are caught locally.
			target_compile_options(${target} PRIVATE -Wimplicit-int-conversion -Wshorten-64-to-32)
		endif()
	endif()

	if(SLOTMAP_CLANG_TIDY)
		set_target_properties(${target} PROPERTIES CXX_CLANG_TIDY "${SLOTMAP_CLANG_TIDY_EXE}")
	endif()
endfunction()
