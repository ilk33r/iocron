
iocron_executer: $(SAPI_IOCRON_EXECUTER_PATH)

$(SAPI_IOCRON_EXECUTER_PATH): $(PHP_GLOBAL_OBJS) $(PHP_BINARY_OBJS) $(PHP_CGI_OBJS)
	$(BUILD_IOCRON_EXECUTER)

install-iocron_executer: $(SAPI_IOCRON_EXECUTER_PATH)
	@echo "Installing iocron_executer binary:        $(INSTALL_ROOT)$(bindir)/"
	@$(mkinstalldirs) $(INSTALL_ROOT)$(bindir)
	@$(INSTALL) -m 0755 $(EXECUTER_PATH) $(INSTALL_ROOT)$(bindir)/$(program_prefix)iocron_executer$(program_suffix)$(EXEEXT)

install-iocron-service:
	@echo "Installing Serverapi Service:        $(SERVERAPI_SERVICE_PATH)/"
	@$(INSTALL) -m 0755 $(srcdir)/serverapi_service $(SERVERAPI_SERVICE_PATH)/serverapi_service

compile-iocron-executer:
 	@echo "Compiling executer:        $(prefix)/bin/iocron_executer"
	$(LIBTOOL) --mode=compile $(CC) -I. -I$(srcdir) $(COMMON_FLAGS) $(CFLAGS_CLEAN) $(EXTRA_CFLAGS)  -c $(srcdir)/iocron_executer.c -o iocron_executer.lo
	@echo "Installing executer:        $(prefix)/bin/iocron_executer"
	@$(INSTALL) -m 0755 $(srcdir)/iocron_executer.lo $(prefix)/bin/iocron_executer