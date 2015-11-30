macro (Lint _FILE_LIST)
  # build a list of full paths to files in the list
  set (out_list "")
  foreach( _in_file ${${_FILE_LIST}})
    list (APPEND out_list ${CMAKE_CURRENT_SOURCE_DIR}/${_in_file})
  endforeach(_in_file)
  # make a unique target name for the current directory, by replacing
  # illegal characters in the path
  string(REPLACE "/" "_" lint_target1 ${CMAKE_CURRENT_SOURCE_DIR}lint)
  string(REPLACE ":" "_" lint_target ${lint_target1})
  # create a target to run lint on source files
  add_custom_target(${lint_target} ALL python ${PROJECT_SOURCE_DIR}/retrace/daemon/cpplint.py --output=emacs ${out_list})
endmacro(Lint)