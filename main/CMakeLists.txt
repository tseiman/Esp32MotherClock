

if(NOT CMAKE_BUILD_EARLY_EXPANSION)

# file(MAKE_DIRECTORY ${COMPONENT_DIR}/www)


add_custom_command(OUTPUT ${COMPONENT_DIR}/www
COMMAND ${CMAKE_COMMAND} -E make_directory ${COMPONENT_DIR}/www
)
add_custom_target(WWW_Target ALL DEPENDS)


add_custom_command(OUTPUT ${COMPONENT_DIR}/www/index.html.gz
COMMAND gzip < ${COMPONENT_DIR}/../resources/index.html > ${COMPONENT_DIR}/www/index.html.gz
DEPENDS ${COMPONENT_DIR}/../resources/index.html 
)
add_custom_target(IndexHTML_Target ALL DEPENDS ${COMPONENT_DIR}/../resources/index.html ${COMPONENT_DIR}/www/)


add_custom_command(OUTPUT ${COMPONENT_DIR}/www/favicon.ico.gz
COMMAND gzip < ${COMPONENT_DIR}/../resources/favicon.ico > ${COMPONENT_DIR}/www/favicon.ico.gz
DEPENDS ${COMPONENT_DIR}/../resources/favicon.ico
)
add_custom_target(Favico_Target ALL DEPENDS ${COMPONENT_DIR}/../resources/favicon.ico ${COMPONENT_DIR}/www/)

add_custom_command(OUTPUT ${COMPONENT_DIR}/www/favicon.png.gz
COMMAND gzip < ${COMPONENT_DIR}/../resources/32.png > ${COMPONENT_DIR}/www/favicon.png.gz
DEPENDS ${COMPONENT_DIR}/../resources/32.png
)
add_custom_target(FavicoPng_Target ALL DEPENDS ${COMPONENT_DIR}/../resources/32.png ${COMPONENT_DIR}/www/)


add_custom_command(OUTPUT ${COMPONENT_DIR}/www/jquery.slim.min.js.gz
COMMAND gzip < ${COMPONENT_DIR}/../resources/jquery.slim.min.js > ${COMPONENT_DIR}/www/jquery.slim.min.js.gz
DEPENDS ${COMPONENT_DIR}/../resources/jquery.slim.min.js
)
add_custom_target(JQuery_Target ALL DEPENDS ${COMPONENT_DIR}/../resources/jquery.slim.min.js ${COMPONENT_DIR}/www/)

add_custom_command(OUTPUT ${COMPONENT_DIR}/www/main.js.gz
COMMAND gzip < ${COMPONENT_DIR}/../resources/main.js > ${COMPONENT_DIR}/www/main.js.gz
DEPENDS ${COMPONENT_DIR}/../resources/main.js
)
add_custom_target(MainJs_Target ALL DEPENDS ${COMPONENT_DIR}/../resources/main.js ${COMPONENT_DIR}/www/)

add_custom_command(OUTPUT ${COMPONENT_DIR}/www/style.css.gz
COMMAND gzip < ${COMPONENT_DIR}/../resources/style.css > ${COMPONENT_DIR}/www/style.css.gz
DEPENDS ${COMPONENT_DIR}/../resources/style.css
)
add_custom_target(StyleCss_Target ALL DEPENDS ${COMPONENT_DIR}/../resources/style.css ${COMPONENT_DIR}/www/)


add_custom_command(OUTPUT ${COMPONENT_DIR}/www/clock-bg-img.jpg.gz
COMMAND gzip < ${COMPONENT_DIR}/../resources/clock-bg-img.jpg > ${COMPONENT_DIR}/www/clock-bg-img.jpg.gz
DEPENDS ${COMPONENT_DIR}/../resources/clock-bg-img.jpg
)
add_custom_target(ClockBgImg_Target ALL DEPENDS ${COMPONENT_DIR}/../resources/clock-bg-img.jpg ${COMPONENT_DIR}/www/)




set(WEB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../main/www")
# if(EXISTS ${WEB_SRC_DIR})
spiffs_create_partition_image(www ${WEB_SRC_DIR} FLASH_IN_PROJECT 
								DEPENDS 
								${COMPONENT_DIR}/www/
								${COMPONENT_DIR}/www/index.html.gz
								${COMPONENT_DIR}/www/favicon.ico.gz
								${COMPONENT_DIR}/www/jquery.slim.min.js.gz
								${COMPONENT_DIR}/www/main.js.gz
								${COMPONENT_DIR}/www/clock-bg-img.jpg.gz
								${COMPONENT_DIR}/www/style.css.gz
								${COMPONENT_DIR}/www/favicon.png.gz
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
                    INCLUDE_DIRS 	"."
                    EMBED_TXTFILES 	"certs/cacert.pem"
                                	"certs/prvtkey.pem")


