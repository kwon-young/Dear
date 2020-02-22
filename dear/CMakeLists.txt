include(FindPythonInterp)
if (PYTHONINTERP_FOUND)
    set(DEAR_PYTHON_EXECUTABLE "python${PYTHON_VERSION_MAJOR}")
else()
    message(FATAL_ERROR "Have not found python interpreter")
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/dear.py ${CMAKE_CURRENT_BINARY_DIR}/dear)

include(GNUInstallDirs)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/dear
    DESTINATION ${CMAKE_INSTALL_BINDIR})
