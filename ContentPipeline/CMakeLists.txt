add_executable(ContentPipeline2 ContentPipeline.cpp "TexturePacker.cpp")
find_package(nlohmann_json CONFIG REQUIRED)

find_package(torch REQUIRED)
target_link_libraries(ContentPipeline2 PUBLIC torch)

target_link_libraries(ContentPipeline2 PUBLIC Strife.Engine)
target_link_libraries(ContentPipeline2 PUBLIC tmxlite)
target_link_libraries(ContentPipeline2 PRIVATE nlohmann_json nlohmann_json::nlohmann_json)

#target_include_directories(ContentPipeline PRIVATE "../../tmxlite/tmxlite/include")

set_property(TARGET ContentPipeline2 PROPERTY CXX_STANDARD 17)
