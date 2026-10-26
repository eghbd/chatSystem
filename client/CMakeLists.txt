cmake_minimum_required(VERSION 3.19)
project(ChatClient LANGUAGES CXX)

if(MSVC)
    add_compile_options(/utf-8)
endif()
file(GLOB PB_FILES "./proto/*.proto")
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network WebSockets Protobuf Multimedia)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
qt_standard_project_setup()

# set(CHATCLIENT_PROTO_FILES
#     proto/base.proto
#     proto/file.proto
#     proto/friend.proto
#     proto/gateway.proto
#     proto/message_storage.proto
#     proto/message_transmit.proto
#     proto/notify.proto
#     proto/speech_recognition.proto
#     proto/user.proto
# )

qt_add_executable(ChatClient
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwidget.cpp
    mainwidget.h
    mainwidget.ui
    model/data.h
    resource.qrc
    sessionfriendarea.h sessionfriendarea.cpp
    debug.h
    messageeditarea.h messageeditarea.cpp
    messageshowarea.h messageshowarea.cpp
    selfinfowidget.h selfinfowidget.cpp
    userinfowidget.h userinfowidget.cpp
    sessiondetailwidget.h sessiondetailwidget.cpp
    choosefrienddialog.h choosefrienddialog.cpp
    groupsessiondetailwidget.h groupsessiondetailwidget.cpp
    addfrienddialog.h addfrienddialog.cpp
    historymessagewidget.h historymessagewidget.cpp
    loginwidget.h loginwidget.cpp
    phoneloginwidget.h phoneloginwidget.cpp
    toast.h toast.cpp
    model/datacenter.h model/datacenter.cpp
    network/netclient.h network/netclient.cpp
    verifycodewidget.h verifycodewidget.cpp
    soundrecorder.h soundrecorder.cpp

)

qt_add_protobuf(ChatClient
    PROTO_FILES ${PB_FILES}
)

target_link_libraries(ChatClient
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Network
        Qt::WebSockets
        Qt::Protobuf
        Qt::Multimedia
)

install(TARGETS ChatClient
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET ChatClient
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
