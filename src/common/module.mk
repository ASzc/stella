MODULE := src/common

MODULE_OBJS := \
	src/common/Base.o \
	src/common/EventHandlerSDL2.o \
	src/common/FBSurfaceSDL2.o \
	src/common/FrameBufferSDL2.o \
	src/common/FSNodeZIP.o \
	src/common/main.o \
	src/common/MouseControl.o \
	src/common/PhysicalJoystick.o \
	src/common/PJoystickHandler.o \
	src/common/PKeyboardHandler.o \
	src/common/PNGLibrary.o \
	src/common/RewindManager.o \
	src/common/SoundSDL2.o \
	src/common/StateManager.o \
	src/common/ZipHandler.o \
	src/common/AudioQueue.o \
	src/common/AudioSettings.o

MODULE_DIRS += \
	src/common

# Include common rules
include $(srcdir)/common.rules
