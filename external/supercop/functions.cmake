# Copyright (c) 2020, The Monero Project
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification, are
# permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this list of
#    conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice, this list
#    of conditions and the following disclaimer in the documentation and/or other
#    materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors may be
#    used to endorse or promote products derived from this software without specific
#    prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
# THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
# THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

set(MONERO_CRYPTO_DIR ${CMAKE_CURRENT_LIST_DIR})
add_subdirectory("${MONERO_CRYPTO_DIR}/src" ${CMAKE_CURRENT_BINARY_DIR}/monero_crypto_src)

# Set `OUT` to a list of all valid library names.
function(monero_crypto_libraries OUT)
  set(${OUT} "amd64-64-24k" "amd64-51-30k" PARENT_SCOPE)
endfunction (monero_crypto_libraries)

# `OUT` is set to 1 iff `CRYPTO_LIBRARY` is a valid library name.
function(monero_crypto_valid CRYPTO_LIBRARY OUT)
  monero_crypto_libraries(ALL_LIBRARIES)
  list(FIND ALL_LIBRARIES ${CRYPTO_LIBRARY} FOUND)
  if (FOUND LESS 0)
    set(${OUT} 0 PARENT_SCOPE)
  else ()
    set(${OUT} 1 PARENT_SCOPE)
  endif ()
endfunction (monero_crypto_valid)

# Set `OUT` to a valid C/C++ namespace name based on `CRYPTO_LIBRARY`.
function(monero_crypto_get_namespace CRYPTO_LIBRARY OUT)
  string(REPLACE "-" "_" TEMP "${CRYPTO_LIBRARY}")
  set(${OUT} ${TEMP} PARENT_SCOPE)
endfunction (monero_crypto_get_namespace)

# Set `OUT` to a valid target dependency name based on `CRYPTO_LIBRARY`.
function(monero_crypto_get_target CRYPTO_LIBRARY OUT)
  set(${OUT} "monero-crypto-${CRYPTO_LIBRARY}" PARENT_SCOPE)
endfunction (monero_crypto_get_target)

# Sets `BEST` to the preferred crypto library for the target platform, and
# sets `AVAILABLE` to all valid crypto libraries for the target platform.
function(monero_crypto_autodetect AVAILABLE BEST)
  include(CheckLanguage)
  check_language(ASM-ATT)
  if (CMAKE_ASM-ATT_COMPILER AND UNIX AND (CMAKE_SYSTEM_PROCESSOR MATCHES "amd64.*|AMD64.*|x86_64.*"))
    set(${AVAILABLE} "amd64-64-24k" "amd64-51-30k" PARENT_SCOPE)
    set(${BEST} "amd64-64-24k" PARENT_SCOPE)
  else ()
    message("Monero crypto autodetect failed to find any libraries for target platform")
    unset(${AVAILABLE} PARENT_SCOPE)
    unset(${BEST} PARENT_SCOPE)
  endif ()
endfunction (monero_crypto_autodetect)

# Writes a C header to `HEADER_FILE` that contains these C functions:
#   - `monero_crypto_generate_key_derivation`
#   - `monero_crypto_derive_subaddress_public_key`
# that map to the selected `CRYPTO_LIBRARY` implementation using macro
# replacement. See README.md for function explanation.
#
# A fatal error is generated if `CRYPTO_LIBRARY` is not valid - linking will
# always fails in this situation.
function(monero_crypto_generate_header MONERO_CRYPTO_LIBRARY HEADER_FILE)
  monero_crypto_valid(${MONERO_CRYPTO_LIBRARY} VALID)
  if (NOT VALID)
    monero_crypto_libraries(ALL_LIBRARIES)
    string(REPLACE ";" " " ALL_LIBRARIES "${ALL_LIBRARIES}")
    message(FATAL_ERROR "Library ${MONERO_CRYPTO_LIBRARY} is not valid. Must be one of: ${ALL_LIBRARIES}")
  endif ()
  monero_crypto_get_namespace(${MONERO_CRYPTO_LIBRARY} MONERO_CRYPTO_NAMESPACE)
  configure_file("${MONERO_CRYPTO_DIR}/src/crypto.h.in" ${HEADER_FILE})
endfunction (monero_crypto_generate_header)

