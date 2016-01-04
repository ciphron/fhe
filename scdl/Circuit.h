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

#ifndef CIRCUIT_H
#define CIRCUIT_H

#include <set>
#include <vector>
#include <map>
#include <cstdlib>
#include <stdint.h>



#include <iostream>

namespace scdl {
   
enum GateType {
    GATE_MULT,
    GATE_ADD,
    GATE_OUT,
    GATE_IN
};


struct Gate {
    GateType type;
    unsigned int input_index;
    size_t fan_in;
    Gate **in_gates;
};

struct InternalGate {
    GateType type;
    unsigned int input_index;
    size_t fan_in;
    unsigned int *in_gates;
    bool visited;
    int depth;
};



typedef std::set<Gate*> GateSet;

class Circuit {
public:
    Circuit() {n_inputs = 0;}

    Circuit(size_t n_inputs, Gate *output_gate)
        : n_inputs(n_inputs), mult_depth(0) {
        std::map<Gate*,unsigned int> visited;
        if (!check_well_formed(gates, n_inputs, output_gate, visited,
                               &output_gate_index)) {
            free_gates();
            throw "Circuit not well formed";
        }
        mult_depth = compute_depth();
        count_gates();
    }
    ~Circuit() {
        free_gates();
    }

    size_t get_num_inputs() const {
        return n_inputs;
    }

    int get_mult_depth() const {
        return mult_depth;
    }

    size_t get_num_add_gates() const {
        return n_add_gates;
    }

    size_t get_num_mult_gates() const {
        return n_mult_gates;
    }

    template <class T>
    T evaluate(const T *inputs, bool store=false) {
        for (int i = 0; i < gates.size(); i++)
            gates[i].visited = false;

        if (store) {
            // The line below does not work because T may not have a default
            // constructor so we have to do it a different way
            // std::vector<T> stored(gates.size());
            std::vector<T> stored;
            for (int i = 0; i < gates.size(); i++)
                stored.push_back(inputs[0]);
            return eval_gate_with_store(output_gate_index, inputs, &stored[0]);
        }
        else
            return eval_gate_no_store(output_gate_index, inputs);
    }



    template <class T>
    T eval_gate_with_store(unsigned int gate_index, const T *inputs,
                           T *stored) {
            InternalGate &gate = gates[gate_index];
            if (gate.visited) {
                return stored[gate_index];
            }

            if (gate.type == GATE_IN) {
                T value = inputs[gate.input_index];
                stored[gate_index] = value;
                gate.visited = true;
                return value;
            }

            int fan_in = gate.fan_in;
            T aggr = eval_gate_with_store(gate.in_gates[0], inputs, stored);
            for (int i = 1; i < fan_in; i++) {
                T v = eval_gate_with_store(gate.in_gates[i], inputs, stored);
                if (gate.type == GATE_MULT) {
                    aggr *= v;
                }
                else if (gate.type == GATE_ADD)
                    aggr += v;
            }
            stored[gate_index] = aggr;
            gate.visited = true;
            
            return aggr;
        }

    template <class T> 
        T eval_gate_no_store(unsigned int gate_index, const T *inputs)
    {
            InternalGate &gate = gates[gate_index];

            if (gate.type == GATE_IN) {
                T value = inputs[gate.input_index];
                gate.visited = true;
                return value;
            }

            int fan_in = gate.fan_in;

            T aggr = eval_gate_no_store(gate.in_gates[0], inputs);
            for (int i = 1; i < fan_in; i++) {
                T v = eval_gate_no_store(gate.in_gates[i], inputs);
                if (gate.type == GATE_MULT)
                    aggr *= v;
                else if (gate.type == GATE_ADD)
                    aggr += v;
            }

            gate.visited = true;
            return aggr;
        }


 private:
    unsigned int output_gate_index;
    std::vector<InternalGate> gates;
    size_t n_inputs;
    size_t n_add_gates;
    size_t n_mult_gates;
    int mult_depth;

    int compute_depth();
    int compute_depth_rec(unsigned int gate_index, int depth);
    void count_gates();
    void count_gates_rec(unsigned int gate_index);

    bool check_well_formed(std::vector<InternalGate> &gates, size_t n_inputs,
                           Gate *current_gate, std::map<Gate*,unsigned int> &visited,
                           unsigned int *gate_index);
    
    void free_gates();

    
};

Gate input_gate(unsigned int index);
Gate operator_gate(GateType type, Gate *in1, Gate *in2);

Gate *new_input_gate(unsigned int index);
Gate *new_operator_gate(GateType type, Gate *in1, Gate *in2);

}

#endif // CIRCUIT_H
