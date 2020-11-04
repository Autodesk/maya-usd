#------------------------------------------------------------------------------
# compiler flags/definitions
#------------------------------------------------------------------------------
set(GNU_CLANG_FLAGS
    # we want to be as strict as possible
    -Wall
    $<$<BOOL:${BUILD_STRICT_MODE}>:-Werror>
    # optimization
    -msse3
    # disable warnings
    -Wno-deprecated
    -Wno-deprecated-declarations
    -Wno-unused-local-typedefs
)

set(MSVC_FLAGS
    # we want to be as strict as possible
    /W3
    $<$<BOOL:${BUILD_STRICT_MODE}>:/WX>
    # enable pdb generation.
    /Zi
    # standards compliant.
    /Zc:inline
    /Zc:rvalueCast
    # enable multiprocessor builds.
    /MP
    # enable exception handling.
    /EHsc
    # enable initialization order as a level 3 warning
    /w35038
    # disable warnings
    /wd4244
    /wd4267
    /wd4273
    /wd4305
    /wd4506
    /wd4996
    /wd4180
)

set(MSVC_DEFINITIONS
    # Make sure WinDef.h does not define min and max macros which
    # will conflict with std::min() and std::max().
    NOMINMAX

    _CRT_SECURE_NO_WARNINGS
    _SCL_SECURE_NO_WARNINGS

    # Boost
    BOOST_ALL_DYN_LINK
    BOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE

    # Needed to prevent Python from adding a define for snprintf
    # since it was added in Visual Studio 2015.
    HAVE_SNPRINTF
)

#------------------------------------------------------------------------------
# compiler configuration
#------------------------------------------------------------------------------
# Do not use GNU extension 
# Use -std=c++11 instead of -std=gnu++11
set(CMAKE_CXX_EXTENSIONS OFF)

function(mayaUsd_compile_config TARGET)
    # required compiler feature
    # Require C++14 if we're either building for Maya 2019 or later, or if we're building against 
    # USD 20.05 or later. Otherwise require C++11.
    if ((MAYA_APP_VERSION VERSION_GREATER_EQUAL 2019) OR (USD_VERSION_NUM VERSION_GREATER_EQUAL 2005))
        target_compile_features(${TARGET} 
            PRIVATE
                cxx_std_14
        )
    else()
        target_compile_features(${TARGET} 
            PRIVATE
                cxx_std_11
        )
    endif()
    if(IS_GNU OR IS_CLANG)
        target_compile_options(${TARGET} 
            PRIVATE
                ${GNU_CLANG_FLAGS}
        )
    elseif(IS_MSVC)
        target_compile_options(${TARGET} 
            PRIVATE
                ${MSVC_FLAGS}
        )
        target_compile_definitions(${TARGET} 
            PRIVATE
                ${MSVC_DEFINITIONS}
        )
    endif()
endfunction()
