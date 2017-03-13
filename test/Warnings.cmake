cmake_minimum_required(VERSION 2.8)

if(COMMAND CMAKE_POLICY)
  if(POLICY CMP0009)
    cmake_policy(SET CMP0009 NEW)
  endif(POLICY CMP0009)
endif(COMMAND CMAKE_POLICY)
set(rs "$ENV{ROOTSYS}")
set(ar "$ENV{ALICE_ROOT}")
#warnings exceptions
set(exceptions "always evaluates both arguments" "std::" "TGeoMatrix::operator\\*" "struct AliRawDataHeader" "/usr/local/dim" " from " "JETAN/fastjet/fastjet/" "G__" "base class .struct AliHLTCaloChannelDataStruct. has a non-virtual destructor")

#Function to count no of lines in a file equivalent to wc -l
function (wcl _nLinesVar fileName)
  file(READ ${fileName} fileData)
  if(fileData)
    if(WIN32)
      string(REGEX MATCHALL "\r\n" nLines ${fileData})
    else()
      string(REGEX MATCHALL "[\r\n]" nLines ${fileData})
    endif(WIN32)
    list(LENGTH nLines nLines)
    set(${_nLinesVar} ${nLines} PARENT_SCOPE)
  else()
    set(${_nLinesVar} -1 PARENT_SCOPE)
  endif()
endfunction()

find_program(svn_command svn)

if(ar AND rs)
  #Read build log from (default - make.log)
  set(buildLog "$ENV{ALICE_INSTALL}/make.log")
  file(READ ${buildLog} lineList)
  string(REGEX REPLACE "\n" ";" "lineList" "${lineList}")
  foreach (line ${lineList})
    set(warning "")
    string(REGEX MATCH ".+warning:.+" "warning" "${line}")
    if(warning)
      #    message("${warning}")
      list(APPEND warnList "${line}")
      string(REGEX MATCH "(.+):[0-9]+" "file" "${line}")
      string(REGEX REPLACE ":[0-9]+" "" "file" "${file}")
      list(APPEND fileList "${file}")
    endif(warning)
  endforeach(line)
  list(REMOVE_DUPLICATES fileList)
  set(rootOut "+--| ROOT\n")
  set(aliOut "+--| ALIROOT\n")
  list(SORT fileList)
  foreach(file ${fileList})
    set(root "")
    set(aliroot "")
    string(REGEX MATCH "${rs}" "root" "${file}")  
    string(REGEX MATCH "${ar}" "aliroot" "${aliroot}")
    if(root)
      string(REGEX REPLACE "${root}[/]*" "" "file" "${file}")
      set(rootOut "${rootOut}   |\n")
      set(rootOut "${rootOut}   +--| ${file}\n")
    else()
      set(aliroot ${ar})
      string(REGEX REPLACE "${aliroot}[/]*" "" "file" "${file}")
      set(aliOut "${aliOut}   |\n")
      set(aliOut "${aliOut}   +--| ${file}\n")
    endif(root)
    list(REMOVE_DUPLICATES warnList)
    foreach(line ${warnList})
      set(match "")
      string(REGEX MATCH "${file}[:]*" "match" "${line}")
      if(match)
	string(REGEX REPLACE "${match}" "" "line" "${line}")
	if(aliroot)
	  string(REGEX REPLACE "${aliroot}" "" "line" "${line}")
	  set(aliOut "${aliOut}      +--| ${line}\n")
	elseif(root)
	    string(REGEX REPLACE "${root}" "" "line" "${line}")
	    set(rootOut "${rootOut}      +--| ${line}\n")
	  endif(aliroot)
	endif(match)
      endforeach(line)
    endforeach(file)

set(tot 0)
set (anlys "${anlys}\n\n==============================Detailed Analysis==============================\n")
#Blaming people
set (anlys "${anlys}+--| AliRoot\n")
foreach( blameFile ${fileList} )
  string(REPLACE "$ENV{ALICE_ROOT}/" "" mainFile ${blameFile})
  set(anlys "${anlys}   |\n")
  execute_process(COMMAND "${svn_command}" "blame" "${blameFile}"
    WORKING_DIRECTORY "$ENV{ALICE_ROOT}"
    OUTPUT_FILE tmpViols
    OUTPUT_VARIABLE tmpVio)
  wcl(fileLen "$ENV{ALICE_ROOT}/tmpViols")
  set(anlys "${anlys}   |--| ${mainFile} |-- ${fileLen} lines\n")  
  set(nameList "")
  file(STRINGS "$ENV{ALICE_ROOT}/tmpViols" violData)
  # file(READ "tmpViols" violData)
  # string(REGEX REPLACE "\n" ";" "violData" "${violData}")
  foreach(violLine ${violData})
    string(REGEX MATCH "[0-9]+[ ]+[a-zA-Z0-9]+" "name" "${violLine}")
    string(REGEX REPLACE "^[0-9]+[ ]+" "" "name" "${name}")
    list(APPEND nameList ${name})
    list(APPEND fullViolList ${name})
  endforeach(violLine)
  list(LENGTH violData tot)
  set (userList ${nameList})
  set(finale "")  
  if(userList)
    list(REMOVE_DUPLICATES userList)
  endif(userList)
  foreach(name ${userList})
    string(REGEX MATCHALL "${name}" "nameCount" "${nameList}")
    list(LENGTH nameCount nameCount)
    math(EXPR num "(${nameCount}*100)/${tot}")
    math(EXPR dec "((10000*${nameCount}/${tot}-100*${num}+5))")
    if( ${dec} GREATER 99 )
      set(dec 0)
      math(EXPR num "${num}+1")
    endif(${dec} GREATER 99)
    math(EXPR dec "${dec}/10")
    math(EXPR pcnt "99999 - ${num}${dec}")
    string(LENGTH ${num} digit)
    if(digit EQUAL 1)
      set(space "  ")
    elseif(digit EQUAL 2)
      set(space " ")
    endif(digit EQUAL 1) 
    list(APPEND finale "${pcnt}-${space}${num}.${dec}% | ${name} |-- ${nameCount} lines")
  endforeach(name)
  list(SORT finale)
  foreach(record ${finale})
    string(LENGTH ${record} recLen)
    math(EXPR recLen "${recLen}-6")
    string(SUBSTRING ${record} 6 ${recLen}  record)
    set(anlys "${anlys}      +--|${record}\n")
  endforeach(record)
endforeach( blameFile)



message("${anlys}")
    message("==============================ROOT FILE  WARNINGS==============================\n")
    message("${rootOut}")
    message("\n\n==============================ALIROOT FILE WARNINGS==============================\n")
    message("${aliOut}")
  else()
    message(WARNING "Environment Variable ALICE_ROOT and ROOTSYS must be set")
  endif(ar AND rs)