ifeq ($(FRC_BUILD_VERSION), )
FRC_BUILD_VERSION = $(shell $(FRCDIR)/build_version.sh debug)
endif

BUILD_VERSION = $(FRC_BUILD_VERSION)
MAJOR_VERSION = 1
MINOR_VERSION = 1



VTARGET = $(TARGET)_version
VCLEAN  = $(VTARGET)_clean
VOBJ    = $(VTARGET).o
VSRC    = $(VTARGET).c

$(VOBJ): $(VSRC)
	$(CC) $(CFLAGS) -c $< -o $@

$(VSRC):
	@echo  "==== Auto generate version info, BUILD_VERSION $(BUILD_VERSION) ===="
	@echo "/* FRC version auto generate */" > $(VSRC)
	@echo "typedef struct {" >> $(VSRC)
	@echo "    unsigned char  major;" >> $(VSRC)
	@echo "    unsigned char  minor;" >> $(VSRC)
	@echo "    unsigned short build;" >> $(VSRC)
	@echo "} $(TARGET)_version_t;" >> $(VSRC)
	@echo "" >> $(VSRC)
	@echo "void" >> $(VSRC)
	@echo "$(VTARGET)_get(void *ptr)" >> $(VSRC)
	@echo "{" >> $(VSRC)
	@echo "    $(TARGET)_version_t *version = ($(TARGET)_version_t *) ptr;" >> $(VSRC)
	@echo "    version->major = "$(MAJOR_VERSION)"; " >> $(VSRC)
	@echo "    version->minor = "$(MINOR_VERSION)"; " >> $(VSRC)
	@echo "    version->build = "$(BUILD_VERSION)"; " >> $(VSRC)
	@echo "}" >> $(VSRC)
	@echo "" >> $(VSRC)
	@echo "/* End of file */" >> $(VSRC)

$(VCLEAN):
	rm -rf $(VSRC)
