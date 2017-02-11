#include <stdio.h>

#if defined(WINDOWS)
	#include <windows.h>
#elif defined(LINUX) || defined(APPLE)
	#include <dlfcn.h>
	#include <glob.h>
#endif

#include "plugin.hpp"


namespace rack {

std::list<Plugin*> gPlugins;

static
int loadPlugin(const char *path) {
	// Load dynamic/shared library
	#if defined(WINDOWS)
		HINSTANCE handle = LoadLibrary(path);
	if (!handle) {
		fprintf(stderr, "Failed to load library %s\n", path);
		return -1;
	}
	#elif defined(LINUX) || defined(APPLE)
		char ppath[1024];
		snprintf(ppath, sizeof(ppath), "./%s", path);
		void *handle = dlopen(ppath, RTLD_NOW | RTLD_GLOBAL);
	if (!handle) {
		fprintf(stderr, "Failed to load library %s: %s\n", path, dlerror());
		return -1;
	}
	#endif

	// Call plugin init() function
	typedef Plugin *(*InitCallback)();
	InitCallback initCallback;
	#if defined(WINDOWS)
		initCallback = (InitCallback) GetProcAddress(handle, "init");
	#elif defined(LINUX) || defined(APPLE)
		initCallback = (InitCallback) dlsym(handle, "init");
	#endif
	if (!initCallback) {
		fprintf(stderr, "Failed to read init() symbol in %s\n", path);
		return -2;
	}

	// Add plugin to map
	Plugin *plugin = initCallback();
	if (!plugin) {
		fprintf(stderr, "Library %s did not return a plugin\n", path);
		return -3;
	}
	gPlugins.push_back(plugin);
	fprintf(stderr, "Loaded plugin %s\n", path);
	return 0;
}

void pluginInit() {
	// Load core
	// This function is defined in core.cpp
	Plugin *corePlugin = init();
	gPlugins.push_back(corePlugin);

	// Search for plugin libraries
	#if defined(WINDOWS)
		WIN32_FIND_DATA ffd;
		HANDLE hFind = FindFirstFile("plugins/*", &ffd);
		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				char pluginFilename[MAX_PATH];
				snprintf(pluginFilename, sizeof(pluginFilename), "plugins/%s/plugin.dll", ffd.cFileName);
				loadPlugin(pluginFilename);
			} while (FindNextFile(hFind, &ffd));
		}
		FindClose(hFind);

	#elif defined(LINUX) || defined(APPLE)
		#if defined(LINUX)
			const char *globPath = "plugins/*/plugin.so";
		#elif defined(WINDOWS)
			const char *globPath = "plugins/*/plugin.dll";
		#elif defined(APPLE)
			const char *globPath = "plugins/*/plugin.dylib";
		#endif
		glob_t result;
		glob(globPath, GLOB_TILDE, NULL, &result);
		for (int i = 0; i < (int) result.gl_pathc; i++) {
			loadPlugin(result.gl_pathv[i]);
		}
		globfree(&result);
	#endif
}

void pluginDestroy() {
	for (Plugin *plugin : gPlugins) {
		// TODO unload plugin with `dlclose` or `FreeLibrary`
		delete plugin;
	}
	gPlugins.clear();
}


Plugin::~Plugin() {
	for (Model *model : models) {
		delete model;
	}
}


} // namespace rack