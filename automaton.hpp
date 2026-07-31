#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <optional>
#include <utility>
#include <memory>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_set>
#include <ranges>

namespace automaton{

template <typename Domain, typename Codomain>
class Func{
private:
    Domain name;
    std::unordered_map<Domain, Codomain> mapping;
public:
    Func() = default;
    Func(const Domain& name) : name(name) {}
    Func(const Domain &name, const std::unordered_map<Domain, Codomain> &mapping) : mapping(mapping), name(name) {}
    Func(const Domain &name, std::unordered_map<Domain, Codomain> &&mapping) : mapping(std::move(mapping)), name(name) {}
    ~Func() = default;
    void add_element_to_domain(const Domain& dom, const Codomain& codom){
        mapping[dom] = codom;
    }
    void add_element_to_domain(const Domain& dom, Codomain&& codom){
        mapping[dom] = std::move(codom);
    }
    [[nodiscard]] std::optional<Codomain> map_element(const Domain& dom) const {
        auto it = mapping.find(dom);
        if (it != mapping.end())
            return it->second;
        return std::nullopt;
    }
    [[nodiscard]] std::optional<Codomain> operator () (const Domain& dom) const {
        return map_element(dom);
    }
};

class DFA{
private:
    std::vector<std::string> alphabet;
    std::vector<std::string> nodes;
    std::string initial_state;
    std::vector<std::string> final_states;
    std::unordered_map<std::string,Func<std::string, std::string>> transition_func;

    friend class NFA;
public:
    DFA() = default;
    DFA(std::ifstream &input_file){
        read_DFA(input_file);
    }
    void add_node(const std::string& node){
        nodes.push_back(node);
        transition_func.emplace(node, node);
    }
    void add_symbol(const std::string& symbol){
        alphabet.push_back(symbol);
    }
    void add_symbol(std::string&& symbol){
        alphabet.push_back(std::move(symbol));
    }
    void make_initial(const std::string_view initial){
        for (const auto& i : nodes){
            if (i == initial){
                initial_state = i;
                return;
            }
        }
    }
    void add_final(const std::string_view final_state){
        for (const auto& i : nodes){
            if (i == final_state){
                final_states.push_back(i);
                return;
            }
        }
    }
    void add_transition(const std::tuple<const std::string, const std::string, const std::string> &transition_add){
       transition_func.at(std::get<0>(transition_add)).add_element_to_domain(std::get<1>(transition_add), std::get<2>(transition_add));
    }
    std::optional<std::string> Transition(const std::pair<const std::string, const std::string> &input) const{
        if (transition_func.count(input.first))
            return transition_func.at(input.first).map_element(input.second);
        return std::nullopt;
    }
    void read_nodes(const std::string &nodes_line){
        std::stringstream ss(nodes_line);
        std::string token;
        std::getline(ss, token, ':');
        while (std::getline(ss, token, ';')){
            this->add_node(token);
        }
    }
    void read_symbols(const std::string &symbols_line){
        std::stringstream ss(symbols_line);
        std::string token;
        std::getline(ss, token, ':');
        while (std::getline(ss, token, ';')){
            this->add_symbol(std::move(token));
        }
    }
    void read_transitions(const std::string &transition_line){
        std::stringstream ss(transition_line);
        std::string token;
        std::getline(ss, token, ':');
        do {
            std::getline(ss, token, '(');
            std::string first;
            std::getline(ss, first, ',');
            std::string second;
            std::getline(ss, second, ',');
            std::string third;
            std::getline(ss, third, ')');
            add_transition(std::tuple(first, second, third));
        } while (std::getline(ss, token, ';'));
    }
    void read_final_states(const std::string &final_line){
        std::stringstream ss(final_line);
        std::string token;
        std::getline(ss, token, ':');
        while (std::getline(ss, token, ';')){
            add_final(token);
        }
    }
    void read_DFA(std::ifstream &input_file){
        if (!input_file.is_open())
            return;
        std::string line;
        if (!std::getline(input_file, line))
            throw std::runtime_error("Unexpected EOF");
        std::erase_if(line, [](unsigned char c){return std::isspace(c);});
        if (!line.starts_with("NODES:")){
            throw std::runtime_error("Invalid input file: Missing NODES");
        }
        read_nodes(line);
        if (!std::getline(input_file, line))
            throw std::runtime_error("Unexpected EOF");
        std::erase_if(line, [](unsigned char c){return std::isspace(c);});
        if (!line.starts_with("SYMBOLS:")){
            throw std::runtime_error("Invalid input file: Missing SYMBOLS");
        }
        read_symbols(line);
        if (!std::getline(input_file, line))
            throw std::runtime_error("Unexpected EOF");
        std::erase_if(line, [](unsigned char c){return std::isspace(c);});
        if (!line.starts_with("TRANSITIONS:")){
            throw std::runtime_error("Invalid input file: Missing TRANSITIONS");
        }
        read_transitions(line);
        if (!std::getline(input_file, line))
            throw std::runtime_error("Unexpected EOF");
        std::erase_if(line, [](unsigned char c){return std::isspace(c);});
        if (!line.starts_with("INITIAL:")){
            throw std::runtime_error("Invalid input file: Missing INITIAL");
        }
        std::stringstream ss(line);
        std::string token;
        std::getline(ss, token, ':');
        std::getline(ss, token, ';');
        make_initial(token);
        if (!std::getline(input_file, line))
            throw std::runtime_error("Unexpected EOF");
        std::erase_if(line, [](unsigned char c){return std::isspace(c);});
        if (!line.starts_with("FINALS:")){
            throw std::runtime_error("Invalid input file: Missing FINALS");
        }
        read_final_states(line);
    }
    [[nodiscard]] std::string simulate(const std::vector<std::string> &word) const{
        std::string ret = initial_state;
        for (const auto &i : word){
            auto tmp = transition_func.at(ret)(i);
            if (!tmp.has_value())
                throw std::invalid_argument("Simulation failed due to an invalid input file");
            ret = *tmp;
        }
        return ret;
    }
    [[nodiscard]] bool is_valid_word(const std::vector<std::string> &word) const{
        std::string intermediate = initial_state;
        for (const auto &i : word){
            auto tmp = transition_func.at(intermediate)(i);
            if (tmp.has_value())
                intermediate = *tmp;
            else{
                return false;
            }
        }
        for (const auto &i : final_states){
            if (intermediate == i)
                return true;
        }
        return false;
    }
    ~DFA() = default;
};



class NFA
{
private:
    struct UnorderedSetHash
    {
        std::size_t operator () (const std::unordered_set<std::string>& key) const {
            std::size_t ret = 0;
            for (const auto& str : key){
                ret += std::hash<std::string>{}(str);
            }
            return ret;
        }
    };
    using DFA_NODE = std::unordered_set<std::string>;
    std::vector<std::string> alphabet;
    std::vector<std::string> nodes;
    std::string initial_state;
    std::vector<std::string> final_states;
    std::unordered_map<std::string,Func<std::string, DFA_NODE>> transition_func;
    DFA equiv_dfa;
    std::unordered_map<std::string, DFA_NODE> epsilon_closures;
    std::vector<DFA_NODE> equiv_dfa_nodes;
    DFA_NODE equiv_dfa_init_node;
    std::unordered_set<std::string> final_states_closure;//epsilon closures of the NFA final states
    std::unordered_map<DFA_NODE, Func<std::string, DFA_NODE>, UnorderedSetHash> equiv_dfa_transition_func;
    void finally_fill_out_equiv_dfa(){
        equiv_dfa.alphabet = alphabet;
        equiv_dfa.nodes = {};
        for (const auto &node : equiv_dfa_nodes){
            equiv_dfa.nodes.push_back(generate_string_from_DFA_node(node));
            Func<std::string, std::string> func_for_node(generate_string_from_DFA_node(node));
            for (const auto &letter : alphabet){
                func_for_node.add_element_to_domain(letter, generate_string_from_DFA_node(*equiv_dfa_transition_func.at(node)(letter)));
            }
            equiv_dfa.transition_func[generate_string_from_DFA_node(node)] = std::move(func_for_node);
        }
        equiv_dfa.initial_state = generate_string_from_DFA_node(equiv_dfa_init_node);
        equiv_dfa.final_states = {};
        for (const auto &dfa_node : equiv_dfa_nodes){
            for (const auto &nfa_state : dfa_node){
                if (std::ranges::contains(final_states, nfa_state)){
                    equiv_dfa.final_states.push_back(generate_string_from_DFA_node(dfa_node));
                    break;
                }
            }
        }
    }
    void compute_epsilon_closure(const std::string &node, std::unordered_set<std::string>& closure){
        if (closure.contains(node))
            return;
        closure.insert(node);
        auto result = transition_func.at(node)("");
        if (result){
            for (const auto &new_node : *result){
                compute_epsilon_closure(new_node, closure);
                closure.insert(new_node);
            }
        }
    }
    std::unordered_set<std::string> find_reachable_set(const std::unordered_set<std::string> &DFA_node, const std::string &input_symbol){
        std::unordered_set<std::string> buffer{};
        for (const auto &i : DFA_node){
            auto tmp = transition_func.at(i)(input_symbol);
            if (tmp){
                for (const auto &ins : *tmp)
                    buffer.insert(ins);
            }
        }
        std::unordered_set<std::string> ret{};
        for (const auto &i : buffer){
            //auto tmp = (epsilon_closures.contains(i)) ? epsilon_closures.at(i) : get_epsilon_closure(i);
            auto& tmp = epsilon_closures.at(i);
            ret.insert(tmp.begin(), tmp.end());
        }
        return ret;
    }
    //node has to already be pushed onto equiv_dfa_nodes
    void dfa_add_transitions_for_node(const DFA_NODE &node){
        if (equiv_dfa_transition_func.contains(node))
            return;
        Func<std::string, DFA_NODE> transition_function{};
        for (const auto &letter : alphabet){
            auto tmp = find_reachable_set(node, letter);
            transition_function.add_element_to_domain(letter, tmp);
            if (!std::ranges::contains(equiv_dfa_nodes, tmp)){
                equiv_dfa_nodes.push_back(tmp);
                dfa_add_transitions_for_node(tmp);
            }
        }
        equiv_dfa_transition_func[node] = std::move(transition_function);
    }
    std::string generate_string_from_DFA_node(const DFA_NODE &node) const {
        if (node.empty())
            return "{}";
        std::vector<std::string_view> node_components{};
        for (const auto &i : node)
            node_components.push_back(i);
        std::sort(node_components.begin(), node_components.end());
        std::string ret = "{";
        for (const auto &i : node_components){
            ret += i;
            ret += ",";
        }
        ret.back() = '}';
        return ret;
    }
    void convert_to_DFA(){
        fill_epsilon_closures();
        std::string dfa_init_string = generate_string_from_DFA_node(equiv_dfa_init_node);
        equiv_dfa_nodes = {equiv_dfa_init_node};
        dfa_add_transitions_for_node(equiv_dfa_init_node);
        finally_fill_out_equiv_dfa();
    }
public:
    NFA() = default;
    NFA(std::ifstream &input_file){
        read_NFA(input_file);
    }
    void print_out_NFA() const {
        std::cout << "These are the nodes:" << std::endl;
        for (const auto &i : equiv_dfa_nodes)
            std::cout << generate_string_from_DFA_node(i) << std::endl;
        std::cout << "These are the letters:" << std::endl;
        for (const auto &i : alphabet)
            std::cout << i << std::endl;
    }
    std::unordered_set<std::string> get_epsilon_closure(const std::string &node){
        std::unordered_set<std::string> closure{};
        compute_epsilon_closure(node, closure);
        if (node == initial_state)
            equiv_dfa_init_node = closure;
        if (std::ranges::contains(final_states, node))
            final_states_closure.insert(closure.begin(), closure.end());
        return closure;
    }
    void fill_epsilon_closures(){
        for (const auto& node : nodes){
            epsilon_closures[node] = get_epsilon_closure(node);
        }
    }
    void add_node(const std::string& node){
        nodes.push_back(node);
        transition_func.emplace(node, node);
    }
    void add_symbol(const std::string& symbol){
        alphabet.push_back(symbol);
    }
    void add_symbol(std::string&& symbol){
        alphabet.push_back(std::move(symbol));
    }
    void make_initial(const std::string_view initial){
        for (const auto& i : nodes){
            if (i == initial){
                initial_state = i;
                return;
            }
        }
    }
    void add_final(const std::string_view final_state){
        for (const auto& i : nodes){
            if (i == final_state){
                final_states.push_back(i);
                return;
            }
        }
    }
    void add_transition(const std::tuple<const std::string, const std::string, const std::unordered_set<std::string>> &transition_add){
        transition_func.at(std::get<0>(transition_add)).add_element_to_domain(std::get<1>(transition_add), std::get<2>(transition_add));
    };
    void read_nodes(const std::string &nodes_line){
        std::stringstream ss(nodes_line);
        std::string token;
        std::getline(ss, token, ':');
        while (std::getline(ss, token, ';')){
            this->add_node(token);
        }
    }
    void read_symbols(const std::string &symbols_line){
        std::stringstream ss(symbols_line);
        std::string token;
        std::getline(ss, token, ':');
        while (std::getline(ss, token, ';')){
            this->add_symbol(std::move(token));
        }
    }
    void read_transitions(const std::string &transition_line){
        std::stringstream ss(transition_line);
        std::string token;
        std::getline(ss, token, ':');
        do {
            std::getline(ss, token, '(');
            std::string first;
            std::getline(ss, first, ',');
            std::string second;
            std::getline(ss, second, ',');
            std::unordered_set<std::string> third;
            std::string tmp;
            std::getline(ss, tmp, '{');
            std::getline(ss, tmp, '}');
            if (!tmp.empty())
                if (tmp.back() == '}')
                    tmp.pop_back();
            if (!tmp.empty()){
                std::stringstream tmp_stream(tmp);
                std::string individual;
                while (std::getline(tmp_stream, individual, ',')){
                    third.insert(std::move(individual));
                }
            }
            add_transition(std::tuple(first, second, third));
            std::getline(ss, tmp, ')');
        } while (std::getline(ss, token, ';'));
    }
    void read_final_states(const std::string &final_line){
        std::stringstream ss(final_line);
        std::string token;
        std::getline(ss, token, ':');
        while (std::getline(ss, token, ';')){
            add_final(token);
        }
    }
    void read_NFA(std::ifstream &input_file){
        if (!input_file.is_open())
            return;
        std::string line;
        if (!std::getline(input_file, line))
            throw std::runtime_error("Unexpected EOF");
        std::erase_if(line, [](unsigned char c){return std::isspace(c);});
        if (!line.starts_with("NODES:")){
            throw std::runtime_error("Invalid input file: Missing NODES");
        }
        read_nodes(line);
        if (!std::getline(input_file, line))
            throw std::runtime_error("Unexpected EOF");
        std::erase_if(line, [](unsigned char c){return std::isspace(c);});
        if (!line.starts_with("SYMBOLS:")){
            throw std::runtime_error("Invalid input file: Missing SYMBOLS");
        }
        read_symbols(line);
        if (!std::getline(input_file, line))
            throw std::runtime_error("Unexpected EOF");
        std::erase_if(line, [](unsigned char c){return std::isspace(c);});
        if (!line.starts_with("TRANSITIONS:")){
            throw std::runtime_error("Invalid input file: Missing TRANSITIONS");
        }
        read_transitions(line);
        if (!std::getline(input_file, line))
            throw std::runtime_error("Unexpected EOF");
        std::erase_if(line, [](unsigned char c){return std::isspace(c);});
        if (!line.starts_with("INITIAL:")){
            throw std::runtime_error("Invalid input file: Missing INITIAL");
        }
        std::stringstream ss(line);
        std::string token;
        std::getline(ss, token, ':');
        std::getline(ss, token, ';');
        make_initial(token);
        if (!std::getline(input_file, line))
            throw std::runtime_error("Unexpected EOF");
        std::erase_if(line, [](unsigned char c){return std::isspace(c);});
        if (!line.starts_with("FINALS:")){
            throw std::runtime_error("Invalid input file: Missing FINALS");
        }
        read_final_states(line);
        convert_to_DFA();
    }
    [[nodiscard]] std::string simulate(const std::vector<std::string> &word) const {
        DFA_NODE ret = equiv_dfa_init_node;
        for (const auto &i : word){
            auto tmp = equiv_dfa_transition_func.at(ret)(i);
            if (!tmp.has_value())
                throw std::invalid_argument("Simulation failed: invalid input file");
            ret = *tmp;
        }
        return generate_string_from_DFA_node(ret);
    }
    [[nodiscard]] bool is_valid_word(const std::vector<std::string> &word) const {
        DFA_NODE intermediate = equiv_dfa_init_node;
        for (const auto &i : word){
            auto tmp = equiv_dfa_transition_func.at(intermediate)(i);
            if (tmp.has_value())
                intermediate = *tmp;
            else{
                return false;
            }
        }
        for (const auto& state: intermediate){
            if (std::ranges::contains(final_states, state))
                return true;
        }
        return false;
    }
    ~NFA() = default;
};
}
