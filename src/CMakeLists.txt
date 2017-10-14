add_executable(bidmp
	main.cpp
	bidmp.cpp
	mdmp.cpp
)

target_link_libraries(bidmp ${CONAN_LIBS})
target_compile_definitions(bidmp PRIVATE _HAS_AUTO_PTR_ETC=1)
