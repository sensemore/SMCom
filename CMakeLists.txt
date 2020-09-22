set(srcs "src/SM_Communication.cpp" )

idf_component_register(SRCS "${srcs}")

#add_subdirectory(include)
target_include_directories(${COMPONENT_LIB} PUBLIC include)
