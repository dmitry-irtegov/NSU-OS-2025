BUILD_DIR = build
SRC_DIR = src
TARGET_EXEC = $(BUILD_DIR)/proxy.out

CCC = g++
CPPFLAGS = -Wall -I$(SRC_DIR) -MMD -MP -std=c++17 -g

SOURCES = \
	Connection.cpp \
	Http.cpp \
	MessageBuffer.cpp \
	Proxy.cpp \
	ResponseCache.cpp \
	ErrorResponse.cpp \
	main.cpp
LIBS = -lsocket -lnsl

OBJECTS = $(SOURCES:%.cpp=$(BUILD_DIR)/%.o)
DEPS = $(SOURCES:%.cpp=$(BUILD_DIR)/%.d)

$(TARGET_EXEC): $(OBJECTS)
	$(LINK.cc) $(LIBS) $(OBJECTS) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(COMPILE.cc) $< -o $@

$(BUILD_DIR)/%.d: $(BUILD_DIR)/%.o
	@: # Needed for GNU Make

include $(DEPS)
