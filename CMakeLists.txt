file (GLOB sources ${CMAKE_CURRENT_SOURCE_DIR}/sources/*.cpp)
file (GLOB headers ${CMAKE_CURRENT_SOURCE_DIR}/kittens_trace/*.h)
add_library(kittens_trace ${sources} ${headers})

target_include_directories(kittens_trace PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

