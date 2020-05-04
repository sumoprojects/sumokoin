package=native_cdrkit
$(package)_version=1.1.11
$(package)_download_path=https://src.fedoraproject.org/repo/pkgs/cdrkit/cdrkit-$($(package)_version).tar.gz/efe08e2f3ca478486037b053acd512e9/
$(package)_file_name=cdrkit-$($(package)_version).tar.gz
$(package)_sha256_hash=d1c030756ecc182defee9fe885638c1785d35a2c2a297b4604c0e0dcc78e47da
$(package)_patches=cdrkit-deterministic.patch

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/cdrkit-deterministic.patch
endef

define $(package)_config_cmds
  cmake -DCMAKE_INSTALL_PREFIX=$(build_prefix)
endef

define $(package)_build_cmds
  $(MAKE) genisoimage
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) -C genisoimage install
endef

define $(package)_postprocess_cmds
  rm bin/isovfy bin/isoinfo bin/isodump bin/isodebug bin/devdump
endef
