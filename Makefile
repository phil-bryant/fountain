BUILD_DIR ?= build
BATS_CMD ?= bats
RUN_EXECUTABLE ?= $(BUILD_DIR)/examples/cpp_basic/fountain_cpp_basic
SEMGREP_CMD ?= semgrep
CLANG_TIDY_CMD ?= clang-tidy
CLANG_TIDY_CHECKS ?= clang-analyzer-*,bugprone-*,performance-*,portability-*
CLANG_TIDY_FIRST_PARTY_PATH_REGEX ?= (^src/|^include/|$(CURDIR)/src/|$(CURDIR)/include/)
CLANG_TIDY_THIRD_PARTY_PATH_REGEX ?= build/_deps/
GITLEAKS_CMD ?= gitleaks
SHELLCHECK_CMD ?= shellcheck
SHELL_SOURCES := 00_verify_requirements_traceability.sh 01_install_prerequisites.sh
CPP_SOURCES := $(shell rg --files src --glob '*.cpp')
CLEAN_PATHS := $(BUILD_DIR) .build CMakeCache.txt CMakeFiles

.PHONY: help build test run sast clean _sast_shell _sast_semgrep _sast_clang_tidy _sast_clang_tidy_report _sast_secrets _sast_prepare_compile_db _sast_tool_header

#R001: Help target lists consolidated developer entrypoints.
#R040: Help target emits concise operator-readable status output.
help:
	@echo "Fountain Make Targets"
	@echo "  build        Configure and compile with CMake"
	@echo "  test         Run ctest and Bats tests"
	@echo "  run          Execute configured repository binary"
	@echo "  sast         Run blocking and extended SAST lanes"
	@echo "  clean        Move generated artifacts to Trash"
#R010: Build lane runs deterministic CMake configure/build sequence.
#R015: Build lane uses configurable top-level variables.
#R040: Build lane emits concise status output.
build:
	@echo "[build] Configuring CMake in $(BUILD_DIR)"
	cmake -S . -B "$(BUILD_DIR)"
	@echo "[build] Building targets in $(BUILD_DIR)"
	cmake --build "$(BUILD_DIR)"

#R005: Test lane runs ctest and Bats tests.
#R040: Test lane emits concise status output.
test: build
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

#R020: Run lane validates configured executable path before launch.
#R040: Run lane emits concise status output.
run: build
	@echo "[run] Checking executable at $(RUN_EXECUTABLE)"
	@if [ -x "$(RUN_EXECUTABLE)" ]; then \
	  echo "[run] Launching $(RUN_EXECUTABLE)"; \
	  "$(RUN_EXECUTABLE)"; \
	else \
	  echo "[run] Missing executable: $(RUN_EXECUTABLE)"; \
	  echo "[run] Set RUN_EXECUTABLE to a valid binary path and rerun"; \
	  exit 1; \
	fi

#R030: SAST lane aggregates blocking analysis checks.
#R060: SAST lane also runs non-blocking extended reporting.
#R040: SAST lane emits concise status output.
sast: _sast_shell _sast_semgrep _sast_clang_tidy _sast_secrets _sast_clang_tidy_report
	@echo "[sast] Blocking checks passed and extended reporting completed"

#R035: Shell lane checks shell sources with ShellCheck.
#R047: SAST lane prints explanatory header before tool execution.
_sast_shell:
	@$(MAKE) _sast_tool_header TOOL_NAME="ShellCheck" TOOL_DESC="Analyzes shell scripts for correctness and security issues." TOOL_URL="https://www.shellcheck.net/"
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

#R045: Semgrep lane runs semgrep auto configuration.
#R046: Semgrep lane preserves findings visibility by avoiding quiet mode.
#R047: SAST lane prints explanatory header before tool execution.
_sast_semgrep:
	@$(MAKE) _sast_tool_header TOOL_NAME="Semgrep" TOOL_DESC="Static pattern-based scanning for security and correctness issues." TOOL_URL="https://semgrep.dev/docs/"
	@echo "[sast:semgrep] Running semgrep auto rules"
	@if ! command -v "$(SEMGREP_CMD)" >/dev/null; then \
	  echo "[sast:semgrep] semgrep missing; install and retry"; \
	  exit 1; \
	fi
	"$(SEMGREP_CMD)" --config auto --error  .

#R050: Blocking clang-tidy lane scans repository C++ sources.
#R051: Third-party filtering is limited to portability/performance/bugprone warning families.
#R052: Broad suppression patterns are prohibited in clang-tidy output handling.
#R053: Blocking clang-tidy lane prints explicit first-party warning/suppression summary counts.
#R054: Blocking clang-tidy lane rejects first-party NOLINT suppression usage.
#R065: Blocking lane remains separate from report-only behavior.
#R047: SAST lane prints explanatory header before tool execution.
_sast_clang_tidy:
	@$(MAKE) _sast_tool_header TOOL_NAME="clang-tidy" TOOL_DESC="Performs static analysis and linting for C++ sources." TOOL_URL="https://clang.llvm.org/extra/clang-tidy/"
	@echo "[sast:clang-tidy] Running blocking clang-tidy checks"
	@if ! command -v "$(CLANG_TIDY_CMD)" >/dev/null; then \
	  echo "[sast:clang-tidy] clang-tidy missing; install and retry"; \
	  exit 1; \
	fi
	@if [ -z "$(CPP_SOURCES)" ]; then \
	  echo "[sast:clang-tidy] No C++ sources discovered"; \
	else \
	  $(MAKE) _sast_prepare_compile_db; \
	  first_party_nolint_count=$$(rg --glob '*.cpp' --glob '*.h' --glob '*.hpp' --glob '*.cxx' --glob '*.cc' --glob '*.c' "NOLINT" src include --count | awk -F: '{sum += $$NF} END {print sum + 0}'); \
	  "$(CLANG_TIDY_CMD)" --checks="$(CLANG_TIDY_CHECKS)" $(CPP_SOURCES) -- -Iinclude -Isrc -I"$(BUILD_DIR)/_deps/nlohmann_json-src/include" | \
	    awk -v first_party_path="$(CLANG_TIDY_FIRST_PARTY_PATH_REGEX)" \
	        -v third_party_path="$(CLANG_TIDY_THIRD_PARTY_PATH_REGEX)" \
	        -v first_party_nolint_count="$$first_party_nolint_count" \
	        'BEGIN { first_party_warning_count = 0 } \
	         { is_warning = ($$0 ~ / warning: /); \
	           is_third_party = ($$0 ~ third_party_path); \
	           is_first_party = ($$0 ~ first_party_path); \
	           is_suppressed_third_party = (is_warning && is_third_party && $$0 ~ /\[(portability|performance|bugprone)-/); \
	           if (!is_suppressed_third_party) print $$0; \
	           if (is_warning && is_first_party) first_party_warning_count += 1; } \
	         END { print "[sast:clang-tidy] first-party warning count: " first_party_warning_count; \
	               print "[sast:clang-tidy] first-party NOLINT count: " first_party_nolint_count; \
	               exit (first_party_warning_count > 0 || first_party_nolint_count > 0) ? 2 : 0; }'; \
	fi

#R060: Report clang-tidy lane is non-blocking.
#R051: Report lane preserves third-party diagnostics outside allowed suppressed families.
#R065: Report lane remains separated from blocking checks.
_sast_clang_tidy_report:
	@echo "[sast:clang-tidy-report] Running non-blocking clang-tidy checks"
	@if ! command -v "$(CLANG_TIDY_CMD)" >/dev/null; then \
	  echo "[sast:clang-tidy-report] clang-tidy missing; skipping report"; \
	else \
	  if [ -z "$(CPP_SOURCES)" ]; then \
	    echo "[sast:clang-tidy-report] No C++ sources discovered"; \
	  else \
	    $(MAKE) _sast_prepare_compile_db; \
	    "$(CLANG_TIDY_CMD)" --checks="$(CLANG_TIDY_CHECKS)" $(CPP_SOURCES) -- -Iinclude -Isrc -I"$(BUILD_DIR)/_deps/nlohmann_json-src/include" | \
	      awk -v third_party_path="$(CLANG_TIDY_THIRD_PARTY_PATH_REGEX)" \
	          '{ is_warning = ($$0 ~ / warning: /); \
	             is_third_party = ($$0 ~ third_party_path); \
	             is_suppressed_third_party = (is_warning && is_third_party && $$0 ~ /\[(portability|performance|bugprone)-/); \
	             if (!is_suppressed_third_party) print $$0; }' || \
	      echo "[sast:clang-tidy-report] diagnostics reported"; \
	  fi; \
	fi

_sast_prepare_compile_db:
	@echo "[sast:clang-tidy] Preparing CMake compile database in $(BUILD_DIR)"
	cmake -S . -B "$(BUILD_DIR)" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DFOUNTAIN_BUILD_TESTS=OFF -DFOUNTAIN_BUILD_EXAMPLES=OFF

#R055: Secrets lane runs gitleaks detection.
#R047: SAST lane prints explanatory header before tool execution.
_sast_secrets:
	@$(MAKE) _sast_tool_header TOOL_NAME="gitleaks" TOOL_DESC="Detects hardcoded secrets and sensitive tokens in repositories." TOOL_URL="https://github.com/gitleaks/gitleaks"
	@echo "[sast:secrets] Running gitleaks detect"
	@if ! command -v "$(GITLEAKS_CMD)" >/dev/null; then \
	  echo "[sast:secrets] gitleaks missing; install and retry"; \
	  exit 1; \
	fi
	"$(GITLEAKS_CMD)" detect --source . --no-banner --redact --exit-code 1

_sast_tool_header:
	@printf '%s\n' "+==============================================================================+"
	@printf '%s\n' "| SAST Tool: $(TOOL_NAME)"
	@printf '%s\n' "| $(TOOL_DESC)"
	@printf '%s\n' "| URL: $(TOOL_URL)"
	@printf '%s\n' "+==============================================================================+"

#R080: Clean lane moves generated artifacts to timestamped Trash paths.
#R040: Clean lane emits concise status output.
clean:
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
