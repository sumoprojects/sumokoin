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

function(add_amd64_library LIBNAME LIBFOLDER)
  set(MULTIVARS SOURCES)
  cmake_parse_arguments(AMD64_PERF "" "" "${MULTIVARS}" ${ARGN})

  add_library("monero-crypto-${LIBNAME}" OBJECT
    "${LIBNAME}.c" "${LIBNAME}-choose_tp.s"
    "${LIBFOLDER}/choose_t.s"
    "${LIBFOLDER}/consts.s"
    "${LIBFOLDER}/fe25519_getparity.c"
    "${LIBFOLDER}/fe25519_freeze.s"
    "${LIBFOLDER}/fe25519_invert.c"
    "${LIBFOLDER}/fe25519_iseq.c"
    "${LIBFOLDER}/fe25519_mul.s"
    "${LIBFOLDER}/fe25519_neg.c"
    "${LIBFOLDER}/fe25519_pack.c"
    "${LIBFOLDER}/fe25519_pow2523.c"
    "${LIBFOLDER}/fe25519_setint.c"
    "${LIBFOLDER}/fe25519_square.s"
    "${LIBFOLDER}/fe25519_unpack.c"
    "${LIBFOLDER}/ge25519_add.c"
    "${LIBFOLDER}/ge25519_add_p1p1.s"
    "${LIBFOLDER}/ge25519_dbl_p1p1.s"
    "${LIBFOLDER}/ge25519_double.c"
    "${LIBFOLDER}/ge25519_nielsadd_p1p1.s"
    "${LIBFOLDER}/ge25519_nielsadd2.s"
    "${LIBFOLDER}/ge25519_pack.c"
    "${LIBFOLDER}/ge25519_p1p1_to_p2.s"
    "${LIBFOLDER}/ge25519_p1p1_to_p3.s"
    "${LIBFOLDER}/ge25519_pnielsadd_p1p1.s"
    "${LIBFOLDER}/ge25519_scalarmult_base.c"
    "${LIBFOLDER}/ge25519_unpackneg.c"
    "${LIBFOLDER}/sc25519_from32bytes.c"
    "${LIBFOLDER}/sc25519_window4.c"
    ${AMD64_PERF_SOURCES})
  target_include_directories("monero-crypto-${LIBNAME}" PRIVATE ${LIBFOLDER})
  set_source_files_properties("${LIBFOLDER}/consts.s" PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp" LANGUAGE "C")
endfunction (add_amd64_library)

add_amd64_library(
  "amd64-64-24k"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../crypto_sign/ed25519/amd64-64-24k"
  SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/../../crypto_sign/ed25519/amd64-64-24k/fe25519_add.s"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../crypto_sign/ed25519/amd64-64-24k/fe25519_sub.s")

add_amd64_library(
  "amd64-51-30k"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../crypto_sign/ed25519/amd64-51-30k"
  SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/../../crypto_sign/ed25519/amd64-51-30k/fe25519_add.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../crypto_sign/ed25519/amd64-51-30k/fe25519_nsquare.s"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../crypto_sign/ed25519/amd64-51-30k/fe25519_sub.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../crypto_sign/ed25519/amd64-51-30k/ge25519_p1p1_to_pniels.s")
