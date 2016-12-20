project( strainpp )
cmake_minimum_required( VERSION 3.5 )
set ( CMAKE_BUILD_TYPE RELEASE )

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# set(CMAKE_AUTOMOC ON) # this seemed to not work, did it manually anyway...
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

add_definitions ( -Wall )

find_package ( Qt5Widgets REQUIRED )
find_package ( Qt5PrintSupport REQUIRED )
find_package ( Qt5Svg REQUIRED )

find_package ( FFTW REQUIRED )
find_package ( Eigen3 REQUIRED )
find_package ( TIFF REQUIRED )
find_package ( QCustomPlot REQUIRED )

set (CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS}
	${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}
	${Qt5PrintSupport_EXECUTABLE_COMPILE_FLAGS}
	${Qt5Svg_EXECUTABLE_COMPILE_FLAGS})

add_definitions (
	${Qt5Widgets_DEFINITIONS}
	${Qt5PrintSupport_DEFINITIONS}
	${Qt5Svg_DEFINITIONS}
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
	)

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
target_link_libraries ( strainpp  Qt5::Widgets Qt5::PrintSupport Qt5::Svg ${QCustomPlot_LIBRARY} ${FFTW_LIBRARIES} ${TIFF_LIBRARY})
