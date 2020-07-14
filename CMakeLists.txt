cmake_minimum_required(VERSION 3.7)
project (options_converter)

macro(build_idls)
  foreach(_idl ${ARGN})
    add_custom_command(OUTPUT ${_idl}.cpp ${_idl}.h
      COMMAND LD_LIBRARY_PATH=${CORBA_PREFIX}/lib ${CORBA_PREFIX}/bin/idl --no-skeletons --output-dir ${CMAKE_BINARY_DIR} ${_idl}.idl
      DEPENDS ${_idl}.idl
      WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
      )
  endforeach(_idl)
endmacro()
    
#set(IDL main_data)
#set(IDL1 included_data)

set(IDLS main_data included_data)
#set(IDLS main_data)
set(CMAKE_CXX_STANDARD 11)
set(MY_FLAGS -fpermissive -Wno-deprecated -g)
set(CORBA_PREFIX /home/pete/tests/OB1)
set(CORBA_LIBRARIES -Wl,-rpath=${CORBA_PREFIX}/lib -L${CORBA_PREFIX}/lib -lOB -lJTC -lpthread -ldl)

#add_custom_command(OUTPUT ${IDL}.cpp ${IDL}.h
#  COMMAND LD_LIBRARY_PATH=${CORBA_PREFIX}/lib ${CORBA_PREFIX}/bin/idl --no-skeletons --output-dir ${CMAKE_BINARY_DIR} ${IDL}.idl
#  DEPENDS ${IDL}.idl
#  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
#  )
#add_custom_command(OUTPUT ${IDL1}.cpp ${IDL1}.h
#  COMMAND LD_LIBRARY_PATH=${CORBA_PREFIX}/lib ${CORBA_PREFIX}/bin/idl --no-skeletons --output-dir ${CMAKE_BINARY_DIR} ${IDL1}.idl
#  DEPENDS ${IDL1}.idl
#  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
#  )

#add_executable(client client.cpp main_data.cpp included_data.cpp)

set(src client.cpp)
#message(${IDLS})
foreach(_i ${IDLS})
  list(APPEND src ${_i}.cpp)
endforeach()
#message(${src})

build_idls(${IDLS})

add_executable(client ${src})
target_include_directories(client PUBLIC ${CORBA_PREFIX}/include . ${CMAKE_BINARY_DIR})
target_compile_options(client PUBLIC ${MY_FLAGS})
target_link_libraries(client ${CORBA_LIBRARIES})
