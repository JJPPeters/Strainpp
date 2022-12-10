project( strainpp )
cmake_minimum_required( VERSION 3.5 )

cmake_policy(SET CMP0074 NEW)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# set(CMAKE_AUTOMOC ON) # this seemed to not work, did it manually anyway...
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

find_package (Qt5Widgets REQUIRED)
find_package (Qt5PrintSupport REQUIRED)
find_package (Qt5Svg REQUIRED)

if(WIN32)
	if(CMAKE_COMPILER_IS_GNUCXX)
        # I think this is needed to make it a full GUI app, otherwise it will show the console
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwindows")
	endif(CMAKE_COMPILER_IS_GNUCXX)
endif(WIN32)

# Add a custom command that produces version.cpp, plus
# a dummy output that's not actually produced, in order
# to force version.cmake to always be re-run before the build
ADD_CUSTOM_COMMAND(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/version.cpp
		${CMAKE_CURRENT_BINARY_DIR}/_version.cpp
		COMMAND ${CMAKE_COMMAND} -P
		${CMAKE_CURRENT_SOURCE_DIR}/version.cmake)

# this is important as it will be used by the installer scripts
configure_file(version.cpp version.cpp COPYONLY)
#

FIND_PACKAGE( OpenMP REQUIRED )
if(OpenMP_FOUND)
    message(STATUS "Found OpenMP")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif(OpenMP_FOUND)

set(FFTW_ROOT $ENV{FFTW_DIR})
find_package (FFTW REQUIRED)
if(FFTW_FOUND)
    message(STATUS "FFTW3 found (include: ${FFTW_INCLUDES})")
endif(FFTW_FOUND)

find_package (Eigen3 REQUIRED)
if(FFTW_FOUND)
	message(STATUS "EIGEN3 found (include: ${EIGEN3_INCLUDE_DIR})")
endif(FFTW_FOUND)

find_package (TIFF REQUIRED)
if(TIFF_FOUND)
	message(STATUS "TIFF found (include: ${TIFF_INCLUDE_DIR})")
endif(TIFF_FOUND)

find_package (QCustomPlot REQUIRED)
if(QCUSTOMPLOT_FOUND)
	message(STATUS "QCUSTOMPLOT found (include: ${QCustomPlot_INCLUDE_DIR})")
    message(STATUS "QCUSTOMPLOT found (include: ${QCustomPlot_LIBRARIES})")
endif(QCUSTOMPLOT_FOUND)

set (CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS}
	${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}
	${Qt5PrintSupport_EXECUTABLE_COMPILE_FLAGS}
	${Qt5Svg_EXECUTABLE_COMPILE_FLAGS})

add_definitions (
	${Qt5Widgets_DEFINITIONS}
	${Qt5PrintSupport_DEFINITIONS}
	${Qt5Svg_DEFINITIONS}
    ${QCustomPlot_DEFINITIONS}
	)

include_directories (
	${Qt5Widgets_INCLUDE_DIRS}
	${Qt5PrintSupport_INCLUDE_DIRS}
	${Qt5Svg_INCLUDE_DIRS}
	${FFTW_INCLUDES}
	${EIGEN3_INCLUDE_DIR}
	${QCustomPlot_INCLUDE_DIR}
    ${TIFF_INCLUDE_DIR}
	Strain
	Utils
	Plotting
	ReadDM
	)

set ( Strainpp_SRCS
	main.cpp
	mainwindow.cpp
	Strain/phase.cpp
	Strain/gpa.cpp
	Utils/exceptions.cpp
	${CMAKE_CURRENT_BINARY_DIR}/version.cpp
		versiondialog.cpp versiondialog.h)

set ( Strainpp_UIS
	mainwindow.ui
	)
QT5_WRAP_UI(UIS ${Strainpp_UIS})

set ( Strainpp_RSCS
	axesresource.qrc
	)
QT5_ADD_RESOURCES(RSCS ${Strainpp_RSCS})

set ( Strainpp_MOCS
	mainwindow.h
	Plotting/imageplot.h
	Plotting/colorbarplot.h
	)
QT5_WRAP_CPP(MOCS ${Strainpp_MOCS})

add_executable ( strainpp ${Strainpp_SRCS} ${UIS} ${RSCS} ${MOCS} strainpp.rc)
target_link_libraries ( strainpp  Qt5::Widgets Qt5::PrintSupport Qt5::Svg ${FFTW_LIBRARIES} ${TIFF_LIBRARY} ${QCustomPlot_LIBRARY})
