BUILD_DIR ?= build
BATS_CMD ?= bats
RUN_EXECUTABLE ?= $(BUILD_DIR)/examples/cpp_basic/fountain_cpp_basic
SEMGREP_CMD ?= semgrep
CLANG_TIDY_CMD ?= clang-tidy
GITLEAKS_CMD ?= gitleaks
SHELLCHECK_CMD ?= shellcheck
SHELL_SOURCES := 00_verify_requirements_traceability.sh 01_install_prerequisites.sh
CPP_SOURCES := $(shell rg --files src --glob '*.cpp')
CLEAN_PATHS := $(BUILD_DIR) .build CMakeCache.txt CMakeFiles

.PHONY: help build test run sast clean _sast_shell _sast_semgrep _sast_clang_tidy _sast_clang_tidy_report _sast_secrets

help:
	@echo "Fountain Make Targets"
	@echo "  build        Configure and compile with CMake"
	@echo "  test         Run ctest and Bats tests"
	@echo "  run          Execute configured repository binary"
	@echo "  sast         Run blocking and extended SAST lanes"
	@echo "  clean        Move generated artifacts to Trash"

build:
	#R010: Build lane runs deterministic CMake configure/build sequence.
	#R015: Build lane uses configurable top-level variables.
	#R040: Build lane emits concise status output.
	@echo "[build] Configuring CMake in $(BUILD_DIR)"
	cmake -S . -B "$(BUILD_DIR)"
	@echo "[build] Building targets in $(BUILD_DIR)"
	cmake --build "$(BUILD_DIR)"

test: build
	#R005: Test lane runs ctest and Bats tests.
	#R040: Test lane emits concise status output.
	@echo "[test] Running ctest suite"
	ctest --test-dir "$(BUILD_DIR)" --output-on-failure
	@echo "[test] Running Bats suite" #R005 #R040
	@if [ -d tests/sh ]; then \
	  TEST_FILES=$$(rg --files tests/sh --glob '*.bats'); \
	  if [ -z "$$TEST_FILES" ]; then \
	    echo "[test] No Bats tests found; skipping"; \
	  else \
	    $(BATS_CMD) $$TEST_FILES; \
	  fi; \
	else \
	  echo "[test] tests/sh not found; skipping Bats lane"; \
	fi

run: build
	#R020: Run lane validates configured executable path before launch.
	#R040: Run lane emits concise status output.
	@echo "[run] Checking executable at $(RUN_EXECUTABLE)"
	@if [ -x "$(RUN_EXECUTABLE)" ]; then \
	  echo "[run] Launching $(RUN_EXECUTABLE)"; \
	  "$(RUN_EXECUTABLE)"; \
	else \
	  echo "[run] Missing executable: $(RUN_EXECUTABLE)"; \
	  echo "[run] Set RUN_EXECUTABLE to a valid binary path and rerun"; \
	  exit 1; \
	fi

sast: _sast_shell _sast_semgrep _sast_clang_tidy _sast_secrets _sast_clang_tidy_report
	#R030: SAST lane aggregates blocking analysis checks.
	#R060: SAST lane also runs non-blocking extended reporting.
	#R040: SAST lane emits concise status output.
	@echo "[sast] Blocking checks passed and extended reporting completed"

_sast_shell:
	#R035: Shell lane checks shell sources with ShellCheck.
	@echo "[sast:shell] Running shellcheck"
	@if ! command -v "$(SHELLCHECK_CMD)" >/dev/null; then \
	  echo "[sast:shell] shellcheck missing; install and retry"; \
	  exit 1; \
	fi
	@if [ -z "$(strip $(SHELL_SOURCES))" ]; then \
	  echo "[sast:shell] No shell files discovered"; \
	else \
	  "$(SHELLCHECK_CMD)" $(SHELL_SOURCES); \
	fi

_sast_semgrep:
	#R045: Semgrep lane runs semgrep auto configuration.
	@echo "[sast:semgrep] Running semgrep auto rules"
	@if ! command -v "$(SEMGREP_CMD)" >/dev/null; then \
	  echo "[sast:semgrep] semgrep missing; install and retry"; \
	  exit 1; \
	fi
	"$(SEMGREP_CMD)" --config auto --error --quiet .

_sast_clang_tidy:
	#R050: Blocking clang-tidy lane scans repository C++ sources.
	#R065: Blocking lane remains separate from report-only behavior.
	@echo "[sast:clang-tidy] Running blocking clang-tidy checks"
	@if ! command -v "$(CLANG_TIDY_CMD)" >/dev/null; then \
	  echo "[sast:clang-tidy] clang-tidy missing; install and retry"; \
	  exit 1; \
	fi
	@if [ -z "$(CPP_SOURCES)" ]; then \
	  echo "[sast:clang-tidy] No C++ sources discovered"; \
	else \
	  "$(CLANG_TIDY_CMD)" $(CPP_SOURCES) -- -Iinclude -Isrc; \
	fi

_sast_clang_tidy_report:
	#R060: Report clang-tidy lane is non-blocking.
	#R065: Report lane remains separated from blocking checks.
	@echo "[sast:clang-tidy-report] Running non-blocking clang-tidy checks"
	@if ! command -v "$(CLANG_TIDY_CMD)" >/dev/null; then \
	  echo "[sast:clang-tidy-report] clang-tidy missing; skipping report"; \
	else \
	  if [ -z "$(CPP_SOURCES)" ]; then \
	    echo "[sast:clang-tidy-report] No C++ sources discovered"; \
	  else \
	    "$(CLANG_TIDY_CMD)" $(CPP_SOURCES) -- -Iinclude -Isrc || echo "[sast:clang-tidy-report] diagnostics reported"; \
	  fi; \
	fi

_sast_secrets:
	#R055: Secrets lane runs gitleaks detection.
	@echo "[sast:secrets] Running gitleaks detect"
	@if ! command -v "$(GITLEAKS_CMD)" >/dev/null; then \
	  echo "[sast:secrets] gitleaks missing; install and retry"; \
	  exit 1; \
	fi
	"$(GITLEAKS_CMD)" detect --source . --no-banner --redact --exit-code 1

clean:
	#R080: Clean lane moves generated artifacts to timestamped Trash paths.
	#R040: Clean lane emits concise status output.
	@echo "[clean] Moving generated artifacts to Trash"
	@timestamp=$$(date +%Y%m%d-%H%M%S); \
	trash_base="$$HOME/.Trash/fountain-clean-$$timestamp"; \
	mkdir -p "$$trash_base"; \
	for path in $(CLEAN_PATHS); do \
	  if [ -e "$$path" ]; then \
	    sanitized=$$(echo "$$path" | tr '/.' '__'); \
	    destination="$$trash_base/$$sanitized"; \
	    mv "$$path" "$$destination"; \
	    echo "[clean] moved $$path -> $$destination"; \
	  else \
	    echo "[clean] skip $$path (absent)"; \
	  fi; \
	done
