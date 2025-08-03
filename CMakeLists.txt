cmake_minimum_required(VERSION 3.14)
project(WayFromPiterToHome)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(lib/crt-w-t-hm)
add_subdirectory(lib/roadTgBot)

add_executable(WayFromPiterToHome bin/main.cpp)

include(FetchContent)


FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.10.5
)
FetchContent_MakeAvailable(cpr)

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

FetchContent_Declare(
    TgBot
    GIT_REPOSITORY https://github.com/reo7sp/tgbot-cpp.git
    GIT_TAG master
)
FetchContent_MakeAvailable(TgBot)

set(Boost_INCLUDE_DIR /usr/include)
set(Boost_LIBRARY_DIR /usr/lib64)

find_package(Boost REQUIRED COMPONENTS system)

include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})

find_package(OpenSSL REQUIRED)
set(CURL_DIR /usr/lib64/cmake/CURL)
find_package(CURL REQUIRED)

include_directories(/usr/local/include)
link_directories(/usr/local/lib)


target_link_libraries(WayFromPiterToHome PRIVATE
    TgBot
    Boost::system
    OpenSSL::SSL OpenSSL::Crypto
    CURL::libcurl
    pthread
    create_way
    cpr::cpr
    nlohmann_json::nlohmann_json
    roadTgBot
)

include_directories(${PROJECT_SOURCE_DIR}/lib/crt-w-t-hm)
include_directories(${PROJECT_SOURCE_DIR}/lib/roadTgBot)
