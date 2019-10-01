CONTRACT_EXPORTS=['_dispatch']

LIBRARY_SOURCES = ${PDO_SOURCE_ROOT}/common/packages/parson/parson.cpp
COMMON_PACKAGE_SOURCES = $(wildcard common/*.cpp)
MOCK_CONTRACT_SOURCES = $(wildcard mock-contract/*.cpp)

PACKAGE_INCLUDES = -I${PDO_SOURCE_ROOT}/contracts/wawaka/common
PACKAGE_INCLUDES += -I${PDO_SOURCE_ROOT}/common/interpreter/wawaka_wasm
PACKAGE_INCLUDES += -I${PDO_SOURCE_ROOT}/common/packages/parson
PACKAGE_INCLUDES += -I${PDO_SOURCE_ROOT}/common

mock-contract.b64 : mock-contract.wasm
	rm -f mock-contract.b64
	base64 -w 0 mock-contract.wasm > mock-contract.b64

encoded_contract.h : contract.wasm contract.cpp
	xxd -i contract.wasm encoded_contract.h

mock-contract.wasm : ${MOCK_CONTRACT_SOURCES} ${COMMON_PACKAGE_SOURCES}
	emcc -g -O3 ${LIBRARY_SOURCES} ${COMMON_PACKAGE_SOURCES} ${MOCK_CONTRACT_SOURCES} \
		-o mock-contract.wasm \
		${PACKAGE_INCLUDES} \
		-I${WASM_SRC}/core/iwasm/lib/native-interface \
		-I${WASM_SRC}/core/iwasm/lib/app-libs/base \
		-s WASM=1 -s SIDE_MODULE=1 -s ASSERTIONS=1 -s STACK_OVERFLOW_CHECK=2 \
		-s TOTAL_MEMORY=65536 -s TOTAL_STACK=4096 \
		-s "EXPORTED_FUNCTIONS=${CONTRACT_EXPORTS}"

clean:
	rm -f mock-contract.b64 mock-contract.wasm mock-contract.wasm.map mock-contract.wast
