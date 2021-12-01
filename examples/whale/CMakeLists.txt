project(whale)
add_executable(${PROJECT_NAME} main.cpp model.cpp openglwindow.cpp
                               trackball.cpp)
enable_abcg(${PROJECT_NAME})
