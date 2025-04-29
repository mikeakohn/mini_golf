idf_component_register(
  SRCS
    GolfGameTee.cpp
    PN532.cpp
    NetworkClient.cpp
    ../../common/Network.cpp
    main.cpp
  INCLUDE_DIRS "")

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-format)
target_include_directories(${COMPONENT_LIB} PUBLIC ../../common)

