###############################################
# Shared Eyrie runtime (used by both packages)
###############################################
set(eyrie_plugins "io_syscall linux_syscall env_setup")
set(eyrie_files_to_copy .options_log eyrie-rt loader.bin)
add_eyrie_runtime(workload-peripheral-eyrie
  ${eyrie_plugins}
  ${eyrie_files_to_copy})

###############################################
# Workload package
###############################################
set(workload_eapp_bin workload-eapp)
set(workload_eapp_src workload/eapp/workload.c)
set(workload_host_bin workload-runner)
set(workload_host_src workload/host/host.cpp)
set(workload_package_name "workload.ke")
set(workload_package_script "./workload-runner workload-eapp eyrie-rt loader.bin")

# workload eapp

add_executable(${workload_eapp_bin} ${workload_eapp_src})
target_link_libraries(${workload_eapp_bin} "-nostdlib -static" ${KEYSTONE_LIB_EAPP} ${KEYSTONE_LIB_EDGE})
target_include_directories(${workload_eapp_bin}
  PUBLIC ${KEYSTONE_SDK_DIR}/include/app
  PUBLIC ${KEYSTONE_SDK_DIR}/include/edge)

# workload host

add_executable(${workload_host_bin} ${workload_host_src})
target_link_libraries(${workload_host_bin} ${KEYSTONE_LIB_HOST} ${KEYSTONE_LIB_EDGE})

# workload packaging

add_keystone_package(${workload_eapp_bin}-package
  ${workload_package_name}
  ${workload_package_script}
  ${eyrie_files_to_copy} ${workload_eapp_bin} ${workload_host_bin})

add_dependencies(${workload_eapp_bin}-package workload-peripheral-eyrie)
add_dependencies(examples ${workload_eapp_bin}-package)

###############################################
# Peripheral package
###############################################
set(peripheral_eapp_bin peripheral-eapp)
set(peripheral_eapp_src peripheral/eapp/peripheral.c)
set(peripheral_host_bin peripheral-runner)
set(peripheral_host_src peripheral/host/host.cpp)
set(peripheral_package_name "peripheral.ke")
set(peripheral_package_script "./peripheral-runner peripheral-eapp eyrie-rt loader.bin")

# peripheral eapp

add_executable(${peripheral_eapp_bin} ${peripheral_eapp_src})
target_link_libraries(${peripheral_eapp_bin} "-static")

# peripheral host

add_executable(${peripheral_host_bin} ${peripheral_host_src})
target_link_libraries(${peripheral_host_bin} ${KEYSTONE_LIB_HOST} ${KEYSTONE_LIB_EDGE})

# peripheral packaging

add_keystone_package(${peripheral_eapp_bin}-package
  ${peripheral_package_name}
  ${peripheral_package_script}
  ${eyrie_files_to_copy} ${peripheral_eapp_bin} ${peripheral_host_bin})

add_dependencies(${peripheral_eapp_bin}-package workload-peripheral-eyrie)
add_dependencies(examples ${peripheral_eapp_bin}-package)
