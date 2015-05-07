# -*- explicit-buffer-name: "CMakeLists.txt<spaghetti/src>" -*-

   include_directories( ${SPAGHETTI_SOURCE_DIR}/src
                        ${COLOQUINTE_INCLUDE_DIR} 
                        ${HURRICANE_INCLUDE_DIR}
                      )
                   set( includes      spaghetti/Graph.h
                                      spaghetti/Grid.h
                                      spaghetti/UnionFindMap.h
                                      spaghetti/CostFunctions.h
                      )               
                   set( cpps          Graph.cpp
                                      RoutableGraph.cpp
                                      Grid.cpp
                                      CostFunctions.cpp
                                      SteinerRoute.cpp
                      )
                   set( deplibs       ${HURRICANE_LIBRARIES}
                                      ${Boost_LIBRARIES}
                                      ${COLOQUINTE_LIBRARIES}
                      )

           add_library( spaghetti       ${cpps} )
 set_target_properties( spaghetti       PROPERTIES VERSION 1.0 SOVERSION 1 )
 target_link_libraries( spaghetti       ${deplibs})

        add_executable( spaghetti.bin   main.cpp)
 target_link_libraries( spaghetti.bin   spaghetti ${deplibs})

               install( TARGETS       spaghetti           DESTINATION lib${LIB_SUFFIX} )
               install( TARGETS       spaghetti.bin       DESTINATION bin )
               install( FILES         ${includes}
                                      DESTINATION include/coriolis2/spaghetti ) 
   
