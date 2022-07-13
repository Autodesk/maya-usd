
####################################################################################################
# Setup 
####################################################################################################

set(USDTRANSACTION_LIBRARY_NAME "AL_USDTransaction")
set(USDTRANSACTION_LIBRARY_LOCATION ${AL_INSTALL_PREFIX}/lib) 

# Python bindings
if(IS_WINDOWS AND MAYAUSD_DEFINE_BOOST_DEBUG_PYTHON_FLAG)
  # On Windows when compiling with debug python the library must be named with _d.
  set(USDTRANSACTION_PYTHON_LIBRARY_NAME _${USDTRANSACTION_LIBRARY_NAME}_d)
  else()
  set(USDTRANSACTION_PYTHON_LIBRARY_NAME _${USDTRANSACTION_LIBRARY_NAME})
endif()

set(USDTRANSACTION_PYTHON_LIBRARY_LOCATION ${AL_INSTALL_PREFIX}/lib/python/AL/usd/transaction) 

####################################################################################################
# General 
####################################################################################################

list(APPEND usdtransaction_headers
  Api.h
  Notice.h
  Transaction.h
  TransactionManager.h
)

list(APPEND usdtransaction_source
  Notice.cpp
  Transaction.cpp
  TransactionManager.cpp
)

add_library(${USDTRANSACTION_LIBRARY_NAME}
  SHARED
    ${usdtransaction_source}
)

# compiler configuration
mayaUsd_compile_config(${USDTRANSACTION_LIBRARY_NAME})

target_compile_definitions(${USDTRANSACTION_LIBRARY_NAME}
    PRIVATE
        AL_USD_TRANSACTION_EXPORT
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:TBB_USE_DEBUG>
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_DEBUG_PYTHON>
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_LINKING_PYTHON>
)

target_include_directories(${USDTRANSACTION_LIBRARY_NAME} 
  PUBLIC
    ${USDTRANSACTION_INCLUDE_LOCATION} 
    ${USD_INCLUDE_DIR}
)

target_link_libraries(${USDTRANSACTION_LIBRARY_NAME}
  tf
  sdf
  usd
  ${PYTHON_LIBRARIES}
)

list(APPEND usdtransaction_python_source
  module.cpp
  wrapNotice.cpp
  wrapTransaction.cpp
  wrapTransactionManager.cpp
)

add_library(${USDTRANSACTION_PYTHON_LIBRARY_NAME}
  SHARED
    ${usdtransaction_python_source}
)

# compiler configuration
mayaUsd_compile_config(${USDTRANSACTION_PYTHON_LIBRARY_NAME})

target_compile_definitions(${USDTRANSACTION_PYTHON_LIBRARY_NAME}
  PRIVATE
    $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:TBB_USE_DEBUG>
    $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_DEBUG_PYTHON>
    $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_LINKING_PYTHON>
    MFB_PACKAGE_NAME=${USDTRANSACTION_LIBRARY_NAME}
    MFB_ALT_PACKAGE_NAME=${USDTRANSACTION_LIBRARY_NAME}
    MFB_PACKAGE_MODULE=${USDTRANSACTION_LIBRARY_NAME}
)

target_include_directories(${USDTRANSACTION_PYTHON_LIBRARY_NAME} 
  PUBLIC
    ${USDTRANSACTION_INCLUDE_LOCATION} 
    ${USD_INCLUDE_DIR}
)

target_link_libraries(${USDTRANSACTION_PYTHON_LIBRARY_NAME}
  ${USDTRANSACTION_LIBRARY_NAME}
  ${PYTHON_LIBRARIES}
)

set_target_properties(${USDTRANSACTION_PYTHON_LIBRARY_NAME}
  PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# handle run-time search paths
if(IS_MACOSX OR IS_LINUX)
    mayaUsd_init_rpath(rpath ${USDTRANSACTION_PYTHON_LIBRARY_LOCATION})
    mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/plugin/al/lib")
    mayaUsd_install_rpath(rpath ${USDTRANSACTION_PYTHON_LIBRARY_NAME})
endif()

set_python_module_property(${USDTRANSACTION_PYTHON_LIBRARY_NAME})

if(NOT SKIP_USDMAYA_TESTS)
  add_subdirectory(tests)
endif()

####################################################################################################
# Install 
####################################################################################################

# install the public headers
install(FILES ${usdtransaction_headers}
  DESTINATION ${AL_INSTALL_PREFIX}/include/AL/usd/transaction
)

# install the library
install(TARGETS ${USDTRANSACTION_LIBRARY_NAME}
  LIBRARY
  DESTINATION ${USDTRANSACTION_LIBRARY_LOCATION}
  RUNTIME
  DESTINATION ${USDTRANSACTION_LIBRARY_LOCATION}
)
if(MSVC)
  install(FILES $<TARGET_PDB_FILE:${USDTRANSACTION_LIBRARY_NAME}> DESTINATION ${USDTRANSACTION_LIBRARY_LOCATION} OPTIONAL)
endif()

# install python library
install(TARGETS ${USDTRANSACTION_PYTHON_LIBRARY_NAME}
  LIBRARY
  DESTINATION ${USDTRANSACTION_PYTHON_LIBRARY_LOCATION}
  RUNTIME
  DESTINATION ${USDTRANSACTION_PYTHON_LIBRARY_LOCATION}
)
