# Build type (dev or prod)
BUILD_TYPE ?= dev
CONAN_FILE = conanfile.$(if $(filter dev,$(BUILD_TYPE)),dev.,)txt

# conan definitions
CONAN_INCLUDE_DIR_FLAG ?= -I
CONAN_LIB_DIR_FLAG ?= -L
CONAN_LIB_FLAG ?= -l
# Include Conan-generated make variables
CONANDEPS_FILE = build/conandeps.mk
# include $(CONANDEPS_FILE)

# Create framework flags
FRAMEWORK_FLAGS = $(addprefix -framework ,$(CONAN_FRAMEWORKS))

SOURCE = src/*/*.cpp 
EXECUTABLE = okinawa

# first target in the makefile is the default target 
# (if you run make without arguments)
all: dev

engine-install:
	clang++ $(SOURCE) $(CONAN_INCLUDE_DIRS) $(CONAN_LIB_DIRS) \
			-o $(EXECUTABLE) \
			$(CONAN_LIBS) \
			$(FRAMEWORK_FLAGS)

engine-clean: 
	@rm -f $(EXECUTABLE)

conan-clean:
	@echo "Cleaning Conan generated files..."
	@rm -f build/conandeps.mk
	@rm -f build/conan*
	@rm -f build/*conan*
	@rm -rf build/generators
	@rm -rf .conan

clean: engine-clean conan-clean test-clean

compile: engine-install

# debug:
# 	@echo "CONAN_INCLUDE_DIRS: $(CONAN_INCLUDE_DIRS)"
# 	@echo "CONAN_LIB_DIRS: $(CONAN_LIB_DIRS)"
# 	@echo "CONAN_LIBS: $(CONAN_LIBS)"
# 	@echo "FRAMEWORK_FLAGS: $(FRAMEWORK_FLAGS)"

# ------------------------------------------------------------------
# Library installation and building with conan
# ------------------------------------------------------------------

install:
	$(info Installing needed libraries:)
	@if [ ! -f $(CONANDEPS_FILE) ]; then \
		echo "Dependencies not found. Generating..." && \
		conan profile detect --force; \
		conan install $(CONAN_FILE) --build=missing --output-folder build/; \
	else \
		echo ; \
		echo "Dependencies found. To force regeneration, run:"; \
		echo "conan install $(CONAN_FILE) --build=missing --output-folder build/"; \
		echo ; \
	fi

# ------------------------------------------------------------------
# Development build
# ------------------------------------------------------------------

_dev1:
	$(info Building for development environment:)
	$(eval BUILD_TYPE := dev)
	$(eval CONAN_FILE := conanfile.dev.txt)
_dev2: install
_dev3:
	$(eval include $(CONANDEPS_FILE))
_dev4: engine-install
_dev5:
	$(info Build process for development finished.)
dev: _dev1 _dev2 _dev3 _dev4 _dev5

# ------------------------------------------------------------------
# Production build
# ------------------------------------------------------------------

_prod1:
	$(info Building for production environment:)
	$(eval BUILD_TYPE := prod)
	$(eval CONAN_FILE := conanfile.txt)
_prod2: install
_prod3:
	$(eval include $(CONANDEPS_FILE))
_prod4: engine-install
_prod5:
	$(info Build process for production finished.)
prod: _prod1 _prod2 _prod3 _prod4 _prod5

# ------------------------------------------------------------------
# Testing
# ------------------------------------------------------------------

# Test configuration
TEST_SOURCE = tests/*.cpp src/*/*.cpp
TEST_EXECUTABLE = tests/okinawa_test
COVERAGE_DIR = coverage

test-clean:
	@rm -f tests/*.profraw tests/*.profdata $(TEST_EXECUTABLE)
	@rm -rf $(COVERAGE_DIR)

test: install
	$(info Building and running tests...)
	$(eval include $(CONANDEPS_FILE))
	@clang++ -std=c++17 -fprofile-instr-generate -fcoverage-mapping \
			$(TEST_SOURCE) $(CONAN_INCLUDE_DIRS) $(CONAN_LIB_DIRS) \
			-o $(TEST_EXECUTABLE) \
			$(CONAN_LIBS) \
			$(FRAMEWORK_FLAGS)
	@LLVM_PROFILE_FILE="tests/okinawa.profraw" ./$(TEST_EXECUTABLE) --reporter=console
	@xcrun llvm-profdata merge -sparse tests/okinawa.profraw -o tests/okinawa.profdata
	@xcrun llvm-cov show ./$(TEST_EXECUTABLE) \
					-instr-profile=tests/okinawa.profdata \
					-format=html \
					-show-line-counts-or-regions \
          -show-instantiations \
					-ignore-filename-regex='.*tests' \
					-output-dir=$(COVERAGE_DIR) \
					$(TEST_SOURCE)
# -show-branches=count
	@echo "Coverage report generated in coverage/index.html"
