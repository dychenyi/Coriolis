# - Find the Spaghetti includes and libraries.
# The following variables are set if Coriolis is found.  If SPAGHETTI is not
# found, SPAGHETTI_FOUND is set to false.
#  SPAGHETTI_FOUND       - True when the Coriolis include directory is found.
#  SPAGHETTI_INCLUDE_DIR - the path to where the Coriolis include files are.
#  SPAGHETTI_LIBRARIES   - The path to where the Coriolis library files are.


SET(SPAGHETTI_INCLUDE_PATH_DESCRIPTION "directory containing the Spaghetti include files. E.g /usr/local/include/coriolis or /asim/coriolis/include/coriolis")

SET(SPAGHETTI_DIR_MESSAGE "Set the SPAGHETTI_INCLUDE_DIR cmake cache entry to the ${SPAGHETTI_INCLUDE_PATH_DESCRIPTION}")

# don't even bother under WIN32
IF(UNIX)
  #
  # Look for an installation.
  #
  FIND_PATH(SPAGHETTI_INCLUDE_PATH NAMES spaghetti/SpaghettiEngine.h PATHS
    # Look in other places.
    ${CORIOLIS_DIR_SEARCH}
    PATH_SUFFIXES include/coriolis
    # Help the user find it if we cannot.
    DOC "The ${SPAGHETTI_INCLUDE_PATH_DESCRIPTION}"
  )

  FIND_LIBRARY(SPAGHETTI_LIBRARY_PATH
    NAMES spaghetti
    PATHS ${CORIOLIS_DIR_SEARCH}
    PATH_SUFFIXES lib${LIB_SUFFIX}
    # Help the user find it if we cannot.
    DOC "The ${SPAGHETTI_INCLUDE_PATH_DESCRIPTION}"
  )

  SET_LIBRARIES_PATH(SPAGHETTI SPAGHETTI)
  HURRICANE_CHECK_LIBRARIES(SPAGHETTI)

ENDIF(UNIX)
