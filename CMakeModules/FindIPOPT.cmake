# -Try to find ipopt
#
#
# The following are set after configuration is done: 
#  IPOPT_FOUND
#  IPOPT_INCLUDE_DIR
#  IPOPT_LIBRARIES
#  IPOPT_LIBRARY
#  MUMPS_LIBRARY
#  METIS_LIBRARY

SET(IPOPT_POSSIBLE_LIBPATHS

  /usr/lib/
  /usr/local/lib/
  )

FIND_LIBRARY(IPOPT_LIBRARY

  NAMES libipopt.so
  PATHS ${IPOPT_POSSIBLE_LIBPATHS}
  )

IF(NOT IPOPT_LIBRARY)
  MESSAGE(SEND_ERROR "IPOPT libraty is not found.")
ELSE(NOT IPOPT_LIBRARY)
  SET(IPOPT_LIBRARIES ${IPOPT_LIBRARY})
ENDIF(NOT IPOPT_LIBRARY)

FIND_LIBRARY(MUMPS_LIBRARY
  NAMES libcoinmumps.so
  PATHS ${IPOPT_POSSIBLE_LIBPATHS}
  )

IF(NOT MUMPS_LIBRARY)
  MESSAGE(SEND_ERROR "MUMPS_LIBRARY for IPOPT libraty is not found.")
ELSE(NOT MUMPS_LIBRARY)
  SET(IPOPT_LIBRARIES ${IPOPT_LIBRARIES};${MUMPS_LIBRARY})
ENDIF(NOT MUMPS_LIBRARY)

FIND_LIBRARY(METIS_LIBRARY
  NAMES libcoinmetis.so
  PATHS ${IPOPT_POSSIBLE_LIBPATHS}
  )

IF(NOT METIS_LIBRARY)
  MESSAGE(SEND_ERROR "METIS_LIBRARY for IPOPT libraty is not found.")
ELSE(NOT METIS_LIBRARY)
  SET(IPOPT_LIBRARIES ${IPOPT_LIBRARIES};${METIS_LIBRARY})
ENDIF(NOT METIS_LIBRARY)

FIND_PATH(IPOPT_INCLUDE_DIR IpIpoptNLP.hpp

  /usr/include/coin
  /usr/local/include/coin
  )

IF(NOT IPOPT_INCLUDE_DIR)
  MESSAGE(SEND_ERROR "IPOPT include dir is not found.")
ENDIF(NOT IPOPT_INCLUDE_DIR)

MESSAGE(${IPOPT_LIBRARIES})

MARK_AS_ADVANCED(

  IPOPT_LIBRARIES
  IPOPT_INCLUDE_DIR
  IPOPT_FOUND
  )

SET( IPOPT_FOUND FALSE )

IF( IPOPT_LIBRARY )
  IF( IPOPT_INCLUDE_DIR )
	SET( IPOPT_FOUND TRUE )
  ENDIF( IPOPT_INCLUDE_DIR )
ENDIF( IPOPT_LIBRARY )

IF (GLM_FOUND)
ENDIF (GLM_FOUND)

IF( NOT IPOPT_FOUND )
  MESSAGE( SEND_ERROR "IPOPT library is not found." )
ELSE(NOT IPOPT_FOUND)
  MESSAGE(STATUS "Found IPOPT: ${IPOPT_LIBRARIES}")
ENDIF(NOT IPOPT_FOUND )