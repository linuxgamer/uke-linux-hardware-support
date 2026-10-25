// SPDX-License-Identifier: GPL-2.0-or-later
#include "../src/touch-thp-geometry.hpp"
#include <cassert>
#include <tuple>

int main(int argc, char **argv) {
    assert(argc==3);
    for(const auto &[path,panel,other]:{
            std::tuple{argv[1],NormalPanel::Tm,NormalPanel::Csot},
            std::tuple{argv[2],NormalPanel::Csot,NormalPanel::Tm}}) {
        const auto geometry=touch_geometry(path,panel);
        assert(geometry.width==32000 && geometry.height==21360);
        bool refused=false;
        try {touch_geometry(path,other);}
        catch(const std::runtime_error &error) {
            refused=std::string(error.what())=="O82 configuration does not match selected touch panel";
        }
        assert(refused);
    }
}
