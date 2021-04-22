
set(RESOURCES_LIST_WWW 
	index.html
	favicon.ico
	favicon32.png
	jquery.min.js
	main.js
	style.css
	clock-bg-img.jpg
	main.part.html
	jquery-ui.min.css
	jquery-ui.min.js
	jquery-ui.theme.min.css
	/images/ui-bg_diagonals-medium_100_aaaaaa_40x40.png
	/images/ui-icons_444444_256x240.png
	/images/ui-icons_555555_256x240.png
	/images/ui-icons_777620_256x240.png
	/images/ui-icons_777777_256x240.png
	/images/ui-icons_cc0000_256x240.png
	/images/ui-icons_ffffff_256x240.png
	jquery-ui.icon-font.min.css
	/font/jquery-ui.eot
	/font/jquery-ui.svg
	/font/jquery-ui.ttf
	/font/jquery-ui.woff
	/font/jquery-ui.woff2
)


set(RESOURCES_LIST_ETC
	configMap.json
)


if(NOT CMAKE_BUILD_EARLY_EXPANSION)
	list(LENGTH RESOURCES_LIST_WWW RESOURCES_LIST_WWW_FILE_COUNT)
	list(LENGTH RESOURCES_LIST_ETC RESOURCES_LIST_ETC_FILE_COUNT)
	add_definitions(-DRESOURCES_LIST_WWW_FILE_COUNT=${RESOURCES_LIST_WWW_FILE_COUNT} -DRESOURCES_LIST_ETC_FILE_COUNT=${RESOURCES_LIST_ETC_FILE_COUNT})

	add_custom_command(OUTPUT ${COMPONENT_DIR}/www
	COMMAND ${CMAKE_COMMAND} -E make_directory ${COMPONENT_DIR}/www
	)
	add_custom_target(WWW_Target ALL DEPENDS)

	add_custom_command(OUTPUT ${COMPONENT_DIR}/www/font
	COMMAND ${CMAKE_COMMAND} -E make_directory ${COMPONENT_DIR}/www/font
	)
	add_custom_target(WWW_Font_Target ALL DEPENDS)

	add_custom_command(OUTPUT ${COMPONENT_DIR}/www/images
	COMMAND ${CMAKE_COMMAND} -E make_directory ${COMPONENT_DIR}/www/images
	)
	add_custom_target(WWW_Img_Target ALL DEPENDS)



	foreach(RESOURCE IN LISTS RESOURCES_LIST_WWW)
		string(REPLACE "/" "_" TARGET_NAME ${RESOURCE})


		add_custom_command( OUTPUT ${COMPONENT_DIR}/www/${RESOURCE}.gz
							COMMAND gzip < ${COMPONENT_DIR}/../resources/${RESOURCE} > ${COMPONENT_DIR}/www/${RESOURCE}.gz
							DEPENDS ${COMPONENT_DIR}/../resources/${RESOURCE}
		)
		add_custom_target(${TARGET_NAME}_Resource_Target ALL DEPENDS ${COMPONENT_DIR}/../resources/${RESOURCE} ${COMPONENT_DIR}/www/ ${COMPONENT_DIR}/www/images/)
		list(APPEND RESOURCES_LIST_WWW_DEP ${COMPONENT_DIR}/www/${RESOURCE}.gz)
		message(STATUS "adding resource file ${RESOURCE}")
	endforeach()


	foreach(RESOURCE IN LISTS RESOURCES_LIST_ETC)
		list(APPEND RESOURCES_LIST_ETC_DEP ${COMPONENT_DIR}/etc/${RESOURCE})
		message(STATUS "adding etc file ${RESOURCE}")
	endforeach()



set(WEB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../main/www")
# if(EXISTS ${WEB_SRC_DIR})
spiffs_create_partition_image(www ${WEB_SRC_DIR} FLASH_IN_PROJECT 
								DEPENDS 
								${COMPONENT_DIR}/www/
								${COMPONENT_DIR}/www/images/
								${COMPONENT_DIR}/www/font/
								${RESOURCES_LIST_WWW_DEP}

)

set(ETC_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../main/etc")

spiffs_create_partition_image(etc ${ETC_SRC_DIR} FLASH_IN_PROJECT 
								DEPENDS 
								${RESOURCES_LIST_ETC_DEP}
)


endif()


idf_component_register(
		    SRCS 	"main.c"
					"staticHttpServer.c"
					"websocket.c"
					"filesystem.c"
					"commandHandler.c"
                    INCLUDE_DIRS 	"."
                    EMBED_TXTFILES 	"certs/cacert.pem"
                                	"certs/prvtkey.pem")


