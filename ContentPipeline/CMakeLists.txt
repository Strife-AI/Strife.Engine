add_executable(ContentPipeline ContentPipeline.cpp "TexturePacker.cpp")
find_package(nlohmann_json CONFIG REQUIRED)

find_package(torch REQUIRED)
target_link_libraries(ContentPipeline PUBLIC torch)

target_link_libraries(ContentPipeline PUBLIC Strife.Engine)
target_link_libraries(ContentPipeline PUBLIC tmxlite)
target_link_libraries(ContentPipeline PRIVATE nlohmann_json nlohmann_json::nlohmann_json)

#target_include_directories(ContentPipeline PRIVATE "../../tmxlite/tmxlite/include")

set_property(TARGET ContentPipeline PROPERTY CXX_STANDARD 17)
