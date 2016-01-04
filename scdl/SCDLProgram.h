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

#ifndef SCDL_PROGRAM_H
#define SCDL_PROGRAM_H

#include <string>
#include <vector>
#include <map>
#include <cstdlib>
#include <iostream>
#include "Circuit.h"

#include <boost/lexical_cast.hpp>

namespace scdl {
namespace compiler {

struct Constant {
    int value;
    unsigned int input_index;
};

struct Variable {
    size_t len;
    unsigned int input_index;
};




class SCDLProgram {
 public:

    ~SCDLProgram();

    bool has_variable(const std::string &var_name) const;
    Variable get_variable(const std::string &var_name) const;
    Constant get_constant(const std::string &const_name) const;
    Constant get_constant(unsigned int constant_no) const;
    std::string get_variable_name(unsigned int input_index) const;
    size_t get_num_variables() const;
    size_t get_num_variable_inputs() const;
    Circuit *get_circuit(const std::string &circuit_name) const;
    std::vector<std::string>::const_iterator get_circuit_names() const;
    bool has_circuit(const std::string &circuit_name) const;
    size_t get_num_circuits() const;
    bool has_input_variable(const std::string &var_name) const;
    size_t get_num_constants() const;
    bool has_constant(const std::string &const_name) const;
    std::string get_constant_name(unsigned int constant_no) const;

    template <class T>
    T run(const std::string &circuit_name, T *var_inputs, T *constants) {
        if (circuit_map.find(circuit_name) == circuit_map.end())
            throw "Could not find circuit";
        Circuit *circuit = circuit_map[circuit_name];
        std::vector<T> inputs;

        for (int i = 0; i < n_var_inputs; i++)
            inputs.push_back(var_inputs[i]);

        for (int i =0; i < const_names.size(); i++) {            
            Constant constant = const_map[const_names[i]];

            inputs.push_back(constants[i]);
        }

        return circuit->evaluate(&inputs[0], true);
    }

    template <class T>
    T run(T *var_inputs, T *constants) {
        return run("out", var_inputs, constants);
    }

       
    static SCDLProgram *compile_program_from_stream(std::istream &in);
    static SCDLProgram *compile_program_from_file(std::string file_name);

 protected:
    SCDLProgram(std::map<std::string,Gate*> &func_gates,
                std::map<std::string,Variable> &var_map,
                std::map<std::string,Constant> &const_map);



 private:
    std::map<std::string,Constant> const_map;
    std::vector<std::string> const_names;
    std::map<std::string,Variable> var_map;
    std::vector<std::string> var_names;
    std::map<std::string,Circuit*> circuit_map;
    size_t n_var_inputs;
    std::vector<std::string> circuit_names;

};

}
}

#endif // SCDL_PROGRAM_H
