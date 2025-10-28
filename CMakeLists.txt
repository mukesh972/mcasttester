cmake_minimum_required(VERSION 3.16)
project(mctester)

set(TARGET "mcasttester")

file(GLOB SOURCES "src/*.cpp")
add_executable(${TARGET} ${SOURCES})

# Optional: if you have headers in src/
include_directories(src)
target_link_libraries(${TARGET} -ljsoncpp -lcurl)
# ✅ Add install rule
install(TARGETS ${TARGET} DESTINATION bin)
