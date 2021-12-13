#  Apollo64 - Cross-platform Nintendo 64 emulator
#  Copyright (C) 2017-2022 - Azimer - http://www.apollo64.com/
#
#  This program is distributed under the GNU General Public License v3.
#  See the LICENSE document supplied with this source code for details.
	
project("Apollo64" C CXX)

#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_executable(Apollo64 source/main.cc
                        source/uimain.cc)

