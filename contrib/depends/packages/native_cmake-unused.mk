package=native_cmake
$(package)_version=3.17.3
$(package)_version_dot=v3.17.3
$(package)_download_path=https://cmake.org/files/$($(package)_version_dot)/
$(package)_file_name=cmake-$($(package)_version).tar.gz
$(package)_sha256_hash=0bd60d512275dc9f6ef2a2865426a184642ceb3761794e6b65bff233b91d8c40

define $(package)_set_vars
$(package)_config_opts=
endef

define $(package)_config_cmds
  ./bootstrap &&\
  ./configure $($(package)_config_opts)
endef

define $(package)_build_cmd
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef
