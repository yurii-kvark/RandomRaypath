# cmake/RayShaders.cmake
# Compiles all non-.spv files inside ${CMAKE_SOURCE_DIR}/shaders into *.spv next to them.
# Incremental: rebuilds only when input changed or output missing.

function(ray_enable_shader_compilation)
    message(STATUS
            "Ray shader compilation tool start."
    )

    if(NOT GLSLANG_VALIDATOR)
        find_program(GLSLANG_VALIDATOR NAMES glslangValidator glslangValidator.exe)
    endif()

    if(NOT GLSLANG_VALIDATOR)
        message(WARNING
                "GLSLANG_VALIDATOR not found. Install Vulkan SDK or glslang, "
                "or set GLSLANG_VALIDATOR to the full path."
        )
        return()
    endif()

    set(SHADER_DIR "${CMAKE_SOURCE_DIR}/shaders")

    if(NOT IS_DIRECTORY "${SHADER_DIR}")
        message(STATUS "Shader directory not found: ${SHADER_DIR} (skipping)")
        return()
    endif()

    file(GLOB_RECURSE RAY_SHADER_SOURCES CONFIGURE_DEPENDS
            "${SHADER_DIR}/*.vert"
            "${SHADER_DIR}/*.frag"
            "${SHADER_DIR}/*.comp"
            "${SHADER_DIR}/*.geom"
            "${SHADER_DIR}/*.tesc"
            "${SHADER_DIR}/*.tese"
            "${SHADER_DIR}/*.mesh"
            "${SHADER_DIR}/*.task"
            "${SHADER_DIR}/*.rgen"
            "${SHADER_DIR}/*.rmiss"
            "${SHADER_DIR}/*.rchit"
            "${SHADER_DIR}/*.rahit"
            "${SHADER_DIR}/*.rint"
            "${SHADER_DIR}/*.rcall"
    )

    if(NOT RAY_SHADER_SOURCES)
        message(STATUS "No shader sources found in: ${SHADER_DIR}")
        return()
    endif()

    set(RAY_SPIRV_OUTPUTS "")
    foreach(SHADER IN LISTS RAY_SHADER_SOURCES)
        set(SPIRV "${SHADER}.spv")

        add_custom_command(
                OUTPUT "${SPIRV}"
                COMMAND "${GLSLANG_VALIDATOR}" -V "${SHADER}" -o "${SPIRV}"
                DEPENDS "${SHADER}"
                COMMENT "glslangValidator: ${SHADER} -> ${SPIRV}"
                VERBATIM
        )

        list(APPEND RAY_SPIRV_OUTPUTS "${SPIRV}")
    endforeach()

    # One target that builds all shaders (and is part of default build)
    add_custom_target(ray_shaders ALL DEPENDS ${RAY_SPIRV_OUTPUTS})

    # Make the name accessible if you want to add dependencies elsewhere
    set(RAY_SHADERS_TARGET ray_shaders PARENT_SCOPE)
endfunction()