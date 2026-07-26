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


template <typename Domain, typename Codomain>
class Func
{
private:
    std::vector<std::pair<Domain, Codomain>> mapping;
public:
    Domain name;
    Func() = default;
    Func(const Domain& name){
        this->name = name;
    }
    Func(const Domain &name, const std::vector<std::pair<Domain, Codomain>> &mapping){
        this->mapping = mapping;
        this->name = name;
    }
    Func(const Domain &name, std::vector<std::pair<Domain, Codomain>> &&mapping){
        this->mapping = std::move(mapping);
        this->name = name;
    }
    ~Func() = default;
    void add_element_to_domain(const Domain& dom, const Codomain& codom){
        mapping.push_back(std::pair(dom, codom));
    }
    void add_element_to_domain(const Domain& dom, Codomain&& codom){
        mapping.push_back(std::pair(dom, std::move(codom)));
    }
    std::optional<Codomain> map_element(const Domain& dom) const {
        for (const auto& i : mapping){
            if (i.first == dom)
                return i.second;
        }
        return std::nullopt;
    }
    std::optional<Codomain> operator () (const Domain& dom) const {
        return map_element(dom);
    }
};

class DFA
{
private:
    using string_ptr = std::shared_ptr<const std::string>;
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
    };
    std::optional<std::string> Transition(const std::pair<const std::string, const std::string> &input){
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
            return;
        if (!line.starts_with("NODES:")){
            std::cout << "oh well..." << std::endl;
            return;
        }
        read_nodes(line);
        if (!std::getline(input_file, line))
            return;
        if (!line.starts_with("SYMBOLS:")){
            std::cout << "oh well for symbols..." << std::endl;
            return;
        }
        read_symbols(line);
        if (!std::getline(input_file, line))
            return;
        if (!line.starts_with("TRANSITIONS:")){
            std::cout << "oh well for transitions..." << std::endl;
            return;
        }
        read_transitions(line);
        if (!std::getline(input_file, line))
            return;
        if (!line.starts_with("INITIAL:")){
            std::cout << "oh well for initial..." << std::endl;
            return;
        }
        std::stringstream ss(line);
        std::string token;
        std::getline(ss, token, ':');
        std::getline(ss, token, ';');
        make_initial(token);
        if (!std::getline(input_file, line))
            return;
        if (!line.starts_with("FINALS:")){
            std::cout << "oh well for finals..." << std::endl;
            return;
        }
        read_final_states(line);
    }
    std::string simulate(const std::vector<std::string> &word){
        std::string ret = initial_state;
        for (const auto &i : word){
            ret = *transition_func.at(ret)(i);
        }
        return ret;
    }
    bool is_valid_word(const std::vector<std::string> &word){
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
    //Issue is that you can't have DFA_NODE for unordered_maps, so need to order the strings and combine them into a single string.
    std::unordered_map<std::string, Func<std::string, DFA_NODE>> equiv_dfa_transition_func;
    void finally_fill_out_equiv_dfa(){
        equiv_dfa.alphabet = alphabet;
        equiv_dfa.nodes = {};
        for (const auto &node : equiv_dfa_nodes){
            std::string tmp = generate_string_from_DFA_node(node);
            equiv_dfa.nodes.push_back(tmp);
            Func<std::string, std::string> func_for_node(tmp);
            for (const auto &letter : alphabet){
                func_for_node.add_element_to_domain(letter, generate_string_from_DFA_node(*equiv_dfa_transition_func.at(tmp)(letter)));
            }
            equiv_dfa.transition_func[tmp] = std::move(func_for_node);
        }
        equiv_dfa.initial_state = generate_string_from_DFA_node(equiv_dfa_init_node);
        equiv_dfa.final_states = {};
        for (const auto &i : final_states_closure)
            equiv_dfa.final_states.push_back(i);
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
        std::unordered_set<std::string> buffer = DFA_node;
        for (const auto &i : DFA_node){
            auto tmp = transition_func.at(i)(input_symbol);
            if (tmp){
                for (const auto &ins : *tmp)
                    buffer.insert(ins);
            }
        }
        std::unordered_set<std::string> ret{};
        for (const auto &i : buffer){
            auto tmp = get_epsilon_closure(i);
            ret.insert(tmp.begin(), tmp.end());
        }
        return ret;
    }
    //node has to already be pushed onto equiv_dfa_nodes
    void dfa_add_transitions_for_node(const DFA_NODE &node){
        std::string equiv_string = generate_string_from_DFA_node(node);
        if (equiv_dfa_transition_func.contains(equiv_string))
            return;
        Func<std::string, DFA_NODE> transition_function{};
        for (const auto &letter : alphabet){
            auto tmp = find_reachable_set(node, letter);
            transition_function.add_element_to_domain(letter, tmp);
            if (!tmp.empty() && !std::ranges::contains(equiv_dfa_nodes, tmp)){
                equiv_dfa_nodes.push_back(tmp);
                dfa_add_transitions_for_node(tmp);
            }
        }
        equiv_dfa_transition_func[equiv_string] = std::move(transition_function);
    }
    std::string generate_string_from_DFA_node(const DFA_NODE &node){
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
public:
    NFA() = default;
    NFA(std::ifstream &input_file){
        read_NFA(input_file);
    }
    
    void dfa_generate_transitions(){
        DFA_NODE *node = &equiv_dfa_init_node;
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
            return;
        if (!line.starts_with("NODES:")){
            std::cout << "oh well..." << std::endl;
            return;
        }
        read_nodes(line);
        if (!std::getline(input_file, line))
            return;
        if (!line.starts_with("SYMBOLS:")){
            std::cout << "oh well for symbols..." << std::endl;
            return;
        }
        read_symbols(line);
        if (!std::getline(input_file, line))
            return;
        if (!line.starts_with("TRANSITIONS:")){
            std::cout << "oh well for transitions..." << std::endl;
            return;
        }
        read_transitions(line);
        if (!std::getline(input_file, line))
            return;
        if (!line.starts_with("INITIAL:")){
            std::cout << "oh well for initial..." << std::endl;
            return;
        }
        std::stringstream ss(line);
        std::string token;
        std::getline(ss, token, ':');
        std::getline(ss, token, ';');
        make_initial(token);
        if (!std::getline(input_file, line))
            return;
        if (!line.starts_with("FINALS:")){
            std::cout << "oh well for finals..." << std::endl;
            return;
        }
        read_final_states(line);
    }
    void convert_to_DFA(){
        fill_epsilon_closures();
        std::string dfa_init_string = generate_string_from_DFA_node(equiv_dfa_init_node);
        equiv_dfa_nodes = {equiv_dfa_init_node};
        dfa_add_transitions_for_node(equiv_dfa_init_node);
        finally_fill_out_equiv_dfa();
    }
    std::string simulate(const std::vector<std::string> &word){
        DFA_NODE ret = equiv_dfa_init_node;
        for (const auto &i : word){
            ret = *equiv_dfa_transition_func.at(generate_string_from_DFA_node(ret))(i);
        }
        return generate_string_from_DFA_node(ret);
    }
    bool is_valid_word(const std::vector<std::string> &word){
        DFA_NODE intermediate = equiv_dfa_init_node;
        for (const auto &i : word){
            auto tmp = equiv_dfa_transition_func.at(generate_string_from_DFA_node(intermediate))(i);
            if (tmp.has_value())
                intermediate = *tmp;
            else{
                return false;
            }
        }
        if (intermediate == final_states_closure)
            return true;
        return false;
    }
    ~NFA() = default;
};
