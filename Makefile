#----------------------------------------------------------------------------
#       MAKEFILE
#
#	Controlling makefile for SWFPlayer
#
#	Created:	1st August 2005
#
#	Copyright (C) 1995-2005 71M
#----------------------------------------------------------------------------

#----------------------------------------------------------------------------
#	Target to make
#	--------------

TARGET :=					SWFPlayer

#----------------------------------------------------------------------------
#	Project folders
#	---------------

SOURCE_DIR :=				./Source
INCLUDE_DIR :=				./Source

GAMESWF_DIR :=				$(SOURCE_DIR)/GameSWF

TINY_XML_DIR :=				$(SOURCE_DIR)/TinyXML
TINY_XML_INCLUDE_DIR :=		$(TINY_XML_DIR)

SDL_INCLUDE_DIR :=			$(shell psp-config --pspdev-path)/psp/include/sdl

FFMPEG_DIR :=				../../FFMpeg/trunk
FFMPEG_LIB_DIR :=			$(FFMPEG_DIR)/libavformat \
							$(FFMPEG_DIR)/libavcodec \
							$(FFMPEG_DIR)/libavutil \
							$(FFMPEG_DIR)/ffplay_resample

#----------------------------------------------------------------------------
#	Source to make
#	--------------

SWF_OBJS :=					$(SOURCE_DIR)/Main.o \
							$(SOURCE_DIR)/CMath.o \
							$(SOURCE_DIR)/CAssert.o \
							$(SOURCE_DIR)/CFrameWork.o \
							$(SOURCE_DIR)/CGfx.o \
							$(SOURCE_DIR)/CVector.o \
							$(SOURCE_DIR)/CUSBManager.o \
							$(SOURCE_DIR)/CInput.o \
							$(SOURCE_DIR)/CString.o \
							$(SOURCE_DIR)/CFileSystem.o \
							$(SOURCE_DIR)/CTextureManager.o \
							$(SOURCE_DIR)/PNGReader.o \
							$(SOURCE_DIR)/CFont.o \
							$(SOURCE_DIR)/CProcess.o \
							$(SOURCE_DIR)/CSizedItem.o \
							$(SOURCE_DIR)/CConfigFile.o \
							$(SOURCE_DIR)/CWindow.o \
							$(SOURCE_DIR)/CWindowItem.o \
							$(SOURCE_DIR)/CWindowTable.o \
							$(SOURCE_DIR)/CWindowText.o \
							$(SOURCE_DIR)/CDirectoryList.o \
							$(SOURCE_DIR)/CSkinManager.o \
							$(SOURCE_DIR)/CMessageBox.o \
							$(SOURCE_DIR)/CBackground.o \
							$(SOURCE_DIR)/CRenderable.o \
							$(SOURCE_DIR)/CFileHandler.o \
							$(SOURCE_DIR)/CHUD.o \
							$(SOURCE_DIR)/CSWFPlayer.o \
							$(SOURCE_DIR)/CSWFFileHandler.o

#							$(SOURCE_DIR)/HeapMalloc.o \
#							$(SOURCE_DIR)/HeapMemory.o \
#							$(SOURCE_DIR)/PSPHeapMemory.o

TINY_XML_OBJS :=			$(TINY_XML_DIR)/tinystr.o \
							$(TINY_XML_DIR)/tinyxml.o \
							$(TINY_XML_DIR)/tinyxmlerror.o \
							$(TINY_XML_DIR)/tinyxmlparser.o

GAMESWF_OBJS :=				$(GAMESWF_DIR)/gameswf_sound_handler_sdl.o \
							$(GAMESWF_DIR)/gameswf_sound_handler_mp3.o \
							$(GAMESWF_DIR)/gameswf_render_handler_psp.o \
							$(GAMESWF_DIR)/gameswf_videostream.o \
							$(GAMESWF_DIR)/gameswf_sprite.o \
							$(GAMESWF_DIR)/gameswf_movie_root.o \
							$(GAMESWF_DIR)/gameswf_movie_def_impl.o \
							$(GAMESWF_DIR)/gameswf_impl.o \
							$(GAMESWF_DIR)/gameswf_action.o \
							$(GAMESWF_DIR)/gameswf_button.o \
							$(GAMESWF_DIR)/gameswf_dlist.o \
							$(GAMESWF_DIR)/gameswf_font.o \
							$(GAMESWF_DIR)/gameswf_fontlib.o \
							$(GAMESWF_DIR)/gameswf_log.o \
							$(GAMESWF_DIR)/gameswf_morph2.o \
							$(GAMESWF_DIR)/gameswf_movie.o \
							$(GAMESWF_DIR)/gameswf_render.o \
							$(GAMESWF_DIR)/gameswf_shape.o \
							$(GAMESWF_DIR)/gameswf_sound.o \
							$(GAMESWF_DIR)/gameswf_stream.o \
							$(GAMESWF_DIR)/gameswf_string.o \
							$(GAMESWF_DIR)/gameswf_styles.o \
							$(GAMESWF_DIR)/gameswf_tesselate.o \
							$(GAMESWF_DIR)/gameswf_text.o \
							$(GAMESWF_DIR)/gameswf_textformat.o \
							$(GAMESWF_DIR)/gameswf_timers.o \
							$(GAMESWF_DIR)/gameswf_types.o \
							$(GAMESWF_DIR)/gameswf_xml.o \
							$(GAMESWF_DIR)/gameswf_xmlsocket.o \
							$(GAMESWF_DIR)/base/container.o \
							$(GAMESWF_DIR)/base/image.o \
							$(GAMESWF_DIR)/base/jpeg.o \
							$(GAMESWF_DIR)/base/utf8.o \
							$(GAMESWF_DIR)/base/tu_file.o \
							$(GAMESWF_DIR)/base/tu_random.o \
							$(GAMESWF_DIR)/base/zlib_adapter.o \
							$(GAMESWF_DIR)/base/membuf.o \
							$(GAMESWF_DIR)/base/tu_timer.o \
							$(GAMESWF_DIR)/base/tu_types.o

#							$(GAMESWF_DIR)/gameswf_sound_handler_psp.o \

#----------------------------------------------------------------------------
#	All objects to make
#	-------------------

OBJS :=						$(GAMESWF_OBJS) \
							$(SWF_OBJS) \
							$(TINY_XML_OBJS)

#----------------------------------------------------------------------------
#	Additional includes
#	-------------------

INCDIR   :=					$(INCDIR) \
							$(INCLUDE_DIR) \
							$(GAMESWF_DIR) \
							$(TINY_XML_INCLUDE_DIR) \
							$(SDL_INCLUDE_DIR) \
							$(FFMPEG_DIR) \
							$(FFMPEG_LIB_DIR)

#----------------------------------------------------------------------------
#	Addditional libraries
#	---------------------

SDK_LIBS :=					-lpsprtc \
							-lpspsdk \
							-lpsputility \
							-lpspctrl \
							-lpspusb \
							-lpspaudio \
							-lpspusbstor \
							-lpsphprm \
							-lpsppower \
							-lpspgu

EXTERN_LIBS :=				-ljpeg \
							-lpng \
							-lSDL_mixer \
							-lSDL \
							-lmad \
							-lavformat \
							-lavutil \
							-lavcodec \
							-lz

LIBS :=						$(EXTERN_LIBS) \
							$(SDK_LIBS) \
							-lm \
							-lstdc++

#----------------------------------------------------------------------------
#	Preprocesser defines
#	--------------------

DEFINES :=					-DTU_CONFIG_LINK_TO_JPEGLIB=1 \
							-DTU_CONFIG_LINK_TO_ZLIB=1 \
							-DHAVE_AV_CONFIG_H \
							-D_WCHAR_T_DEFINED \
							-D_REENTRANT \
							-DGAMESWF_MP3_SUPPORT \
							-DPSP

#							-DKERNEL_MODE \
#							-D_DEBUG \
#							-DUSE_LOG_FILE \

#----------------------------------------------------------------------------
#	Compiler settings
#	-----------------

CFLAGS :=					$(DEFINES) -O3 -G0 -g -Wall -funroll-loops -fstrict-aliasing -fsched-interblock -falign-loops=16 -falign-jumps=16 -falign-functions=16 -ffast-math -fstrict-aliasing
CXXFLAGS :=					$(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS :=					$(CFLAGS)

LIBDIR :=					$(FFMPEG_DIR)

LDFLAGS :=					

#----------------------------------------------------------------------------
#	PBP parameters
#	--------------

EXTRA_TARGETS :=			EBOOT.PBP
PSP_EBOOT_ICON :=			ICON0.PNG
PSP_EBOOT_PIC1 :=			PIC1.PNG
PSP_EBOOT_TITLE :=			$(TARGET)

#----------------------------------------------------------------------------
#	Default build settings
#	----------------------

PSPSDK :=					$(shell psp-config --pspsdk-path)

include						$(PSPSDK)/lib/build.mak

#----------------------------------------------------------------------------
#	Copy to PSP
#	-----------

ifneq ($VS_PATH),)
CC       = psp-gcc
CXX      = psp-g++
endif

kx-install: kxploit
ifeq ($(PSP_MOUNT),)
		@echo '*** Error: $$(PSP_MOUNT) undefined. Please set it to for example /cygdrive/e'
		@echo if your PSP is mounted to E: in cygwin.
else
		cp -r $(TARGET) $(PSP_MOUNT)/PSP/GAME/
		cp -r $(TARGET)% $(PSP_MOUNT)/PSP/GAME/
		cp -r -u "Data" $(PSP_MOUNT)/PSP/GAME/$(TARGET)
endif
