#pragma once

#include <iostream>
#include <vector>
#include <unordered_set>
#include <utility>
#include <string>

#define DEFAULT_PATH "./graphs/"


const std::string label = "label="; //front
const std::string style = "style=dashed"; //dashed or dotted - edge style
//const std::string splines = "splines=line"; //curve
const std::string front = " ["; //front
const std::string back = "]"; //back

//see below
struct hash_fun;
struct comp_fun;

class node {
    private:
        //no const because of operator= (but there are no setters and get_luka() return a copy std::string)
        node *left; //left child
        node *right; //right child
        size_t var; //is this node a leaf ?: 0 else var number
        bool th; //thruth value if leaf
        std::string luka; //luka word
        bool walk; //for walking throught the tree without counting multiple times the same node, ex: nbr of nodes (DAG, ROBDD)
                   //avoid to store the visited nodes adress in a table (sizeof(bool) < sizeof(node *))
                   //and possibly reduce the complexity (adding all nodes address in the table)
        bool del; //delete

    public:
        //node default constructor, defined here
        node(): left(nullptr) , right(nullptr), var(0), th(false), luka(""), walk(false), del(false) {}
        
        //copy constructor
        node(const node &n);

        //node constructor, defnied here
        node(const size_t v, const bool t, const bool w): left(nullptr), right(nullptr), var(v), th(t), luka(""), walk(w), del(false) {}
        
        //node destructor, defined in tree.c
        ~node();

        //operator=, copy
        node &operator=(const node &n);

        //operator== (@ == @)
        bool operator==(const node &n) const;

        //redefinition because calling delete on a DAG can call multiple times ~node() on the same node, that lead to segfault...
        //we will store all uniques nodes (adresses) into an std::set object and call ::operator delete on them
        void operator delete(void *ptr) noexcept;

        void *operator new(std::size_t size) noexcept;


        /* prototypes */
        //build Lukaziewice word
        //store every word if luka_root is false
        //else store only for root node (memory saving for large computation)
        //root_addr: addresse of root
        //return luka word on success and "" on failure
        std::string build_luka(const bool luka_root, const node *root_addr);

        //build the node structure
        //n is the node (for new), tt is the thruth table, nb_var the nbr of variables
        //w is used to init walk attribut
        //ixd is used as an index for the thruth table and i for the current tree depth (node var = nb_var-i)
        //return 0 on success and -1 if node or *node is nullptr
        friend int build_tree(node **n, const std::vector<bool> &tt, const size_t nb_var, size_t &idx, size_t i, const bool w);

        //return truth table by searching on the leafs
        //the "return" is on tt...
        void get_thruth_table(std::vector<bool> &tt) const;

        //copy src in dst
        //return 0 on success and -1 on failure
        friend int dup_node(node **dst, const node *src);

        //draw graph body
        //ofs: file, w: avoid to counting multiple times the same node
        void draw_graph(std::ofstream &ofs, const bool w);
        
        //map is a hash set (insert element only if the hashed value isn't present in the hash table)
        //contains lukasiewicz words as keys
        //NB: for this solution, we opted to use the root lukasiewicz word, and split it multiple time
        //to get sub words without rebuild them (split_luka())
        //applied RULES: TERMINAL RULE & MERGING RULE
        //return 0 on success and -1 on failure
        int to_dag(std::unordered_set<std::pair<std::string, node *>, struct hash_fun, struct comp_fun> &map, const std::string &word);

        //return the number of nodes / leafs
        //w: not count multiples time the same node
        size_t count(const bool w);

        //to_robdd():
        //applied rules: DELETION RULE
        //w: walk (may be changed as w != node.walk)
        //nodes unused are set in tab, left and right of each node must be set to nullptr, and then delete all theses nodes
        friend void to_robdd_aux(node *n, std::vector<node *> &tab, const bool w);
        void to_robdd(const bool w);
        

        /* getters */
        //return node's luka word
        //return "" if not actually set
        std::string get_luka() const;
};

class tree {
    private:
        //like node class, no const  because of operator=
        node *root; //first node
        size_t nb_var; //number of var (x1, ... xn)
        bool only_luka_root; //store luka word for all nodes or only for the root node if true
        bool which_walk; //switch between true/false - we can consider that every nodes have the same value because all nodes are reachable

    public:
        //tree default constructor, defined here;
        tree(): root(nullptr), nb_var(0), only_luka_root(true), which_walk(false) {}
        
        //tree constructor, defined in tree.c
        tree(const std::vector<bool> &tt, const size_t n, const bool l, const bool w);
        
        //copy constructor
        tree(const tree &t);

        //destructor, defined in tree.c
        ~tree();

        //operator=, copy
        tree &operator=(const tree &t);


        /* prototypes */
        //call build_luka_word()
        //return luka word on success and "" on failure
        std::string build_luka();

        //create/trunc a file corresponding to the graph to draw
        //set header and end on file
        //call draw_graph(name) node
        //return 0 on success, -1 if path file isn't correct, -2 if the .pdf wasn't generated / generated properly (error too)
        //or return -3 if KEEP_DOT macro is 0 and unlink() failed
        int draw_graph();

        //building DAG by calling to_dag() node
        //applied rules: TERMINAL RULE & MERGING RULE (cf ALGAV subject)
        //return 0 on success and -1 on failure
        int tree_to_dag();

        //number of nodes / leafs
        size_t count();

        //build ROBDD
        //this function will call DAG builder if action is true
        //more simple than applying again TERMINAL & MERGING RULE
        //applied rules: DELETION RULE
        //return: value from to_dag() (-1) on error, else 0
        int tree_to_robdd(const bool action);


        /* getters */
        //return luka word or "" ("" = default value)
        std::string get_luka() const;

};

//used for unordered_set<luka_word, node>
//by default, c++ don't provide an implementation that can directly hash std::pair
//we have to defined the hash function in this case
struct hash_fun {
    size_t operator()(const std::pair<std::string, node *> &p) const {
        const std::hash<std::string> hasher;
        return hasher(p.first);
    }
};

//used for unordered_set to do comparison
struct comp_fun {
    bool operator()(const std::pair<std::string, node *> &p1, const std::pair<std::string, node *> &p2) const {
        return (p1.first == p2.first);
    }
};


/* prototypes */
int build_tree(node **n, const std::vector<bool> &tt, const size_t nb_var, size_t &idx, size_t i, const bool w);

int dup_node(node **dst, const node *src);

void to_robdd_aux(node *n, std::vector<node *> &tab, const bool w);

//split into two part (...) (...) lukasiewicz word
//return 0 on success and -1 on failure
// /!\ word and l must be differents, else an out_of_range exception will occur
int split_luka(const std::string &word, std::string &l, std::string &r);