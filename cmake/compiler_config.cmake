#------------------------------------------------------------------------------
# compiler flags/definitions
#------------------------------------------------------------------------------
set(gnu_clang_flags
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

set(msvc_flags
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
    # disable warnings
    /wd4244
    /wd4267
    /wd4273
    /wd4305
    /wd4506
    /wd4996
    /wd4180
)

set(msvc_definitions
    # Make sure WinDef.h does not define min and max macros which
    # will conflict with std::min() and std::max().
    NOMINMAX

    _CRT_SECURE_NO_WARNINGS
    _SCL_SECURE_NO_WARNINGS

    # Boost
    BOOST_ALL_DYN_LINK
    BOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE
)

#------------------------------------------------------------------------------
# compiler configuration
#------------------------------------------------------------------------------
# Don not use GNU extension 
# Use -std=c++11 instead of -std=gnu++11
set(CMAKE_CXX_EXTENSIONS OFF)

function(mayaUsd_compile_config TARGET)
    # required compiler feature
    target_compile_features(${TARGET} 
        PRIVATE
            $<$<VERSION_LESS:MAYA_APP_VERSION,2019>:cxx_std_11>
            $<$<VERSION_GREATER_EQUAL:MAYA_APP_VERSION,2019>:cxx_std_14>
    )
    if(IS_GNU OR IS_CLANG)
        target_compile_options(${TARGET} 
            PRIVATE
                ${gnu_clang_flags}
        )
    elseif(IS_MSVC)
        target_compile_options(${TARGET} 
            PRIVATE
                ${msvc_flags}
        )
        target_compile_definitions(${TARGET} 
            PRIVATE
                ${msvc_definitions}
        )
    endif()
endfunction()
