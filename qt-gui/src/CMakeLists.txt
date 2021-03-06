set(SOURCE_DIRS
  config
  contactlist
  core
  dialogs
  dockicons
  helpers
  settings
  userdlg
  userevents
  views
  widgets
)

set(sources)
set(mocs)

foreach (dir ${SOURCE_DIRS})
  add_subdirectory(${dir})

  # Get list of sources from subdir
  get_directory_property(${dir}_SRCS DIRECTORY ${dir} DEFINITION ${dir}_SRCS)
  foreach (file ${${dir}_SRCS})
    list(APPEND sources "${dir}/${file}")
  endforeach (file)

  # Get list of header files that should be moc:ed
  get_directory_property(${dir}_MOC_HDRS DIRECTORY ${dir} DEFINITION ${dir}_MOC_HDRS)

  # Add targets for creating moc files
  foreach (file ${${dir}_MOC_HDRS})
    set(outfile "${dir}/moc_${file}.cpp")
    if (WITH_QT5)
      qt5_generate_moc("${CMAKE_CURRENT_SOURCE_DIR}/${dir}/${file}" ${outfile})
    else (WITH_QT5)
      qt4_generate_moc("${CMAKE_CURRENT_SOURCE_DIR}/${dir}/${file}" ${outfile})
    endif (WITH_QT5)
    list(APPEND mocs ${outfile})
  endforeach (file)
endforeach (dir)

if (WITH_KDE)
  set(target licq_kde-gui)
else (WITH_KDE)
  set(target licq_qt-gui)
endif (WITH_KDE)

licq_add_plugin(${target} ${sources} ${mocs})
target_link_libraries(${target} ${LIBRARIES})
if (WITH_QT5)
  target_link_libraries(${target} Qt5::Widgets)
  target_link_libraries(${target} Qt5::Xml)
endif (WITH_QT5)
