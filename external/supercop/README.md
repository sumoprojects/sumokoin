# Supercop

> This project could use a rename :/

The objective of this project is to provide fast cryptographic operations for
Monero wallets. Unfortunately, there isn't necessarily a single fastest library
for all platforms - donna64 for instance likely performs poorly when
cross-compiled to asm.js or 32-bit arm. This project mitigates this issue by
copying the cryptographic libraries with as little changes as possible (ideally
zero), and then provides smaller and easier to audit "glue" functions that use
the cryptographic library to perform typical Monero wallet tasks. These "glue"
functions are often not obvious to the typical developer (otherwise they would
be using the cryptographic libraries already), however auditing these functions
should be straightforward to anyone familar with how the cryptography works.

The project is also designed to be used in-tree in other projects or installable
on a system. The default Monero wallets use the in-tree implementation which
serve as a more complex example of its usage. The project directory structure:

  - **crypto_sign** - whose name is taken from supercop. The code in here is
    from upstream cryptographic libraries with minimal changes (position
    independent ASM).
  - **include** - The only files that should be included by external projects,
    with the exception of the auto-generated header.
  - **src** - Contains all of the glue functions and build code.
  - **functions.cmake** - The raw components for building the library. Used by
    the default Monero wallet.
  - **intree.cmake** - Creates a cmake library target `monero-crypto-intree`
    and generates a header at `<BUILD_LOCATION>/include/monero/crypto.h`.
  - **CMakeLists.txt** - Declares a new project for creating/installing a
    shared or static `monero-crypto` library.

> Downstream projects cannot currently use this project _without_ also using
> the `monero-project/monero/src/crypto/crypto.h` functions - specifically
> `crypto::derivation_to_scalar`. So currently this project provides
> performance enhancements but not standalone usage.

## Usage

Every cryptographic library implements the same function signatures but with
unique namespaces to avoid collisions in the C symbol table. Downstream
projects can either use a specific library and header file OR dynamically
select a library and use a cmake generated header that uses macro replacement
to reference the selected library. Since each library has unique symbol names,
multiple can be used simulatenously (benchmarking, etc).

## C Wallet Functions

Every cryptographic library implements:

  - **`monero_crypto_<namespace>_scalarmult(char* out, const char* pub, const char* sec)`** -
    where `out` is the shared secret, `pub` is an ed25519 public key, and `sec`
    is a user secret key. This operation is a standard ECDH operation on the
    ed25519 curve.
  - **`monero_crypto_<namespace>_generate_key_derivation(char* out, const char* tx_pub, const char* view_sec)`** -
    where `out` is the shared secret, `tx_pub` is the public key from the
    transaction and `view_sec` is the users view-key secret.
  - **`monero_crypto_<namespace>_generate_subaddress_public_key(char* out, const char* output_pub, const char* special_sec)`** -
    where `out` is the computed spend-public (primary or subaddress),
    `output_pub` is the public key from the transaction output, and
    `special_sec` is the output from `crypto::derivation_to_scalar` (which is
    given the shared secret from `generate_key_derivation`).

All `char` pointers must be exactly 32-bytes. The auto-generated header has an
empty namespace for these functions and uses macro replacement to forward to the
selected library. If using dynamically selected libraries, include the
auto-generated header and use `monero_crypto_generate_key_derivation`, etc.

## CMake Usage
### Explicit Library Usage
A library can be selected explicitly in-tree:

```
include(supercop/functions.cmake)
monero_crypto_get_target("amd64-64-24k" CRYPTO_TARGET)

add_library(all $<OBJECT_TARGETS:${CRYPTO_TARGET}> all.cpp)
add_library(referenced referenced.cpp)
target_link_libraries(referenced ${CRYPTO_TARGET})
```

The `all` library will have all C functions in the `amd64-64-24k` library
whereas the `referenced` library will only have C functions referenced in
`referenced.cpp`.

## Basic In-Tree Dynamic Usage
A library can be selected at build time in-tree:

```
include(supercop/intree.cmake)

add_library(all $<OBJECT_TARGETS:monero-crypto-intree> all.cpp)
add_library(referenced referenced.cpp)
target_link_libraries(referenced monero-crypto-intree)
```

The best available library for the target platform is selected which can be
overriden with `-DMONERO_CRYPTO_LIBRARY=amd64-51-30k` or by setting the
variable with the same name.

A header file is written to `<BUILD_LOCATION>/include/monero/crypto.h` which
contains empty namespace macros that expand to the
[wallet functions](#c-wallet-functions) for the selected library. Including
this header file and using the empty namespace macros will allow for the
crypto library to be selected dynamically.

See the [explicit usage section](#explicit-library-usage) for the difference
between the `all` and `referenced` libraries.

## Advanced In-Tree Dynamic Usage
See `intree.cmake` on how to use the individual cmake functions in your project
for more control. Each of the cmake functions is documented.

## Standalone Usage
Is not-yet fully functional since the `crypto::derivation_to_scalar` from the
Monero source tree is still needed. However this is a proof-of-concept that
will be useful for testing if your project does not use cmake.

Running `cmake <source_path> . && make && sudo make install` will autodetect
the best crypto library, build it in the current directory, and then install to
the default location for your system. An error is generated if your target
platform does not have a supported crypto library. The auto-selected library
can be overriden with `-DMONERO_CRYPTO_LIBRARY=amd64-51-30k` at the first
cmake step. Using `-DMONERO_CRYPTO_LIBRARY=invalid` will generate an error
that displays all available crypto libraries.
