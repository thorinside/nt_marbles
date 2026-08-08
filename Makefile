# nt_marbles reproducible hardware build.  Upstream DSP is a pinned submodule.
ARM_CXX ?= arm-none-eabi-g++
ARM_NM ?= arm-none-eabi-nm
ARM_READELF ?= arm-none-eabi-readelf
NATIVE_CXX ?= c++

TARGET := plugins/nt_marbles.o
PACKAGE := nt_marbles-plugin.zip
HOST_TEST := build/tests/host_boundary_test
PLUGIN_TEST := build/tests/plugin_integration_test
ENGINE_MATRIX_CONFIGS := 32000_24 44100_24 48000_4 48000_24 \
	48000_256 88200_24 96000_24
ENGINE_MATRIX_TESTS := $(addprefix build/tests/plugin_integration_test_,\
	$(ENGINE_MATRIX_CONFIGS))
BUILD := build
UPSTREAM := third_party/eurorack
OVERLAY := $(BUILD)/eurorack-overlay
CXXFLAGS := -std=c++11 -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb \
	-Os -Wall -Wextra -fno-rtti -fno-exceptions -fPIC
# The upstream headers intentionally have conditionally unused arguments; do
# not edit source merely to silence that non-behavioral warning.
MARBLES_CXXFLAGS := $(CXXFLAGS) -Wno-unused-parameter
INCLUDES := -Ithird_party/distingNT_API/include -I$(OVERLAY)
HOST_SOURCES := src/nt_marbles_plugin.cc
# These are upstream implementation files, not a curated copy.  Keep this
# list small and auditable while the host adapter is developed.
MARBLES_SOURCES := $(OVERLAY)/marbles/random/t_generator.cc \
	$(OVERLAY)/marbles/random/x_y_generator.cc \
	$(OVERLAY)/marbles/random/output_channel.cc \
	$(OVERLAY)/marbles/random/quantizer.cc \
	$(OVERLAY)/marbles/random/discrete_distribution_quantizer.cc \
	$(OVERLAY)/marbles/random/lag_processor.cc \
	$(OVERLAY)/marbles/ramp/ramp_extractor.cc \
	$(OVERLAY)/marbles/resources.cc \
	$(OVERLAY)/stmlib/dsp/units.cc
HOST_OBJECTS := $(patsubst src/%.cc,$(BUILD)/host/%.o,$(HOST_SOURCES))
MARBLES_OBJECTS := $(patsubst $(OVERLAY)/%.cc,$(BUILD)/upstream/%.o,$(MARBLES_SOURCES))

all: $(TARGET)

$(TARGET): $(HOST_OBJECTS) $(MARBLES_OBJECTS)
	@mkdir -p $(@D)
	$(ARM_CXX) -Wl,--relocatable -nostdlib -o $@ $^

$(OVERLAY)/.prepared: scripts/prepare-eurorack-overlay.sh \
		$(wildcard patches/eurorack/*.patch)
	./scripts/prepare-eurorack-overlay.sh
	@touch $@

$(MARBLES_SOURCES): $(OVERLAY)/.prepared

$(BUILD)/host/%.o: src/%.cc $(OVERLAY)/.prepared
	@mkdir -p $(@D)
	# Host boundary includes pinned upstream headers with intentional unused args.
	$(ARM_CXX) $(MARBLES_CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD)/upstream/%.o: $(OVERLAY)/%.cc $(OVERLAY)/.prepared
	@mkdir -p $(@D)
	$(ARM_CXX) $(MARBLES_CXXFLAGS) -I$(OVERLAY) -c $< -o $@

inspect: $(TARGET)
	$(ARM_READELF) -h $(TARGET)
	$(ARM_NM) -g $(TARGET) | grep -E 'pluginEntry$$'
	! $(ARM_NM) -u $(TARGET) | grep -E '__aeabi_'

package: inspect
	@mkdir -p release/programs/plug-ins release/licenses
	cp $(TARGET) release/programs/plug-ins/
	cp LICENSE release/licenses/nt_marbles-LICENSE.txt
	cp THIRD_PARTY_NOTICES.md release/licenses/
	@git rev-parse HEAD > release/BUILD_COMMIT.txt
	@shasum -a 256 $(TARGET) > release/PLUGIN_SHA256.txt
	@(cd release && zip -qrFS ../$(PACKAGE) programs licenses BUILD_COMMIT.txt PLUGIN_SHA256.txt)
	@shasum -a 256 $(PACKAGE) > $(PACKAGE).sha256

verify-overlay:
	./scripts/apply-eurorack-patches.sh
	./scripts/remove-eurorack-patches.sh
	@test -z "$$(git -C $(UPSTREAM) status --porcelain)"
	./scripts/prepare-eurorack-overlay.sh

$(HOST_TEST): tests/host_boundary_test.cc src/nt_marbles_host_boundary.h
	@mkdir -p $(@D)
	$(NATIVE_CXX) -std=c++11 -Wall -Wextra -Werror -I. $< -o $@

$(PLUGIN_TEST): tests/plugin_integration_test.cc $(HOST_SOURCES) $(MARBLES_SOURCES) \
		src/marbles_factory_scales.h src/nt_marbles_host_boundary.h
	@mkdir -p $(@D)
	$(NATIVE_CXX) -std=c++11 -DTEST -Wall -Wextra -Werror \
		-Wno-unused-parameter $(INCLUDES) -Isrc \
		tests/plugin_integration_test.cc $(HOST_SOURCES) $(MARBLES_SOURCES) -o $@

define ENGINE_MATRIX_RULE
build/tests/plugin_integration_test_$(1): tests/plugin_integration_test.cc \
		$$(HOST_SOURCES) $$(MARBLES_SOURCES) src/marbles_factory_scales.h \
		src/nt_marbles_host_boundary.h
	@mkdir -p $$(@D)
	$$(NATIVE_CXX) -std=c++11 -DTEST \
		-DNT_TEST_SAMPLE_RATE=$$(word 1,$$(subst _, ,$(1))) \
		-DNT_TEST_FRAMES=$$(word 2,$$(subst _, ,$(1))) \
		-Wall -Wextra -Werror -Wno-unused-parameter $$(INCLUDES) -Isrc \
		tests/plugin_integration_test.cc $$(HOST_SOURCES) $$(MARBLES_SOURCES) -o $$@
endef
$(foreach config,$(ENGINE_MATRIX_CONFIGS),$(eval $(call ENGINE_MATRIX_RULE,$(config))))

engine-matrix: $(ENGINE_MATRIX_TESTS)
	@for matrix_test in $^; do $$matrix_test; done
	@echo "engine configuration matrix passed"

test: $(HOST_TEST) $(PLUGIN_TEST) engine-matrix
	$(HOST_TEST)
	$(PLUGIN_TEST)
	python3 tests/control_surface_test.py
	python3 tests/routing_contract_test.py
	python3 tests/persistence_contract_test.py
	python3 tests/verification_contract_test.py

clean:
	rm -rf $(BUILD) plugins release $(PACKAGE) $(PACKAGE).sha256

.PHONY: all clean engine-matrix inspect package verify-overlay test
