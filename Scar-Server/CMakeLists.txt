cmake_minimum_required(VERSION 3.16)

set(CMAKE_PREFIX_PATH "D:/Qt/6.11.1/msvc2022_64")

project(SCat-Server LANGUAGES CXX)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(CMAKE_AUTOMOC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network Sql)

add_executable(SCat-Server
    "main.cpp"
    "Protocol.h"
    "Server/include/Server.h"
    "Server/src/Server.cpp"
    "Server/include/ClientSession.h"
    "Server/src/ClientSession.cpp"
 "Database/include/Database.h" "Database/src/Database.cpp")

target_link_libraries(SCat-Server PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Sql
)

# TODO: 如有需要，请添加测试并安装目标。
