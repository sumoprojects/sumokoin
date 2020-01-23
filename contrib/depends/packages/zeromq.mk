package=zeromq
$(package)_version=4.2.2
$(package)_download_path=https://github.com/zeromq/libzmq/releases/download/v$($(package)_version)/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=5b23f4ca9ef545d5bd3af55d305765e3ee06b986263b31967435d285a3e6df6b
$(package)_patches=0001-fix-build-with-older-mingw64.patch ffe62d3398d5e0191f554f61049aa7ec9fc892ae.patch

define $(package)_set_vars
  $(package)_config_opts=--without-docs --disable-shared --without-libsodium --disable-curve --disable-curve-keygen --disable-perf
  $(package)_config_opts_linux=--with-pic
  $(package)_config_opts_freebsd=--with-pic
  $(package)_cxxflags=-std=c++11
endef

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/0001-fix-build-with-older-mingw64.patch && \
  patch -p1 < $($(package)_patch_dir)/ffe62d3398d5e0191f554f61049aa7ec9fc892ae.patch && \
  ./autogen.sh
endef

define $(package)_config_cmds
  $($(package)_autoconf) AR_FLAGS=$($(package)_arflags)
endef

define $(package)_build_cmds
  $(MAKE) src/libzmq.la
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install-libLTLIBRARIES install-includeHEADERS install-pkgconfigDATA
endef

define $(package)_postprocess_cmds
  rm -rf bin share &&\
  rm lib/*.la
endef

