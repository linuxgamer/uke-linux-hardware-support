// SPDX-License-Identifier: MIT
#include "backend.hpp"
#include <cassert>
#include <chrono>
#include <ctime>
#include <iostream>

int main() {
    const auto started=std::chrono::steady_clock::now();
    const auto cpu=std::clock();
    const auto closed=uke::execute({"/usr/bin/bash","-c","exec 1>&- 2>&-; /usr/bin/sleep 1; exit 7"},4,4096);
    const auto elapsed=std::chrono::steady_clock::now()-started;
    const auto cpu_ms=1000.0*(std::clock()-cpu)/CLOCKS_PER_SEC;
    std::cout<<"Closed output: wall_ms="<<std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()
             <<" parent_cpu_ms="<<cpu_ms<<std::endl;
    assert(closed.status==7&&closed.output.empty());
    assert(elapsed>=std::chrono::milliseconds(900)&&cpu_ms<250);

    const auto timeout=uke::execute({"/usr/bin/bash","-c","exec 1>&- 2>&-; /usr/bin/sleep 5"},1,4096);
    assert(timeout.status==124&&timeout.output.find("time/output limit")!=std::string::npos);
    const auto output=uke::execute({"/usr/bin/printf","%s","literal;$HOME"},2,4096);
    assert(output.status==0&&output.output=="literal;$HOME");
    const auto limited=uke::execute({"/usr/bin/yes"},2,4096);
    assert(limited.status==124&&limited.output.starts_with("y\ny\n")&&limited.output.size()<4200);
    std::cout<<"Actual command runner: early EOF, exit status, timeout, literal arguments and output limit passed\n";
}
