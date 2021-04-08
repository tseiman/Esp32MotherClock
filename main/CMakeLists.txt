


if(NOT CMAKE_BUILD_EARLY_EXPANSION)

add_custom_command(OUTPUT ${COMPONENT_DIR}/www/index.html.gz
COMMAND gzip < ${COMPONENT_DIR}/../resources/index.html > ${COMPONENT_DIR}/www/index.html.gz
DEPENDS ${COMPONENT_DIR}/../resources/index.html
)
add_custom_target(IndexHTML_Target ALL DEPENDS ${COMPONENT_DIR}/../resources/index.html)


endif()



idf_component_register(
		    SRCS 		"main.c"
                    INCLUDE_DIRS 	"."
		    EMBED_FILES		"www/index.html.gz"
                    EMBED_TXTFILES 	"certs/cacert.pem"
                                	"certs/prvtkey.pem")


#					"www/style.css.gz"
#					"www/main.js.gz"
#					"www/favicon.ico.gz"
#					"www/jquery.slim.min.js.gz"
