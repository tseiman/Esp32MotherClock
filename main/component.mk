#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

COMPONENT_EMBED_TXTFILES := certs/cacert.pem
COMPONENT_EMBED_TXTFILES += certs/prvtkey.pem


COMPONENT_EMBED_FILES := www/index.html.gz
# COMPONENT_EMBED_FILES += www/style.css.gz 
# COMPONENT_EMBED_FILES += www/main.js.gz
# COMPONENT_EMBED_FILES += www/favicon.ico.gz 
# COMPONENT_EMBED_FILES += www/jquery.slim.min.js.gz
