add_executable(bidump
	main.cpp
	bidmp.cpp
	mdmp.cpp
)

target_link_libraries(bidump ${CONAN_LIBS})
target_compile_definitions(bidump PRIVATE _HAS_AUTO_PTR_ETC=1)
