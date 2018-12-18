set(THIRD_PARTY_PACK_DIR "${SIMDIS_SDK_SOURCE_DIR}/3rd/${BUILD_SYSTEM_NAME}${BUILD_TYPE}_${BUILD_COMPILER}")
if(IS_DIRECTORY "${THIRD_PARTY_PACK_DIR}")
    message(STATUS "Detected 3rd party pack at:")
    message(STATUS "   ${THIRD_PARTY_PACK_DIR}")
    message(STATUS "   Auto-configuring defaults.")
    set(OSG_DIR ${THIRD_PARTY_PACK_DIR}/OpenSceneGraph-3.6.3 CACHE PATH "3rd party library directory")
    set(OSGEARTH_DIR ${THIRD_PARTY_PACK_DIR}/osgEarth-SDK-1.10 CACHE PATH "3rd party library directory")
    set(PROTOBUF_DIR ${THIRD_PARTY_PACK_DIR}/protobuf-2.6.0 CACHE PATH "3rd party library directory")
    set(SQLITE3_DIR ${THIRD_PARTY_PACK_DIR}/sqlite-3.8.11.1 CACHE PATH "3rd party library directory")
    set(GDAL_DIR ${THIRD_PARTY_PACK_DIR}/gdal-2.1.1 CACHE PATH "3rd party library directory")
endif()
