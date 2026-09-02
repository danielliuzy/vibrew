TARGET    := proxy
SRCDIR    := src
COMMONDIR := ../common
BUILD     := build

CXX      := clang++
CXXFLAGS := -std=c++20 -Wall -Wextra -g -O0 -MMD -MP
LDFLAGS  :=
LDLIBS   :=

# cpp-httplib needs this define to compile its TLS support, plus OpenSSL.
BREW     := /opt/homebrew/opt
CXXFLAGS += -DCPPHTTPLIB_OPENSSL_SUPPORT
CXXFLAGS += -I$(BREW)/cpp-httplib/include
CXXFLAGS += -I$(BREW)/nlohmann-json/include
CXXFLAGS += -I$(BREW)/openssl@3/include
# Protocol code shared with the console build.
CXXFLAGS += -I$(COMMONDIR)
LDFLAGS  += -L$(BREW)/openssl@3/lib
LDLIBS   += -lssl -lcrypto
# macOS: cpp-httplib reads the system trust store via Apple's Security framework.
LDLIBS   += -framework CoreFoundation -framework Security

# Uncomment for address/UB sanitizers (Mac-native debugging).
# CXXFLAGS += -fsanitize=address,undefined
# LDFLAGS  += -fsanitize=address,undefined

SRCS := $(wildcard $(SRCDIR)/*.cpp)
OBJS := $(patsubst $(SRCDIR)/%.cpp,$(BUILD)/%.o,$(SRCS))

# Objects from common/ are prefixed so they cannot collide with src/ basenames.
COMMON_SRCS := $(wildcard $(COMMONDIR)/*.cpp)
COMMON_OBJS := $(patsubst $(COMMONDIR)/%.cpp,$(BUILD)/common_%.o,$(COMMON_SRCS))

DEPS := $(OBJS:.o=.d) $(COMMON_OBJS:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJS) $(COMMON_OBJS)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD)/%.o: $(SRCDIR)/%.cpp | $(BUILD)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/common_%.o: $(COMMONDIR)/%.cpp | $(BUILD)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD):
	@mkdir -p $(BUILD)

clean:
	@rm -rf $(BUILD) $(TARGET)

-include $(DEPS)

.PHONY: all clean
