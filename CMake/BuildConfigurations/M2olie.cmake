set(MITK_CONFIG_PACKAGES
  ACVD
  Qt5
  BLUEBERRY
  MatchPoint
)

set(MITK_CONFIG_PLUGINS
  org.mitk.gui.qt.mitkworkbench.intro
  org.mitk.gui.qt.datamanager
  org.mitk.gui.qt.stdmultiwidgeteditor
  org.mitk.gui.qt.mxnmultiwidgeteditor
  org.mitk.gui.qt.dicombrowser
  org.mitk.gui.qt.imagenavigator
  org.mitk.gui.qt.properties
  org.mitk.gui.qt.segmentation
  org.mitk.planarfigure
  org.mitk.gui.qt.moviemaker
  org.mitk.gui.qt.pointsetinteraction
  org.mitk.gui.qt.viewnavigator
  org.mitk.gui.qt.imagecropper
  org.mitk.gui.qt.pixelvalue
  org.mitk.gui.qt.elastix.registration
  org.mitk.matchpoint.core.helper
  org.mitk.gui.qt.matchpoint.algorithm.browser
  org.mitk.gui.qt.matchpoint.algorithm.control
  org.mitk.gui.qt.matchpoint.framereg
  org.mitk.gui.qt.matchpoint.visualizer
  org.mitk.gui.qt.matchpoint.mapper
  org.mitk.gui.qt.matchpoint.manipulator
  org.mitk.gui.qt.matchpoint.evaluator
)



# set(MITK_CONFIG_PLUGINS ${MITK_CONFIG_PLUGINS}
# )

if(NOT MITK_USE_SUPERBUILD)
  set(BUILD_CoreCmdApps OFF CACHE BOOL "" FORCE)
  set(BUILD_MatchPointCmdApps OFF CACHE BOOL "" FORCE)
endif()

set(MITK_VTK_DEBUG_LEAKS OFF CACHE BOOL "Enable VTK Debug Leaks" FORCE)

find_package(Doxygen REQUIRED)

# Ensure that the in-application help can be build
set(BLUEBERRY_QT_HELP_REQUIRED ON CACHE BOOL "Required Qt help documentation in plug-ins" FORCE)
