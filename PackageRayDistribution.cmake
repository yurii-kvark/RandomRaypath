function(ray_enable_packaging)


    message(STATUS "START ray_enable_packaging")

    if(NOT TARGET RayClientRenderer)
        message(FATAL_ERROR "ray_enable_packaging requires the RayClientRenderer target")
    endif()

    include(GNUInstallDirs)

    set(RAY_PACKAGE_BUILD_TYPE "${CMAKE_BUILD_TYPE}")
    if(NOT RAY_PACKAGE_BUILD_TYPE)
        set(RAY_PACKAGE_BUILD_TYPE "Release")
    endif()
    string(TOLOWER "${RAY_PACKAGE_BUILD_TYPE}" RAY_PACKAGE_BUILD_TYPE_LOWER)

    install(DIRECTORY "${CMAKE_SOURCE_DIR}/config"
            DESTINATION "."
    )
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/resource"
            DESTINATION "."
    )

    if(RAY_GRAPHICS_ENABLE)
        install(DIRECTORY "${CMAKE_SOURCE_DIR}/shaders/bins"
                DESTINATION "shaders"
        )
    endif()

    install(FILES
            "${CMAKE_SOURCE_DIR}/LICENSE.txt"
            "${CMAKE_SOURCE_DIR}/README.md"
            DESTINATION "."
    )

# WIN, DEB
    if(WIN32)
        get_filename_component(RAY_COMPILER_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)

        install(TARGETS RayClientRenderer
                RUNTIME_DEPENDENCY_SET RayClientRenderer_RuntimeDependencies
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )

        install(RUNTIME_DEPENDENCY_SET RayClientRenderer_RuntimeDependencies
                DIRECTORIES "${RAY_COMPILER_BIN_DIR}"
                PRE_EXCLUDE_REGEXES
                    "api-ms-.*"
                    "ext-ms-.*"
                    "[Aa]zure.*"
                    "[Hh]vsi.*"
                    "[Pp]dm.*"
                    "[Ww]pax.*"
                POST_EXCLUDE_REGEXES
                    ".*[\\\\/]+Windows[\\\\/]+System32[\\\\/].*"
                    ".*[\\\\/]+Windows[\\\\/]+SysWOW64[\\\\/].*"
                    ".*[\\\\/]+Windows[\\\\/]+WinSxS[\\\\/].*"
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )

        if(MSVC)
            include(InstallRequiredSystemLibraries)
        endif()
    else()
        install(TARGETS RayClientRenderer
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
    endif()

    set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
    set(CPACK_PACKAGE_VENDOR "${PROJECT_NAME}")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "RandomRaypath distributable binaries")
    set(CPACK_PACKAGE_VERSION "0.0")
    set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)

    set(CPACK_GENERATOR "ZIP")

    include(CPack)
endfunction()
