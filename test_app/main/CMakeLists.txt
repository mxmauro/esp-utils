
set(srcs "main.cpp"
         "test_run_once.cpp")

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS ".")
