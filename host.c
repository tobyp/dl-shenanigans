#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <errno.h>
#include <link.h>

typedef void (*callback_t)();
typedef void (*module_init_t)(callback_t cb);
typedef void (*module_process_t)();

static void callback() {
	Dl_info info;
	if (dladdr(__builtin_return_address(0), &info) == 0) {  // which module was this callback called from?
		fprintf(stderr, "Failed to identify callback source\n");
		return;
	};

	// dli_fbase now uniquely identifies the dynamic module this function was called from

	printf("Callback called from %s(%p) of module %s(%p)\n", info.dli_sname, info.dli_saddr, info.dli_fname, info.dli_fbase);
}

struct module {
	char const* name;
	void * handle;  // must b first 
	void * base;
	void (*module_init)(callback_t);
	void (*module_process)();
};

int module_load(struct module * module, char const* name) {
	module->name = name;
	module->handle = dlmopen(LM_ID_NEWLM, module->name, RTLD_NOW|RTLD_LOCAL);

	// LM_ID_NEWLM only works if the SO was correctly linked to all its dependencies (including e.g. libc/stdlib/etc.)
	// If it wasn't, start by dlmopening the first (leaf!) dependency (probably libc) with LM_ID_NEWLM, then get that LMID using dlinfo(RTLD_DI_LMID), then dlmopen further dependencies bottom-um into that LM.
	// One strategy might be to examine the main processes LM (dlinfo RTLD_DI_LINKMAP on module handle NULL) and re-creating that, though some listed modules (like vdso or ld-linux) may not be possible/necessary becasue they are injected by kernel/linker.
	// @J.H., have fun replicating this on Windows... MWA HA HA HA HAA!

	if (!module->handle) {
		fprintf(stderr, "dlopen(\"%s\"): %s\n", module->name, dlerror());
		return 1;
	}

	module->module_init = dlsym(module->handle, "module_init");
	if (!module->module_init) {
		fprintf(stderr, "dlsym(\"module_init\"): %s\n", dlerror());
		return 1;
	}
	module->module_process = dlsym(module->handle, "module_process");
	if (!module->module_process) {
		fprintf(stderr, "dlsym(\"module_process\"): %s\n", dlerror());
		return 1;
	}

	Dl_info module_info;
	if (!dladdr(module->module_init, &module_info)) {
		fprintf(stderr, "Failed to determine base address\n");
		return 1;
	}
	module->base = module_info.dli_fbase;

	return 0;
}

int main() {
	struct module module1;
	if (module_load(&module1, "module.so")) {
		fprintf(stderr, "Failed to load module.\n");
		return 1;
	}
	printf("Loaded module at base address %p\n", module1.base);

	struct module module2;
	if (module_load(&module2, "module.so")) {
		fprintf(stderr, "Failed to load module.\n");
		return 1;
	}
	printf("Loaded module at base address %p\n", module2.base);

	printf("Registering callbacks\n");
	module1.module_init(&callback);
	module2.module_init(&callback);

	printf("Triggering callbacks\n");
	module1.module_process();
	module2.module_process();
}
