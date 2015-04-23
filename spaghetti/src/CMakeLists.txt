# -*- explicit-buffer-name: "CMakeLists.txt<spaghetti/src>" -*-

#              include( ${QT_USE_FILE} )
   include_directories( ${SPAGHETTI_SOURCE_DIR}/src
                        ${HURRICANE_INCLUDE_DIR}
                      )
                   set( includes      spaghetti/Graph.h
                                      spaghetti/UnionFindMap.h
                      )               
                   set( cpps          Route.cpp
                      )

           add_library( spaghetti       ${cpps} )
 set_target_properties( spaghetti       PROPERTIES VERSION 1.0 SOVERSION 1 )

               install( TARGETS       spaghetti           DESTINATION lib${LIB_SUFFIX} )
               install( FILES         ${includes}
                                      DESTINATION include/coriolis2/spaghetti ) 
   
