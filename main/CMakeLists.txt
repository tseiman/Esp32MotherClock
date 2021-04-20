
set(RESOURCES_LIST 
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



if(NOT CMAKE_BUILD_EARLY_EXPANSION)
	list(LENGTH RESOURCES_LIST RESOURCES_LIST_FILE_COUNT)
	add_definitions(-DRESOURCES_LIST_FILE_COUNT=${RESOURCES_LIST_FILE_COUNT})


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



	foreach(RESOURCE IN LISTS RESOURCES_LIST)
		string(REPLACE "/" "_" TARGET_NAME ${RESOURCE})


		add_custom_command( OUTPUT ${COMPONENT_DIR}/www/${RESOURCE}.gz
							COMMAND gzip < ${COMPONENT_DIR}/../resources/${RESOURCE} > ${COMPONENT_DIR}/www/${RESOURCE}.gz
							DEPENDS ${COMPONENT_DIR}/../resources/${RESOURCE}
		)
		add_custom_target(${TARGET_NAME}_Resource_Target ALL DEPENDS ${COMPONENT_DIR}/../resources/${RESOURCE} ${COMPONENT_DIR}/www/ ${COMPONENT_DIR}/www/images/)
		list(APPEND RESOURCES_LIST_DEP ${COMPONENT_DIR}/www/${RESOURCE}.gz)
		message(STATUS "adding resource file ${RESOURCE}")
	endforeach()

# add_custom_command(OUTPUT ${COMPONENT_DIR}/www/index.html.gz
# COMMAND gzip < ${COMPONENT_DIR}/../resources/index.html > ${COMPONENT_DIR}/www/index.html.gz
#DEPENDS ${COMPONENT_DIR}/../resources/index.html 
#)
#add_custom_target(IndexHTML_Target ALL DEPENDS ${COMPONENT_DIR}/../resources/index.html ${COMPONENT_DIR}/www/)


#add_custom_command(OUTPUT ${COMPONENT_DIR}/www/favicon.ico.gz
#COMMAND gzip < ${COMPONENT_DIR}/../resources/favicon.ico > ${COMPONENT_DIR}/www/favicon.ico.gz
#DEPENDS ${COMPONENT_DIR}/../resources/favicon.ico
#)
# add_custom_target(Favico_Target ALL DEPENDS ${COMPONENT_DIR}/../resources/favicon.ico ${COMPONENT_DIR}/www/)

#add_custom_command(OUTPUT ${COMPONENT_DIR}/www/favicon.png.gz
#COMMAND gzip < ${COMPONENT_DIR}/../resources/32.png > ${COMPONENT_DIR}/www/favicon.png.gz
#DEPENDS ${COMPONENT_DIR}/../resources/32.png
#)
#add_custom_target(FavicoPng_Target ALL DEPENDS ${COMPONENT_DIR}/../resources/32.png ${COMPONENT_DIR}/www/)


#add_custom_command(OUTPUT ${COMPONENT_DIR}/www/jquery.min.js.gz
#COMMAND gzip < ${COMPONENT_DIR}/../resources/jquery.min.js > ${COMPONENT_DIR}/www/jquery.min.js.gz
#DEPENDS ${COMPONENT_DIR}/../resources/jquery.min.js
#)
#add_custom_target(JQuery_Target ALL DEPENDS ${COMPONENT_DIR}/../resources/jquery.min.js ${COMPONENT_DIR}/www/)

# add_custom_command(OUTPUT ${COMPONENT_DIR}/www/main.js.gz
#COMMAND gzip < ${COMPONENT_DIR}/../resources/main.js > ${COMPONENT_DIR}/www/main.js.gz
#DEPENDS ${COMPONENT_DIR}/../resources/main.js
#)
#add_custom_target(MainJs_Target ALL DEPENDS ${COMPONENT_DIR}/../resources/main.js ${COMPONENT_DIR}/www/)

#add_custom_command(OUTPUT ${COMPONENT_DIR}/www/style.css.gz
#COMMAND gzip < ${COMPONENT_DIR}/../resources/style.css > ${COMPONENT_DIR}/www/style.css.gz
#DEPENDS ${COMPONENT_DIR}/../resources/style.css
#)
#add_custom_target(StyleCss_Target ALL DEPENDS ${COMPONENT_DIR}/../resources/style.css ${COMPONENT_DIR}/www/)


#add_custom_command(OUTPUT ${COMPONENT_DIR}/www/clock-bg-img.jpg.gz
#COMMAND gzip < ${COMPONENT_DIR}/../resources/clock-bg-img.jpg > ${COMPONENT_DIR}/www/clock-bg-img.jpg.gz
#DEPENDS ${COMPONENT_DIR}/../resources/clock-bg-img.jpg
#)
# add_custom_target(ClockBgImg_Target ALL DEPENDS ${COMPONENT_DIR}/../resources/clock-bg-img.jpg ${COMPONENT_DIR}/www/)



#add_custom_command(OUTPUT ${COMPONENT_DIR}/www/main.part.html.gz
#COMMAND gzip < ${COMPONENT_DIR}/../resources/main.part.html > ${COMPONENT_DIR}/www/main.part.html.gz
#DEPENDS ${COMPONENT_DIR}/../resources/main.part.html.jpg
#)
#add_custom_target(ClockBgImg_Target ALL DEPENDS ${COMPONENT_DIR}/../resources/main.part.html.jpg ${COMPONENT_DIR}/www/)


#add_custom_command(OUTPUT ${COMPONENT_DIR}/www/jquery-ui.min.css.gz
#COMMAND gzip < ${COMPONENT_DIR}/../resources/jquery-ui.min.css > ${COMPONENT_DIR}/www/jquery-ui.min.css.gz
#DEPENDS ${COMPONENT_DIR}/../resources/jquery-ui.min.css
#)
#add_custom_target(ClockBgImg_Target ALL DEPENDS ${COMPONENT_DIR}/../resources/jquery-ui.min.css.jpg ${COMPONENT_DIR}/www/)




set(WEB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../main/www")
# if(EXISTS ${WEB_SRC_DIR})
spiffs_create_partition_image(www ${WEB_SRC_DIR} FLASH_IN_PROJECT 
								DEPENDS 
								${COMPONENT_DIR}/www/
								${COMPONENT_DIR}/www/images/
								${COMPONENT_DIR}/www/font/
								${RESOURCES_LIST_DEP}
#								${COMPONENT_DIR}/www/index.html.gz
#								${COMPONENT_DIR}/www/favicon.ico.gz
#								${COMPONENT_DIR}/www/jquery.min.js.gz
#								${COMPONENT_DIR}/www/main.js.gz
#								${COMPONENT_DIR}/www/clock-bg-img.jpg.gz
#								${COMPONENT_DIR}/www/style.css.gz
#								${COMPONENT_DIR}/www/favicon.png.gz
#								${COMPONENT_DIR}/www/main.part.html.gz
#								${COMPONENT_DIR}/www/jquery-ui.min.css.gz
)
# else()
#    message(FATAL_ERROR "${WEB_SRC_DIR} doesn't exist.")
# endif()

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


