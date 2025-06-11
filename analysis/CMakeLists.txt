set(ANALYSIS_EXECUTABLES
    analyse_numu_cc
    display_numu_cc
    snapshot_numu_cc
)

foreach(EXE_NAME ${ANALYSIS_EXECUTABLES})
    add_executable(${EXE_NAME} ${EXE_NAME}.cpp)
    target_link_libraries(${EXE_NAME}
        PRIVATE
            AnalysisFramework
            ${ROOT_LIBRARIES}
            ${TBB_LIBRARIES}
            ${TORCH_LIBRARIES}
    )
    target_include_directories(${EXE_NAME}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/framework
    )
endforeach()