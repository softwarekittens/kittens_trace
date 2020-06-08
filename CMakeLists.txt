set(NAME kittens_trace)

file (GLOB sources ${CMAKE_CURRENT_SOURCE_DIR}/sources/*.cpp)
file (GLOB headers ${CMAKE_CURRENT_SOURCE_DIR}/kittens_trace/*.h)
add_library(${NAME} ${sources} ${headers})

target_include_directories(${NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

