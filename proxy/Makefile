TARGET   := proxy
SRCDIR   := src
BUILD    := build

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
LDFLAGS  += -L$(BREW)/openssl@3/lib
LDLIBS   += -lssl -lcrypto
# macOS: cpp-httplib reads the system trust store via Apple's Security framework.
LDLIBS   += -framework CoreFoundation -framework Security

# Uncomment for address/UB sanitizers (Mac-native debugging).
# CXXFLAGS += -fsanitize=address,undefined
# LDFLAGS  += -fsanitize=address,undefined

SRCS := $(wildcard $(SRCDIR)/*.cpp)
OBJS := $(patsubst $(SRCDIR)/%.cpp,$(BUILD)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD)/%.o: $(SRCDIR)/%.cpp | $(BUILD)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD):
	@mkdir -p $(BUILD)

clean:
	@rm -rf $(BUILD) $(TARGET)

-include $(DEPS)

.PHONY: all clean
