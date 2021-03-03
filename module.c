typedef void (*callback_t)();

static callback_t callback;

extern void module_init(callback_t cb) {
	callback = cb;
}

extern void module_process() {
	if (callback) callback();
}
