/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SCDL_EVALUATOR_H
#define SCDL_EVALUATOR_H

#include <iostream>
#include <cstdlib>

#include "SCDLProgram.h"
#include "Variable.h"

namespace scdl {

struct Vars {
    std::vector<scdl::Variable> inputs;
    std::vector<scdl::Variable> outputs;
};

struct CompilerResult {
    scdl::compiler::SCDLProgram *program;
    Vars vars;
};

Vars *read_vars_file(std::istream &in);
void read_variable(const compiler::SCDLProgram *prog,
                   const Variable &var,
                   int *bit_inputs, size_t n_bit_inputs);
void print_variable(const Variable &var,
                    std::map<std::string,int> &wire_bits);



class SCDLEvaluator {
 public:
    SCDLEvaluator();

    static CompilerResult compile(std::istream &scdl_in, std::istream &vars_in);
    //       throws const char *;

    static void evaluate(CompilerResult &result);
        //throws const *char;
};
}

#endif // SCDL_EVALUATOR
