add_executable(
    exercise
    # test.cpp
    # PhiCreate.cpp
    Align.cpp
)
target_link_libraries(exercise PRIVATE ${llvm_libs})