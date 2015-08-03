#
#   Date: Jun 22, 2012
# Author: Eldar Abusalimov
#

ifeq ($(STAGE),1)
embox_o   := $(OBJ_DIR)/embox.o
else
embox_o   := $(OBJ_DIR)/embox-2.o
$(embox_o) : $(OBJ_DIR)/embox.o
endif

image_lds := $(OBJ_DIR)/mk/image.lds

.PHONY : all FORCE
all : $(embox_o) $(image_lds)

FORCE :

include mk/image_lib.mk

include $(MKGEN_DIR)/build.mk

include mk/flags.mk # It must be included after a user-defined config.

.SECONDEXPANSION:

include $(MKGEN_DIR)/include.mk
include $(__include_image)
include $(__include_initfs)
include $(__include)

initfs_cp_prerequisites = $(src_file) $(common_prereqs)

cp_T_if_supported := \
	$(shell $(CP) --version 2>&1 | grep -l GNU >/dev/null && echo -T)

# This rule is necessary because otherwise, when considering a rule for
# creating the $(ROOTFS_DIR) directory (required through $(common_prereqs)
# using the secondarily-expanded order-only '| $(@D)/.' prerequisite),
# the '$(ROOTFS_DIR)/%' rule takes precedence over the proper '%/.' one, since
# the former needs a shorter stem ('.') than the latter ('build/.../rootfs').
# This is reproduced only on GNU Make >= 3.82, because of the change in how
# implicit rule search works in Make.
$(ROOTFS_DIR)/. :
	@mkdir -p $@
$(ROOTFS_DIR)/%/. :
	@mkdir -p $@

$(ROOTFS_DIR)/% :
	$(CP) -r $(cp_T_if_supported) $(src_file) $@$(if \
		$(and $(chmod),$(findstring $(chmod),'')),,;chmod $(chmod) $@)
	@touch $@ # workaround when copying directories
	@find $@ -name .gitkeep -type f -print0 | xargs -0 /bin/rm -rf

fmt_line = $(addprefix \$(\n)$(\t)$(\t),$1)

initfs_prerequisites = $(cpio_files) \
	$(wildcard $(USER_ROOTFS_DIR) $(USER_ROOTFS_DIR)/*) $(common_prereqs)
$(ROOTFS_IMAGE) : rel_cpio_files = \
		$(patsubst $(abspath $(ROOTFS_DIR))/%,%,$(abspath $(cpio_files)))
$(ROOTFS_IMAGE) :
	@mkdir -p $(ROOTFS_DIR)
	cd $(ROOTFS_DIR) \
		&& find $(rel_cpio_files) -depth -print | $(CPIO) -L --quiet -H newc -o -O $(abspath $@)
	if [ -d $(USER_ROOTFS_DIR) ]; \
	then \
		cd $(USER_ROOTFS_DIR) \
			&& find * -depth -print | $(CPIO) -L --quiet -H newc -o -A -O $(abspath $@); \
	fi
	@FILES=`find $(cpio_files) $(USER_ROOTFS_DIR)/* -depth -print 2>/dev/null`; \
	{                                            \
		printf '$(ROOTFS_IMAGE):';               \
		for dep in $$FILES;                      \
			do printf ' \\\n\t%s' "$$dep"; done; \
		printf '\n';                             \
		for dep in $$FILES;                      \
			do printf '\n%s:\n' "$$dep"; done;   \
	} > $@.d
-include $(ROOTFS_IMAGE).d

#XXX
$(OBJ_DIR)/src/fs/driver/initfs/initfs_cpio.o : $(ROOTFS_IMAGE)

ifdef __REBUILD_ROOTFS
initfs_cp_prerequisites += FORCE
initfs_prerequisites    += FORCE
endif

# Module-level rules.
module_prereqs = $(o_files) $(a_files) $(common_prereqs)

$(OBJ_DIR)/module/% : objcopy_flags = \
	$(foreach s,text rodata data bss,--rename-section .$s=.$s.module.$(module_id))

ar_prerequisites = $(module_prereqs)
$(OBJ_DIR)/module/%.a : mk/arhelper.mk
	@$(MAKE) -f mk/arhelper.mk TARGET='$@' \
		AR='$(AR)' ARFLAGS='$(ARFLAGS)' \
		A_FILES='$(a_files)' \
		O_FILES='$(o_files)' \
		APP_ID='$(is_app)' $(if $(is_app), \
			OBJCOPY='$(OBJCOPY)' OBJCOPYFLAGS='$(objcopy_flags)')

ld_prerequisites = $(module_prereqs)
obj_build=$(if $(strip $(value mod_postbuild)),$@.build.o,$@)
obj_postbuild=$@
$(OBJ_DIR)/module/%.o :
	if [ $$(echo $(o_files) $(a_files) | wc -w) -gt 1 ]; then \
		$(LD) -r -o $(obj_build) $(ldflags) $(call fmt_line,$(o_files) \
		    $(if $(a_files),--whole-archive $(a_files) --no-whole-archive)); \
	else \
		cp $(o_files) $(a_files) $(obj_build); \
	fi
	#$(if $(module_id),$(OBJCOPY) $(objcopy_flags) $(obj_build))
	$(mod_postbuild)


# Here goes image creation rules...
#
# workaround to get VPATH and GPATH to work with an OBJ_DIR.
$(shell $(MKDIR) $(OBJ_DIR) 2> /dev/null)
GPATH := $(OBJ_DIR:$(ROOT_DIR)/%=%)
VPATH += $(GPATH)

link_lib = -L$(dir $1) -l$(patsubst lib%,%,$(basename $(notdir $1)))
LARGE_LIB = $(dir $(embox_o))/lib$(basename $(notdir $(embox_o))).large.a

$(embox_o): ldflags_all = $(call fmt_line,$(call ld_scripts_flag,$(ld_scripts)))
$(embox_o): $(LARGE_LIB)
	$(LD) $(ldflags_all) -T $(SRC_DIR)/arch/multiclet/multiclet.lds \
		$(call fmt_line,$(ld_objs)) \
		$(call link_lib,$(LARGE_LIB)) \
	-o $@ -M

stages := $(wordlist 1,$(STAGE),1 2)

image_prereqs = $(ld_scripts) $(ld_objs) $(ld_libs) $(common_prereqs)

$(embox_o) : $$(image_prereqs)
$(embox_o) : mk_file = $(__image_mk_file)
$(embox_o) : ld_scripts = $(__image_ld_scripts1) # TODO check this twice
$(embox_o) : ld_objs = $(foreach s,$(stages),$(__image_ld_objs$s))
ld_libs = $(foreach s,$(stages),$(__image_ld_libs$s))

$(LARGE_LIB) : $(ld_libs)
	$(foreach i,$(ld_libs),echo ***$i;ar t $i;)
	@$(MAKE) -f mk/arhelper.mk TARGET='$@' \
		AR='$(AR)' ARFLAGS='$(ARFLAGS)' \
		A_FILES='$(ld_libs)' \
		O_FILES='' \
		APP_ID=''


$(image_lds) : $$(common_prereqs)
$(image_lds) : flags_before :=
$(image_lds) : flags = \
		$(addprefix -include ,$(wildcard \
			$(SRC_DIR)/arch/$(ARCH)/embox.lds.S \
			$(if $(value PLATFORM), \
				$(PLATFORM_DIR)/$(PLATFORM)/arch/$(ARCH)/platform.lds.S)))
-include $(image_lds).d
