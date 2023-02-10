CMAKE_MINIMUM_REQUIRED(VERSION 3.14)

# Disable specific types of warnings for current directory
# if the compiler supports the flag
FOREACH(warning
    # Downgrade -Werror=suggest-override to warning
    "error=suggest-override"
    )
  MY_CHECK_CXX_COMPILER_WARNING("${warning}" HAS_WARN_FLAG)
  IF(HAS_WARN_FLAG)
    STRING_APPEND(CMAKE_CXX_FLAGS " ${HAS_WARN_FLAG}")
    STRING_APPEND(CMAKE_C_FLAGS " ${HAS_WARN_FLAG}")
  ENDIF()
ENDFOREACH()

# add subdir with blockchain-adapter
add_subdirectory(adapter)

SET(BLOCKCHAIN_PLUGIN_DYNAMIC "ha_blockchain")
SET(BLOCKCHAIN_SOURCES
  engine/src/ha_blockchain.cc
  engine/src/table_service.cc
  engine/src/transaction.cc
)
ADD_DEFINITIONS(-DMYSQL_SERVER)

# C++ 17
set(CMAKE_CXX_STANDARD 17)

# Blockchain storage engine includes
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/engine/include)
INCLUDE_DIRECTORIES(SYSTEM ${BOOST_PATCHES_DIR} ${BOOST_INCLUDE_DIR})

IF(WITH_BLOCKCHAIN_STORAGE_ENGINE AND NOT WITHOUT_BLOCKCHAIN_STORAGE_ENGINE)
  MYSQL_ADD_PLUGIN(blockchain ${BLOCKCHAIN_SOURCES} STORAGE_ENGINE DEFAULT)
ELSEIF(NOT WITHOUT_BLOCKCHAIN_STORAGE_ENGINE)
  MYSQL_ADD_PLUGIN(blockchain ${BLOCKCHAIN_SOURCES}
   STORAGE_ENGINE
   MODULE_ONLY
   LINK_LIBRARIES TrustDBle::adapterFactory
   )
ENDIF()

# Testing
OPTION(WITH_TRUSTDBLE_TESTS
  "Include the TRUSTDBLE test programs" OFF)
IF(WITH_TRUSTDBLE_TESTS)
  add_subdirectory(tests)
ELSE()
ENDIF()
