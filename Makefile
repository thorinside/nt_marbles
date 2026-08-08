# nt_marbles reproducible hardware build.  Upstream DSP is a pinned submodule.
ARM_CXX ?= arm-none-eabi-g++
ARM_NM ?= arm-none-eabi-nm
ARM_READELF ?= arm-none-eabi-readelf
NATIVE_CXX ?= c++

TARGET := plugins/nt_marbles.o
PACKAGE := nt_marbles-plugin.zip
HOST_TEST := build/tests/host_boundary_test
PLUGIN_TEST := build/tests/plugin_integration_test
BUILD := build
UPSTREAM := third_party/eurorack
CXXFLAGS := -std=c++11 -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb \
	-Os -Wall -Wextra -fno-rtti -fno-exceptions -fPIC
# The upstream headers intentionally have conditionally unused arguments; do
# not edit source merely to silence that non-behavioral warning.
MARBLES_CXXFLAGS := $(CXXFLAGS) -Wno-unused-parameter
INCLUDES := -Ithird_party/distingNT_API/include -I$(UPSTREAM)
HOST_SOURCES := src/nt_marbles_plugin.cc
# These are upstream implementation files, not a curated copy.  Keep this
# list small and auditable while the host adapter is developed.
MARBLES_SOURCES := $(UPSTREAM)/marbles/random/t_generator.cc \
	$(UPSTREAM)/marbles/random/x_y_generator.cc \
	$(UPSTREAM)/marbles/random/output_channel.cc \
	$(UPSTREAM)/marbles/random/quantizer.cc \
	$(UPSTREAM)/marbles/random/discrete_distribution_quantizer.cc \
	$(UPSTREAM)/marbles/random/lag_processor.cc \
	$(UPSTREAM)/marbles/ramp/ramp_extractor.cc \
	$(UPSTREAM)/marbles/resources.cc \
	$(UPSTREAM)/stmlib/dsp/units.cc
HOST_OBJECTS := $(patsubst src/%.cc,$(BUILD)/host/%.o,$(HOST_SOURCES))
MARBLES_OBJECTS := $(patsubst $(UPSTREAM)/%.cc,$(BUILD)/upstream/%.o,$(MARBLES_SOURCES))

all: $(TARGET)

$(TARGET): $(HOST_OBJECTS) $(MARBLES_OBJECTS)
	@mkdir -p $(@D)
	$(ARM_CXX) -Wl,--relocatable -nostdlib -o $@ $^

$(BUILD)/host/%.o: src/%.cc
	@mkdir -p $(@D)
	# Host boundary includes pinned upstream headers with intentional unused args.
	$(ARM_CXX) $(MARBLES_CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD)/upstream/%.o: $(UPSTREAM)/%.cc
	@mkdir -p $(@D)
	$(ARM_CXX) $(MARBLES_CXXFLAGS) -I$(UPSTREAM) -c $< -o $@

inspect: $(TARGET)
	$(ARM_READELF) -h $(TARGET)
	$(ARM_NM) -g $(TARGET) | grep -E 'pluginEntry$$'

package: inspect
	@mkdir -p release/programs/plug-ins release/licenses
	cp $(TARGET) release/programs/plug-ins/
	cp THIRD_PARTY_NOTICES.md release/licenses/
	@git rev-parse HEAD > release/BUILD_COMMIT.txt
	@shasum -a 256 $(TARGET) > release/PLUGIN_SHA256.txt
	@(cd release && zip -qrFS ../$(PACKAGE) programs licenses BUILD_COMMIT.txt PLUGIN_SHA256.txt)
	@shasum -a 256 $(PACKAGE) > $(PACKAGE).sha256

verify-overlay:
	./scripts/apply-eurorack-patches.sh
	./scripts/remove-eurorack-patches.sh
	@test -z "$$(git -C $(UPSTREAM) status --porcelain)"

$(HOST_TEST): tests/host_boundary_test.cc src/nt_marbles_host_boundary.h
	@mkdir -p $(@D)
	$(NATIVE_CXX) -std=c++11 -Wall -Wextra -Werror -I. $< -o $@

$(PLUGIN_TEST): tests/plugin_integration_test.cc $(HOST_SOURCES) $(MARBLES_SOURCES) \
		src/marbles_factory_scales.h src/nt_marbles_host_boundary.h
	@mkdir -p $(@D)
	$(NATIVE_CXX) -std=c++11 -DTEST -Wall -Wextra -Werror \
		-Wno-unused-parameter $(INCLUDES) -Isrc \
		tests/plugin_integration_test.cc $(HOST_SOURCES) $(MARBLES_SOURCES) -o $@

test: $(HOST_TEST) $(PLUGIN_TEST)
	$(HOST_TEST)
	$(PLUGIN_TEST)
	python3 tests/control_surface_test.py
	python3 tests/routing_contract_test.py
	python3 tests/persistence_contract_test.py
	python3 tests/verification_contract_test.py

clean:
	rm -rf $(BUILD) plugins release nt_marbles-plugin.zip

.PHONY: all clean inspect package verify-overlay test
