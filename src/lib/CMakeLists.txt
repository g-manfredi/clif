cmake_minimum_required (VERSION 2.6)
project(libclif)

function(createEnumStrings ENUMSTR)
  set(${ENUMSTR}Enums ${${ENUMSTR}} PARENT_SCOPE)
  set(${ENUMSTR}Strs "\"${${ENUMSTR}}\"")
  string(REPLACE "," "\",\"" ${ENUMSTR}Strs ${${ENUMSTR}Strs})
  set(${ENUMSTR}Strs "${${ENUMSTR}Strs},NULL" PARENT_SCOPE)
endfunction(createEnumStrings)

set(DataType     "INVALID,UINT8,UINT16")
set(DataOrg      "INVALID,PLANAR,INTERLEAVED,BAYER_2x2")
set(DataOrder    "INVALID,RGGB,BGGR,GBRG,GRBG,RGB")
set(ExtrType     "INVALID,LINE")
set(CalibPattern "INVALID,CHECKERBOARD,HDMARKER")
set(DistModel    "INVALID,CV8,LINES")

createEnumStrings(DataType)
createEnumStrings(DataOrg)
createEnumStrings(DataOrder)
createEnumStrings(ExtrType)
createEnumStrings(CalibPattern)
createEnumStrings(DistModel)

configure_file(enumtypes.hpp.in enumtypes.hpp)
set(CLIF_HEADERS ${CLIF_HEADERS} ${CMAKE_CURRENT_BINARY_DIR}/enumtypes.hpp)

add_library(clif clif.cpp subset3d.cpp calib.cpp hdf5.cpp attribute.cpp helpers.cpp datastore.cpp dataset.cpp core.cpp clif_cv.cpp clif_vigra.cpp enumtypes.cpp)
add_library(clif_vigra clif_vigra.cpp)

target_link_libraries(clif cliini ${HDF5_LIBRARIES} ${Boost_LIBRARIES} ${OpenCV_LIBRARIES} ${CLIF_LINK_LIBRARIES})
target_link_libraries(clif_vigra clif ${VIGRA_LIBRARIES})
#target_link_libraries(clifcalib clif)

set(CLIF_HEADERS ${CLIF_HEADERS} attribute.hpp calib.hpp clif_cv.hpp clif.hpp clif_vigra.hpp core.hpp dataset.hpp datastore.hpp flexmav.hpp hdf5.hpp helpers.hpp matio.hpp stringtree.hpp subset3d.hpp)

install(TARGETS clif clif_vigra
		EXPORT ${PROJECT_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install(FILES ${CLIF_HEADERS} DESTINATION include/clif)
# Install the export set for use with the install-tree
install(EXPORT ${PROJECT_NAME} DESTINATION "lib/cmake/clif" COMPONENT dev)
