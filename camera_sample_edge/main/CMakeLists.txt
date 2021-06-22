# #set(COMPONENT_SRCS "app_main.c" "app_wifi.c" "app_camera.c" "app_httpd.c" "app_mdns.c" "app_garbage.c")

# set(COMPONENT_SRCS "app_main.c" "app_wifi.c" "app_camera.c" "app_sample.c" "app_tensorflow.cc" "model_settings.c" "tf_model/model_data_28_28.cc")
# set(COMPONENT_ADD_INCLUDEDIRS "lib/include" "include")

# set(COMPONENT_REQUIRES
#     esp32-camera
#     #esp-face
#     nvs_flash
#     #esp_http_server
#     #esp_http_client
#     mqtt
#     fb_gfx
#     mdns
#     )

# # set(COMPONENT_EMBED_FILES
# #         "www/index_ov2640.html.gz"
# #         "www/index_ov3660.html.gz"
# #         "www/index_ov5640.html.gz"
# #         "www/monitor.html.gz")

# register_component()
# target_link_libraries(${COMPONENT_TARGET} "-L ${CMAKE_CURRENT_SOURCE_DIR}/lib")
idf_component_register(SRCS app_main.c app_mdns.c app_wifi.c app_camera.c app_tensorflow.cc model_settings.c image_util/image_util.c tf_model/model_data_28_28_3.cc
                    INCLUDE_DIRS . image_util/include lib/include include 
                    )

target_link_libraries(${COMPONENT_TARGET} "-L ${CMAKE_CURRENT_SOURCE_DIR}/lib")