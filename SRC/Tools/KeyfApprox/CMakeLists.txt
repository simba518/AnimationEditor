SET(APP_NAME KeyfApprox)

AUX_SOURCE_DIRECTORY(. KeyfApprox_src)

ADD_EXECUTABLE(${APP_NAME} 
  
  ${KeyfApprox_src}
  )

TARGET_LINK_LIBRARIES(${APP_NAME}
  
  /usr/local/lib/libutility_math.a
  /usr/local/lib/libutility_common.a
  /usr/local/lib/libutility_elastic.a
  /usr/local/lib/libalglib.a
  ${Boost_LIBRARIES}
  ${ARPACK_LIBRARIES}
  ${SUPERLU2.0_LIBRARIES}
  ${LAPACK_LIBRARIES}
  ${SUITESPARSE_LIBRARIES}
  )