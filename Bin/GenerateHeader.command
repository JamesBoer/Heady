#!/bin/zsh

# Set to current working dir
DIR=$( cd "$( dirname "${(%):-%x}" )" && pwd )
cd "${DIR}"

HEADY_EXE=../Build/Release/Heady
if ! [ -x "${HEADY_EXE}" ]; then
    HEADY_EXE=../Build/Debug/Heady
fi

if ! [ -x "${HEADY_EXE}" ]; then
    echo "Error: Heady executable not found in Build/Release or Build/Debug."
    echo "Please build the Heady project first (see BuildXcode.command)."
    exit 1
fi

# Generate unified header file from all library source
"${HEADY_EXE}" --define HEADY_HEADER_ONLY --source "../Source" --output "../Include/Heady.hpp" --excluded "clara.hpp Main.cpp"
HEADY_RESULT=$?

if [ ${HEADY_RESULT} -ne 0 ]; then
    echo "Heady failed with error code ${HEADY_RESULT}"
    exit ${HEADY_RESULT}
fi

echo "Heady.hpp generated successfully."
exit 0
