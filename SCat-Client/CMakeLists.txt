# 1. 升级最低版本 (Qt6 需要 3.16+)
cmake_minimum_required(VERSION 3.16)
set(CMAKE_PREFIX_PATH "D:/Qt/6.11.1/msvc2022_64")

# 2. 项目名称
project(SCat-Client LANGUAGES CXX)

# --- VS 默认生成的调试配置 (保留即可) ---
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()
# ------------------------------------

# 3. 【核心】开启 Qt 自动化 (必须有！否则信号槽失效)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# 4. 设定 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 5. 寻找 Qt6 库
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Multimedia Network)
find_package(Qt6 COMPONENTS Widgets Network REQUIRED)
find_package(Qt6 COMPONENTS Widgets Sql REQUIRED)

# 6. 添加可执行文件 (只能写一次！)
add_executable(SCat-Client
    "main.cpp"
     "framelesswindow.h"
    "framelesswindow.cpp"
   "Login/include/LoginWindow.h" 
   "Login/src/LoginWindow.cpp" 
   "Login/include/Login.h" 
   "Login/src/Login.cpp"
    "Register/include/RegisterWindow.h"
    "Register/include/Register.h"  
    "Register/src/RegisterWindow.cpp"
     "Register/src/Register.cpp"   
     "Friend/include/FriendList.h" 
     "Friend/src/FriendList.cpp" 
     "Friend/include/FriendManager.h"
     "Friend/src/FriendManager.cpp"
     "Friend/include/RequestWindow.h"
    "Friend/src/RequestWindow.cpp"
    "Friend/include/RequestItem.h"
    "Friend/src/RequestItem.cpp"
    "Setting/include/SettingsPage.h"
    "Setting/src/SettingsPage.cpp"
    "Setting/include/SettingsManager.h"
    "Setting/src/SettingsManager.cpp"
     "Chat/include/ChatInputEdit.h"
     "Chat/src/ChatInputEdit.cpp"
     "Chat/include/ChatWindow.h" 
     "Chat/src/ChatWindow.cpp" 
     "Chat/include/ChatNetWork.h" 
     "Chat/src/ChatNetWork.cpp"
     "Chat/include/ChatMessage.h"
     "Chat/include/ChatStorage.h"
     "Chat/src/ChatStorage.cpp"
     "Chat/include/FileTransfer.h"
     "Chat/src/FileTransfer.cpp"
      "Scat/include/ScatWindow.h"                                              
      "Scat/src/ScatWindow.cpp"
      "Other/include/ImageUtils.h"
      "Other/src/ImageUtils.cpp"
     "Other/include/AvatarUtils.h"
    "Other/src/AvatarUtils.cpp"
      "Other/include/NotifyButton.h"
      "Other/src/NotifyButton.cpp"
      "Other/include/Notification.h"
      "Other/include/AppPath.h"
       "Other/src/AppPath.cpp"
      "Other/src/Notification.cpp" "Friend/include/AddFriendWindow.h" "Friend/src/AddFriendWindow.cpp"  "NoFrame.cpp"
       "resource.qrc" "NetWork/include/NetWorkManager.h" "NetWork/src/NetWorkManager.cpp" "Friend/include/FriendListitem.h" "Friend/src/FriendListitem.cpp" "Other/src/UserSession.cpp"
        "Other/include/UserSession.h"  "Protocol.h" "NoFrame.h" "NoFrame.cpp" "Other/include/AppController.h" "Other/src/AppController.cpp" )

    set_target_properties(SCat-Client PROPERTIES WIN32_EXECUTABLE $<CONFIG:Release>  OUTPUT_NAME "SCat")


# 7. 链接库
target_link_libraries(SCat-Client PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Network
    Qt6::Sql
    dwmapi 
    user32
)

if (WIN32)
    target_link_libraries(SCat-Client PRIVATE dwmapi user32)
endif()