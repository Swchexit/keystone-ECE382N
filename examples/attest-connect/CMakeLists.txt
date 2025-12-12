###############################################
# Shared Eyrie runtime (used by both packages)
###############################################
set(eyrie_plugins "io_syscall linux_syscall env_setup")
set(eyrie_files_to_copy .options_log eyrie-rt loader.bin)
add_eyrie_runtime(attest-connect-eyrie
  ${eyrie_plugins}
  ${eyrie_files_to_copy})

###############################################
# attest_main package
###############################################
set(attest_main_eapp_bin attest-main-eapp)
set(attest_main_eapp_src attest_main/eapp/workload.c)
set(attest_main_host_bin attest-main-runner)
set(attest_main_host_src attest_main/host/host.cpp)
set(attest_main_package_name "attest-main.ke")
set(attest_main_package_script "./attest-main-runner attest-main-eapp eyrie-rt loader.bin")

# attest_main eapp

add_executable(${attest_main_eapp_bin} ${attest_main_eapp_src})
target_link_libraries(${attest_main_eapp_bin} "-nostdlib -static" ${KEYSTONE_LIB_EAPP} ${KEYSTONE_LIB_EDGE})
target_include_directories(${attest_main_eapp_bin}
  PUBLIC ${KEYSTONE_SDK_DIR}/include/app
  PUBLIC ${KEYSTONE_SDK_DIR}/include/edge)

# attest_main host

add_executable(${attest_main_host_bin} ${attest_main_host_src})
target_link_libraries(${attest_main_host_bin}
  ${KEYSTONE_LIB_HOST} ${KEYSTONE_LIB_EDGE} ${KEYSTONE_LIB_VERIFIER})
set_target_properties(${attest_main_host_bin}
  PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO
  )
target_include_directories(${attest_main_host_bin}
  PUBLIC ${KEYSTONE_SDK_DIR}/include/common
  PUBLIC ${KEYSTONE_SDK_DIR}/include/host
  PUBLIC ${KEYSTONE_SDK_DIR}/include/edge
  PUBLIC ${KEYSTONE_SDK_DIR}/include/verifier)

# attest_main packaging

add_keystone_package(${attest_main_eapp_bin}-package
  ${attest_main_package_name}
  ${attest_main_package_script}
  ${eyrie_files_to_copy} ${attest_main_eapp_bin} ${attest_main_host_bin})

add_dependencies(${attest_main_eapp_bin}-package attest-connect-eyrie)
add_dependencies(examples ${attest_main_eapp_bin}-package)

###############################################
# attest_sensor package
###############################################
set(attest_sensor_eapp_bin attest-sensor-eapp)
set(attest_sensor_eapp_src attest_sensor/eapp/peripheral.c)
set(attest_sensor_host_bin attest-sensor-runner)
set(attest_sensor_host_src attest_sensor/host/host.cpp)
set(attest_sensor_package_name "attest-sensor.ke")
set(attest_sensor_package_script "./attest-sensor-runner attest-sensor-eapp eyrie-rt loader.bin")

# attest_sensor eapp

add_executable(${attest_sensor_eapp_bin} ${attest_sensor_eapp_src})
target_link_libraries(${attest_sensor_eapp_bin} "-static")

# attest_sensor host

add_executable(${attest_sensor_host_bin} ${attest_sensor_host_src})
target_link_libraries(${attest_sensor_host_bin} ${KEYSTONE_LIB_HOST} ${KEYSTONE_LIB_EDGE})

# attest_sensor packaging

add_keystone_package(${attest_sensor_eapp_bin}-package
  ${attest_sensor_package_name}
  ${attest_sensor_package_script}
  ${eyrie_files_to_copy} ${attest_sensor_eapp_bin} ${attest_sensor_host_bin})

add_dependencies(${attest_sensor_eapp_bin}-package attest-connect-eyrie)
add_dependencies(examples ${attest_sensor_eapp_bin}-package)
