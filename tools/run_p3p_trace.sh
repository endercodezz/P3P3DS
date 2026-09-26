#!/usr/bin/env bash
# =============================================================================
# tools/run_p3p_trace.sh — P3P3DS Execution Trace & Logging Runner
# =============================================================================
# Builds the project and runs p3p_pc_bootstrap.exe with --verbose, capturing
# full stdout/stderr to a timestamped log file and logs/p3p_bootstrap_latest.log
# while preserving the pipeline exit code.
# =============================================================================

set -e
set -o pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

cd "${WORKSPACE_ROOT}"

# Ensure logs directory exists inside workspace containment
mkdir -p logs

TIMESTAMP="$(date +'%Y%m%d_%H%M%S')"
TIMESTAMPED_LOG="logs/p3p_bootstrap_${TIMESTAMP}.log"
LATEST_LOG="logs/p3p_bootstrap_latest.log"

echo "===================================================="
echo "   P3P3DS Execution Trace Harness"
echo "   Timestamp: ${TIMESTAMP}"
echo "   Log:       ${TIMESTAMPED_LOG}"
echo "===================================================="

# Run build followed by bootstrap execution, streaming to terminal and logs
# tee receives combined stdout/stderr while set -o pipefail ensures failure
# of cmake or the bootstrap binary is propagated.
set +e
{
    echo "=== [1/2] Building P3P3DS ==="
    cmake --build build
    BUILD_STATUS=$?
    if [ ${BUILD_STATUS} -ne 0 ]; then
        echo "Build failed with status ${BUILD_STATUS}"
        exit ${BUILD_STATUS}
    fi

    echo ""
    echo "=== [2/2] Running P3P PC Bootstrap ==="
    ./build/p3p_pc_bootstrap.exe --verbose
    BOOTSTRAP_STATUS=$?
    echo ""
    echo "Bootstrap finished with exit code ${BOOTSTRAP_STATUS}"
    exit ${BOOTSTRAP_STATUS}
} 2>&1 | tee "${TIMESTAMPED_LOG}"
PIPELINE_STATUS="${PIPESTATUS[0]}"

# Maintain latest log pointer / copy
cp -f "${TIMESTAMPED_LOG}" "${LATEST_LOG}"

echo "===================================================="
echo "Trace finished (exit code: ${PIPELINE_STATUS})"
echo "Latest log: ${LATEST_LOG}"
echo "===================================================="

exit "${PIPELINE_STATUS}"
