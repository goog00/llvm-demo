add_executable(
    exercise
    # test.cpp

    PhiCreate.cpp
)
target_link_libraries(exercise PRIVATE ${llvm_libs})