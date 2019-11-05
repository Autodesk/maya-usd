
####################################################################################################
# Setup 
####################################################################################################

set(USDTRANSACTION_LIBRARY_NAME "AL_USDTransaction")
set(USDTRANSACTION_LIBRARY_LOCATION ${CMAKE_INSTALL_PREFIX}/lib) 

set(USDTRANSACTION_PYTHON_LIBRARY_NAME _${USDTRANSACTION_LIBRARY_NAME})
set(USDTRANSACTION_PYTHON_LIBRARY_LOCATION ${CMAKE_INSTALL_PREFIX}/lib/python/AL/usd/transaction) 

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

target_compile_definitions(${USDTRANSACTION_LIBRARY_NAME}
    PRIVATE
        AL_USD_TRANSACTION_EXPORT
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
  ${python_LIBRARIES}
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

target_compile_definitions(${USDTRANSACTION_PYTHON_LIBRARY_NAME}
  PRIVATE
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
  ${python_LIBRARIES}
)

set_target_properties(${USDTRANSACTION_PYTHON_LIBRARY_NAME}
  PROPERTIES 
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

if(NOT SKIP_USDMAYA_TESTS)
  add_subdirectory(tests)
endif()

####################################################################################################
# Install 
####################################################################################################

# install the public headers
install(FILES ${usdtransaction_headers}
  DESTINATION ${CMAKE_INSTALL_PREFIX}/include/AL/usd/transaction
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
