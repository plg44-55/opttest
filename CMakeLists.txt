cmake_minimum_required(VERSION 3.7)
project (options_converter)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "-fpermissive")
set(CORBA_LIBRARIES -L/home/pete/OB.orig/lib -lOB -lJTC -lpthread -ldl)
include_directories(/home/pete/OB.orig/include . ${CMAKE_BINARY_DIR})
add_custom_command(OUTPUT main_data.cpp main_data.h
  COMMAND LD_LIBRARY_PATH=/home/pete/OB.orig/lib /home/pete/OB.orig/bin/idl --no-skeletons --output-dir ${CMAKE_BINARY_DIR} main_data.idl
  DEPENDS main_data.idl
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  )
add_custom_command(OUTPUT included_data.cpp included_data.h
  COMMAND LD_LIBRARY_PATH=/home/pete/OB.orig/lib /home/pete/OB.orig/bin/idl --no-skeletons --output-dir ${CMAKE_BINARY_DIR} included_data.idl
  DEPENDS included_data.idl
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  )
add_executable(client client.cpp main_data.cpp included_data.cpp)
target_link_libraries(client ${CORBA_LIBRARIES})
