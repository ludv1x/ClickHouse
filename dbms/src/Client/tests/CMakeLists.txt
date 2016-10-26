add_executable(test-connect test_connect.cpp)
target_link_libraries (test-connect dbms)

add_executable(bk-primary-key-fix bk_main.cpp)
target_link_libraries(bk-primary-key-fix dbms)
