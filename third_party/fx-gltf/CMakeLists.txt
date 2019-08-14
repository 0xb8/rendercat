add_library(fx-gltf INTERFACE)
add_library(fx-gltf::fx-gltf ALIAS fx-gltf)
target_include_directories(fx-gltf INTERFACE fx-gltf/include)
target_link_libraries(fx-gltf INTERFACE nlohmann_json::nlohmann_json)
