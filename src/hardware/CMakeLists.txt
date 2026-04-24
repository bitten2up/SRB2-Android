target_sources(SRB2SDL2 PRIVATE
	hw_bsp.c
	hw_draw.c
	hw_light.c
	hw_main.c
	hw_clip.c
	hw_md2.c
	hw_cache.c
	hw_md2load.c
	hw_md3load.c
	hw_model.c
	hw_batching.c
	r_glcommon/r_glcommon.c
	shaders/gl_shaders.c
	hw_shaders.c
)

if(SRB2_CONFIG_HAVE_GLES2)
	target_compile_definitions(SRB2SDL2 PRIVATE -DHAVE_GLES -DHAVE_GLES2)
	target_sources(SRB2SDL2 PRIVATE r_gles/r_gles2.c)
elseif(SRB2_CONFIG_HAVE_GLES)
	target_compile_definitions(SRB2SDL2 PRIVATE -DHAVE_GLES)
	target_sources(SRB2SDL2 PRIVATE r_gles/r_gles1.c)
else()
	target_sources(SRB2SDL2 PRIVATE r_opengl/r_opengl.c)
endif()
