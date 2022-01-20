cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(tapp)

set(SDL2_DIR ${CMAKE_SOURCE_DIR}/SDL/dist/lib/cmake/SDL2)

# rapidjson stuff
add_definitions(
        -DRAPIDJSON_ALLOCATOR_DEFAULT_CHUNK_CAPACITY=4096 # rapidjson
        -DRAPIDJSON_HAS_STDSTRING=1 # rapidjson
)
file(GLOB RAPIDJSON_FILES
        ${CMAKE_SOURCE_DIR}/../components/rapidjson/rapidjson/*.h
        )

# link boost statically
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.67.0 COMPONENTS system thread filesystem program_options REQUIRED)

## web server stuff
add_library(simple-web-server INTERFACE)
target_include_directories(simple-web-server INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../simulator/Simple-Web-Server)
find_package(Threads REQUIRED)
target_link_libraries(simple-web-server INTERFACE ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(simple-web-server INTERFACE ${Boost_LIBRARIES})
target_include_directories(simple-web-server INTERFACE ${Boost_INCLUDE_DIR})
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
    target_compile_definitions(simple-web-server INTERFACE USE_BOOST_REGEX)
    find_package(Boost 1.67.0 COMPONENTS regex REQUIRED)
    target_link_libraries(simple-web-server INTERFACE ${Boost_LIBRARIES})
    target_include_directories(simple-web-server INTERFACE ${Boost_INCLUDE_DIR})
endif()

# libserial stuff
set(LIBSER_SRC libserialport/serialport.c libserialport/timing.c)
if(WIN32)
    set(LIBSER_SRC ${LIBSER_SRC} libserialport/windows.c)
elseif(APPLE)
    set(LIBSER_SRC ${LIBSER_SRC} libserialport/macosx.c)
else()
    set(LIBSER_SRC ${LIBSER_SRC} libserialport/linux.c)
endif()
set(LIBSER_HDR libserialport/libserialport.h)
file(WRITE ${CMAKE_BINARY_DIR}/gen_include/config.h "// dummy file for libserial")
add_library(libserialport STATIC ${LIBSER_SRC})
target_compile_definitions(libserialport PRIVATE SP_PRIV=)
target_include_directories(libserialport PRIVATE ${LIBSER_HDR} ${CMAKE_BINARY_DIR}/gen_include)
if(APPLE)
    target_link_libraries(libserialport PRIVATE "-framework IOKit" "-framework CoreFoundation")
endif()

# imgui stuff
find_package(SDL2 REQUIRED)
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/imgui)
set(IMGUI_SRC ${CMAKE_SOURCE_DIR}/FileBrowser/ImGuiFileBrowser.cpp ${IMGUI_DIR}/backends/imgui_impl_sdl.cpp ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp ${IMGUI_DIR}/imgui.cpp ${IMGUI_DIR}/imgui_draw.cpp ${IMGUI_DIR}/imgui_tables.cpp ${IMGUI_DIR}/imgui_widgets.cpp)
set(IMGUI_HDR ${IMGUI_DIR} ${IMGUI_DIR}/backends ${CMAKE_SOURCE_DIR}/FileBrowser)

#tapp
file(GLOB SRC_FILES *.cpp *.hpp)
add_executable(tapp ${SRC_FILES} ${CMAKE_SOURCE_DIR}/../simulator/fake-idf/esp_heap_caps.c ${IMGUI_SRC})
#target_link_options(tapp PRIVATE -static-libgcc -static-libstdc++)

if(WIN32)
    target_link_libraries(tapp -static simple-web-server)
    target_link_libraries(tapp -static ${Boost_LIBRARIES})
    target_link_libraries(tapp -static ws2_32 wsock32)
    target_link_libraries(tapp dl)
elseif(APPLE)
    target_link_libraries(tapp simple-web-server)
    target_link_libraries(tapp ${Boost_LIBRARIES})
    target_link_libraries(tapp iconv)
    target_link_options(tapp PRIVATE
            "SHELL:-framework OpenGL"
            "SHELL:-framework Cocoa"
            "SHELL:-framework IOKit"
            "SHELL:-framework CoreVideo"
            "SHELL:-framework Carbon"
            "SHELL:-framework ForceFeedback")
else() #linux etc?!
    target_link_libraries(tapp simple-web-server)
    target_link_libraries(tapp ${Boost_LIBRARIES})
    target_link_libraries(tapp -static gdi32 opengl32 imm32)
endif()
target_include_directories(tapp PRIVATE ${IMGUI_HDR} ${SDL2_INCLUDE_DIRS})
target_link_libraries(tapp ${SDL2_LIBRARIES})
target_link_libraries(tapp libserialport)
target_include_directories(tapp PRIVATE ${Boost_INCLUDE_DIR})
target_include_directories(tapp PRIVATE ${CMAKE_SOURCE_DIR}/../components/rapidjson)
target_include_directories(tapp PRIVATE ${CMAKE_SOURCE_DIR}/../simulator/fake-idf)
target_include_directories(tapp PRIVATE libserialport)
add_dependencies(tapp copy-www copy-bin)

# copy spiffs files to build folder
add_custom_target(copy-www
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/../spiffs_image/www
        ${CMAKE_BINARY_DIR}/www
        )

# copy bin files to build folder
add_custom_target(copy-bin
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/../bin
        ${CMAKE_BINARY_DIR}/bin
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/../sample_rom/sample-rom.tbd
        ${CMAKE_BINARY_DIR}/bin
        )

install(CODE "set(CMAKE_INSTALL_LOCAL_ONLY true)")
install(TARGETS tapp RUNTIME DESTINATION .)
install(DIRECTORY ${CMAKE_BINARY_DIR}/www DESTINATION .)
install(DIRECTORY ${CMAKE_BINARY_DIR}/bin DESTINATION . USE_SOURCE_PERMISSIONS)
install(FILES ${CMAKE_SOURCE_DIR}/readme.md DESTINATION .)

# package generation
set(CPACK_GENERATOR "ZIP")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "tapp")
SET(CPACK_PACKAGE_VENDOR "ctag-tbd")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/readme.md")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE")

SET(CPACK_PACKAGE_VERSION_MAJOR "1")
SET(CPACK_PACKAGE_VERSION_MINOR "0")
SET(CPACK_PACKAGE_VERSION_PATCH "0")
#SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_SYSTEM_NAME}")
include(CPack)