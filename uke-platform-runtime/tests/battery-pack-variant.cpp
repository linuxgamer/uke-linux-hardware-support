// SPDX-License-Identifier: MIT
#include "../src/battery/pack-variant.hpp"
#include <cassert>
#include <string>

int main() {
    using uke::battery::pack_supplier_hint;
    assert(pack_supplier_hint("NVT", "") == "nvt");
    assert(pack_supplier_hint("sunwoda", "BMXX") == "sunwoda");
    assert(pack_supplier_hint("", "Battery-NVT") == "nvt");
    assert(pack_supplier_hint("Xiaomi", "SUNWODA / BMXX") == "sunwoda");
    assert(pack_supplier_hint("NVT", "SUNWODA") == "conflicting");
    assert(pack_supplier_hint("NVT/SUNWODA", "") == "conflicting");
    assert(pack_supplier_hint("", "") == "unknown");
    assert(pack_supplier_hint("", "BMXX") == "unknown");
    assert(pack_supplier_hint("", "NVTX") == "unknown");
    assert(pack_supplier_hint("", "XSUNWODA") == "unknown");
    assert(pack_supplier_hint("", std::string("NVT\0SUNWODA", 11)) == "unknown");
    assert(pack_supplier_hint("NVT", std::string(129, 'x')) == "unknown");
    assert(pack_supplier_hint("NVT", "NVT") == "nvt");
}
