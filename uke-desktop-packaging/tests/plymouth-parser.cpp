// SPDX-License-Identifier: MIT
// Host-only checks against the distribution's parser and text renderer; no display access.
#include <cassert>
#include <dlfcn.h>
#include <iostream>

int main(int argc, char **argv) {
    assert(argc == 3);
    void *plugin = dlopen(argv[2], RTLD_NOW | RTLD_LOCAL);
    if (!plugin) { std::cerr << dlerror() << '\n'; return 1; }
    auto parse = reinterpret_cast<void *(*)(const char *)>(dlsym(plugin, "script_parse_file"));
    auto parse_string = reinterpret_cast<void *(*)(const char *, const char *)>(dlsym(plugin, "script_parse_string"));
    auto release = reinterpret_cast<void (*)(void *)>(dlsym(plugin, "script_parse_op_free"));
    assert(parse && parse_string && release);
    assert(parse_string("fun {", "invalid-fixture") == nullptr);
    void *script = parse(argv[1]);
    assert(script);
    release(script);
    auto label_new = reinterpret_cast<void *(*)()>(dlsym(plugin, "ply_label_new"));
    auto label_text = reinterpret_cast<void (*)(void *, const char *)>(dlsym(plugin, "ply_label_set_text"));
    auto label_width = reinterpret_cast<long (*)(void *)>(dlsym(plugin, "ply_label_get_width"));
    auto label_height = reinterpret_cast<long (*)(void *)>(dlsym(plugin, "ply_label_get_height"));
    auto label_free = reinterpret_cast<void (*)(void *)>(dlsym(plugin, "ply_label_free"));
    assert(label_new && label_text && label_width && label_height && label_free);
    void *label = label_new();
    assert(label);
    // Match the engine's console-viewer admission check, including its space glyph.
    label_text(label, " ");
    const bool rendered = label_width(label) > 1 && label_height(label) > 1;
    label_free(label);
    if (!rendered) {
        std::cerr << "Plymouth text rendering unavailable: install its label plugin and fonts.\n";
        return 1;
    }
    dlclose(plugin);
    std::cout << "PASS: distribution Plymouth parses the theme, refuses malformed syntax and renders text.\n";
}
