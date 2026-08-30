TARGET   := proxy
SRCDIR   := src
BUILD    := build

CXX      := clang++
CXXFLAGS := -std=c++20 -Wall -Wextra -g -O0 -MMD -MP
LDFLAGS  :=
LDLIBS   :=

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
