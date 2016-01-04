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

#include "FHE.h"
#include "EncryptedArray.h"
#include <NTL/lzz_pXFactoring.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <exception>
#include <sys/time.h>

#include <boost/lexical_cast.hpp>

#include "scdl/SCDLEvaluator.h"

// Parameters
enum {
    P = 2, // size of plaintext space
    R = 1,
    C = 3, // columns in key switching matrix
    W = 64, // hamming weight of secret key
    D = 0
};

enum CiphertextType {
    CT_FRESH = 0,
    CT_EVALUATED = 1
};

template <typename T>
T convert_from_string(const string &s, const char *err_msg)
{
    try {
        return boost::lexical_cast<T>(s);
    }
    catch (boost::bad_lexical_cast &e) {
        throw err_msg;
    }
}

// Keypair generation
void run_gen_command(vector<string> args)
{
    if (args.size() < 4) {
        throw "usage: fhe gen <security parameter> <levels> <public key> "
              "<secret key>";
    }

    unsigned int sec_param = convert_from_string<unsigned int>(args[0],
        "Invalid value for security paraemter"
    );

    unsigned int nlevels = convert_from_string<unsigned int>(args[1],
        "Invalid value for number of levels"
    );

    long m = FindM(sec_param, nlevels, C, P, D, 0, 0);
    FHEcontext context(m, P, R); // initialize context
    buildModChain(context, nlevels, C); // add primes to the modulus chain
    FHESecKey sec_key(context);
    const FHEPubKey& pub_key = sec_key;
    
    ZZX G = context.alMod.getFactorsOverZZ()[0];
    sec_key.GenSecKey(W);
    addSome1DMatrices(sec_key);

    EncryptedArray ea(context, G);
    unsigned long nslots = ea.size();


    // Write out public key
    std::ofstream pk_stream(args[2].c_str(), std::ios::out);
    pk_stream << sec_param << endl << nlevels << endl << pub_key << endl;
    pk_stream.close();

    // Write out secret key
    std::ofstream sk_stream(args[3].c_str(), std::ios::out);
    sk_stream << sec_param << endl << nlevels << sec_key << endl;
    sk_stream.close();
}

// Encryption
void run_encrypt_command(vector<string> args)
{
    if (args.size() < 3) {
        throw "usage: fhe encrypt <public key file> <scdl file> "
              "<output ciphertext file>";
    }

    std::ifstream pk_in(args[0].c_str());
    if (!pk_in.good()) {
        throw (args[0] + " could not be opened").c_str();
    }

    std::ifstream scdl_in(args[1].c_str());
    if (!scdl_in.good()) {
        throw (args[1] + " could not be opened").c_str();
    }

    std::ifstream vars_in((args[1] + ".vars").c_str());
    if (!vars_in.good()) {
        throw "vars file could not be opened";
    }

    std::ofstream out(args[2].c_str());
    if (!out.good()) {
        throw (args[2] + " could not be opened").c_str();
    }

    scdl::CompilerResult result = scdl::SCDLEvaluator::compile(scdl_in,
                                                               vars_in);
    
    size_t n_bit_inputs = result.program->get_num_variable_inputs();
    int *bit_inputs = new int[n_bit_inputs];

    std::vector<scdl::Variable>::iterator itr;
    for (itr = result.vars.inputs.begin(); itr != result.vars.inputs.end();
         itr++) {
        scdl::Variable var = *itr;
        scdl::read_variable(result.program, var, bit_inputs, n_bit_inputs);
    }

    // Read security parameter and number of levels from public key file
    unsigned int sec_param;
    unsigned int nlevels;
    pk_in >> sec_param >> nlevels;
    sec_param >> nlevels;

    // Set up Context and EncryptedArray
    long m = FindM(sec_param, nlevels, C, P, D, 0, 0);
    FHEcontext context(m, P, R); // initialize context
    buildModChain(context, nlevels, C); // add primes to the modulus chain
    ZZX G = context.alMod.getFactorsOverZZ()[0];
    EncryptedArray ea(context, G);

    // Read public key
    FHEPubKey pk(context);
    pk_in >> pk;

    unsigned long nslots = ea.size();
    
    vector<long> plaintext;
    for (int i = 0; i < nslots; i++)
        plaintext.push_back(0);

    out << CT_FRESH << endl; // ciphertext type
    out << n_bit_inputs << endl;
    for (int i = 0; i < n_bit_inputs; i++) {
        Ctxt ciphertext(pk);
        plaintext[0] = bit_inputs[i];
        ea.encrypt(ciphertext, pk, plaintext);
        out << ciphertext << endl;
    }

    pk_in.close();
    scdl_in.close();
    vars_in.close();
    out.close();

    delete[] bit_inputs;
}

/*
 * Evaluation: this code needs to be refactored as there is too much going
 * on here in the one function; we need to split it up so that it calls
 * smaller helper functions.
 */
void run_eval_command(vector<string> args)
{
    if (args.size() < 4) {
        throw "usage: fhe eval <public key file> <scdl file> "
              "<inputs ciphertext file> <output ciphertext file>";
    }

    std::ifstream pk_in(args[0].c_str());
    if (!pk_in.good()) {
        throw (args[0] + " could not be opened").c_str();
    }

    std::ifstream scdl_in(args[1].c_str());
    if (!scdl_in.good()) {
        throw (args[1] + " could not be opened").c_str();
    }

    std::ifstream vars_in((args[1] + ".vars").c_str());
    if (!vars_in.good()) {
        throw "vars file could not be opened";
    }

    std::ifstream ct_in(args[2].c_str());
    if (!ct_in.good()) {
        throw (args[2] + " could not be opened").c_str();
    }

    std::ofstream out(args[3].c_str());
    if (!out.good()) {
        throw (args[3] + " could not be opened").c_str();
    }

    scdl::CompilerResult result = scdl::SCDLEvaluator::compile(scdl_in,
                                                               vars_in);

    int ct_type;
    ct_in >> ct_type;

    if (ct_type != CT_FRESH) {
        throw "Input ciphertext is not fresh";
    }

    size_t n_inputs;
    ct_in >> n_inputs;

    if (n_inputs != result.program->get_num_variable_inputs()) {
        throw "Number of inputs specified in the ciphertext file does not "
              "match the number of inputs required by the program";
    }

    // Read security parameter and number of levels from public key file
    unsigned int sec_param;
    unsigned int nlevels;
    pk_in >> sec_param >> nlevels;

    // Set up Context and EncryptedArray
    long m = FindM(sec_param, nlevels, C, P, D, 0, 0);
    FHEcontext context(m, P, R); // initialize context
    buildModChain(context, nlevels, C); // add primes to the modulus chain
    ZZX G = context.alMod.getFactorsOverZZ()[0];
    EncryptedArray ea(context, G);

    // Read public key
    FHEPubKey pk(context);
    pk_in >> pk;
    

    vector<Ctxt> inputs;
    for (int i = 0; i < n_inputs; i++) {
        Ctxt input(pk);
        ct_in >> input;
        inputs.push_back(input);
    }

    unsigned long nslots = ea.size();
    vector<long> plaintext;
    for (int i = 0; i < nslots; i++)
        plaintext.push_back(0);

    
    size_t n_constants = result.program->get_num_constants();
    vector<Ctxt> constants;
    for (int i = 0; i < n_constants; i++) {
        int v = result.program->get_constant(i).value;
        plaintext[0] = v;
        Ctxt constant(pk);
        ea.encrypt(constant, pk, plaintext);
        constants.push_back(constant);
    }

    std::set<string> evaluated_wires;
    vector<Ctxt> wire_ciphertexts;
    vector<string> wire_names;
    vector<scdl::Variable>::iterator itr;
    scdl::Vars vars = result.vars;
    for (itr = vars.outputs.begin(); itr != vars.outputs.end(); itr++) {
        scdl::Variable var = *itr;
        for (int i = 0; i < var.components.size(); i++) {
            cout << "Running bit " << i << endl;
            // If we have not evaluated the wire yet, evaluate now
            if (evaluated_wires.find(var.components[i]) == evaluated_wires.end()) {
                if (!result.program->has_circuit(var.components[i])) {
                    throw ("Could not find definition for "
                           + var.components[i]).c_str();
                }
                int depth =
                    result.program->get_circuit(var.components[i])->get_mult_depth();
                if (depth > nlevels)
                    throw "Depth of circuit exceeds the maximum supported depth";
                cout << "depth: " << depth << endl;
                Ctxt c = result.program->run(var.components[i], &inputs[0],
                                             &constants[0]);
                wire_ciphertexts.push_back(c);
                wire_names.push_back(var.components[i]);
                evaluated_wires.insert(var.components[i]);
            }
        }
    }

    out << CT_EVALUATED << endl;
    out << vars.outputs.size() << endl;
    for (itr = vars.outputs.begin(); itr != vars.outputs.end(); itr++) {
        scdl::Variable var = *itr;
        out << var.name << endl << var.type << endl
            << var.components.size() << endl;
        for (int i = 0; i < var.components.size() - 1; i++) {
            out << var.components[i] << ' ';
        }
        if (var.components.size() > 0) {
            out << var.components[var.components.size() - 1] << endl;
        }
    }
    out << wire_ciphertexts.size() << endl;
    for (int i = 0; i < wire_ciphertexts.size(); i++) {
        out << wire_names[i] << ' ' << wire_ciphertexts[i] << endl;
    }

    pk_in.close();
    scdl_in.close();
    vars_in.close();
    ct_in.close();
    out.close();
}

// Decryption
void run_decrypt_command(vector<string> args)
{
    if (args.size() < 2) {
        throw "usage: fhe decrypt <secret key file> <ciphertext file> ";
    }

    std::ifstream sk_in(args[0].c_str());
    if (!sk_in.good()) {
        throw (args[0] + " could not be opened").c_str();
    }

    std::ifstream ct_in(args[1].c_str());
    if (!ct_in.good()) {
        throw (args[1] + " could not be opened").c_str();
    }

    // Read security parameter and number of levels from secret key file
    unsigned int sec_param;
    unsigned int nlevels;
    sk_in >> sec_param >> nlevels;

    // Set up Context and EncryptedArray
    long m = FindM(sec_param, nlevels, C, P, D, 0, 0);
    FHEcontext context(m, P, R); // initialize context
    buildModChain(context, nlevels, C); // add primes to the modulus chain
    ZZX G = context.alMod.getFactorsOverZZ()[0];
    EncryptedArray ea(context, G);

    // Read secret key
    FHESecKey sk(context);
    sk_in >> sk;

    const FHEPubKey& pk = sk;

    // Read ciphertext
    int type;
    ct_in >> type;
    if (type != CT_EVALUATED)
        throw "Currently only decryption of evaluated ciphertexts is supported";

    size_t n_output_vars;
    ct_in >> n_output_vars;
    vector<scdl::Variable> output_vars;
    for (int i = 0; i < n_output_vars; i++) {
        scdl::Variable var;
        ct_in >> var.name;

        int var_type;
        ct_in >> var_type;
        var.type = (scdl::VariableType)var_type;

        size_t n_components;
        ct_in >> n_components;
        for (int j = 0; j < n_components; j++) {
            string component;
            ct_in >> component;
            var.components.push_back(component);
        }
        output_vars.push_back(var);
    }

    size_t n_wires;
    ct_in >> n_wires;
    map<string,int> wire_bits;
    for (int i = 0; i < n_wires; i++) {
        string wire_name;
        ct_in >> wire_name;

        Ctxt ciphertext(pk);
        ct_in >> ciphertext;
        vector<long> decrypted;
        ea.decrypt(ciphertext, sk, decrypted);
        wire_bits[wire_name] = (int)decrypted[0];
    }

    // Print variables
    for (vector<scdl::Variable>::iterator itr = output_vars.begin();
             itr != output_vars.end(); itr++) {
        scdl::print_variable(*itr, wire_bits);
    }

    sk_in.close();
    ct_in.close();
}

// Depth calculation
void run_depth_command(vector<string> args)
{
    if (args.size() < 1) {
        throw "usage: fhe depth <scdl file>";
    }

    std::ifstream scdl_in(args[0].c_str());
    if (!scdl_in.good()) {
        throw (args[0] + " could not be opened").c_str();
    }

    std::ifstream vars_in((args[0] + ".vars").c_str());
    if (!vars_in.good()) {
        throw "vars file could not be opened";
    }

    scdl::CompilerResult result = scdl::SCDLEvaluator::compile(scdl_in,
                                                               vars_in);
    scdl::Vars vars = result.vars;
    vector<scdl::Variable>::iterator itr;
    int max_depth = 0;
    for (itr = vars.outputs.begin(); itr != vars.outputs.end(); itr++) {
        scdl::Variable var = *itr;
        for (int i = 0; i < var.components.size(); i++) {
            if (!result.program->has_circuit(var.components[i])) {
                throw ("Could not find definition for "
                       + var.components[i]).c_str();
            }
            int depth =
                result.program->get_circuit(var.components[i])->get_mult_depth();
            if (depth > max_depth)
                max_depth = depth;
        }
    }

    cout << "Maximum depth: " << max_depth << endl;

    scdl_in.close();
    vars_in.close();
}
    
int main(int argc, char *argv[])
{
    if (argc < 2) {
        cerr << "usage: fhe <command>" << endl;
        exit(1);
    }

    string cmd = argv[1];
    vector<string> args;

    for (int i = 2; i < argc; i++)
        args.push_back(argv[i]);
    
    try {
        if (cmd == "gen") {
            run_gen_command(args);
        }
        else if (cmd == "encrypt") {
            run_encrypt_command(args);
        }
        else if (cmd == "eval") {
            run_eval_command(args);
        }
        else if (cmd == "decrypt") {
            run_decrypt_command(args);
        }
        else if (cmd == "depth") {
            run_depth_command(args);
        }
        else {
            throw "Unknown command";
        }
    }
    catch (const char *e) {
        cerr << e << endl;
        exit(1);
    }        

    return 0;
}


