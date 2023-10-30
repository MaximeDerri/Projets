#include "tree.h"

#include <iostream>
#include <stdexcept>
#include <queue>
#include <fstream>
#include <sstream>
#include <ctime>
#include <cstdlib>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


//pow for size_t, not defined in tree.h
size_t pow_size_t(size_t n, size_t p) {
    size_t res = 1;
    for(size_t i = p; i > 0; --i) {
        res *= n;
    }
    return res;
}

//split luka word
int split_luka(const std::string &word, std::string &l, std::string &r) {
    size_t a = 0, b = 0;
    size_t i = 0;
    size_t len = word.size();
    size_t cpt = 1;

    if(word[len-1] != ')') //need to end with ')'
        return -1;

    for(; word[i] != '(' && i < len; ++i); //searching first '('
    if(i == len)
        return -1;

    a = i;
    ++i;
    for(; cpt != 0 && i < len; ++i) { //searching (...) right side of x(...)(...)
        if(word[i] == '(')
            ++cpt;
        if(word[i] == ')')
            --cpt;
    }
    if(i == len)
        return -1;
    
    b = i;
    if(i == len || word[i] != '(') //the left side (...) is checked
        return -1;

    //searching the number of '(' and ')' on the left side
    ++i;
    cpt = 1;
    for(; cpt != 0 && i < len; ++i) {
        if(word[i] == '(')
            ++cpt;
        if(word[i] == ')')
            --cpt;
    }
    if(i != len || cpt != 0)
        return -1;

    //ok! setting l and r
    l = word.substr(a+1, b-a-2);
    r = word.substr(b+1, len-b-2);

    return 0;
}



/* NODE */

//copy constructor
node::node(const node &n):
left(nullptr), right(nullptr), var(n.var), th(n.th), luka(n.luka), walk(n.walk), del(false) { //check if this == n
    if(n.left != nullptr)
        dup_node(&left, n.left);
    if(n.right != nullptr)
        dup_node(&right, n.right);
}

//node destructor
node::~node() {}

//redefinition to avoid segfault by deleting multiple times the same node
void node::operator delete(void *ptr) noexcept {
    if(ptr == nullptr)
        return;
    
    std::vector<node *> map = std::vector<node *>();
    map.reserve(10);
    
    std::queue<node *> fifo = std::queue<node *>();
    node *cur;
    
    //redefinition for node class -> ptr <=> node *
    fifo.push((node *)ptr);

    //BFS
    while(!fifo.empty()) {
        cur = fifo.front();
        fifo.pop();
        if(cur->left != nullptr)
            fifo.push(cur->left);
        if(cur->right != nullptr)
            fifo.push(cur->right);
        if(!cur->del) {
            cur->del = true;
            map.push_back(cur);
        }
    }

    //delete nodes
    for(auto it = map.begin(), _end = map.end(); it != _end; ++it) {
        cur = *it;
        ::operator delete (cur); //will call ~node()
    }
}

void *node::operator new(std::size_t size) noexcept {
    return ::operator new(size);
}



//duplicate nodes structure
int dup_node(node **dst, const node *src) {
    int ret = 0;
    if(src == nullptr)
        return -1;
    
    *dst = ::new node(src->var, src->th, src->walk);
    (*dst)->luka = src->luka;

    //left side
    if(src->left != nullptr)
        if(dup_node(&((*dst)->left), src->left) < 0)
            return -1;

    //right side
    if(src->right != nullptr)
        return dup_node(&((*dst)->right), src->right);
    
    return ret;
}

//node operator= => copy
node &node::operator=(const node &n) {
    if(this == &n) //same node ?
        return *this; //no action
    
    //else
    //let's free right and left 
    delete left;
    delete right;
    
    //let's copy
    var = n.var;
    th = n.th;
    luka = n.luka;
    walk = n.walk;

    //copy of right and left
    if(dup_node(&left, n.left) < 0) {
        throw std::invalid_argument("error from operator= in class node during processing dup_node()");
    }
    if(dup_node(&right, n.right) < 0) {
        throw std::invalid_argument("error from operator= in class node during processing dup_node()");
    }
    
    return *this;
}

//node operator== (@ == @)
bool node::operator==(const node &n) const {
    if(this == &n)
        return true;
    else
        return false;
}

//used by tree constructor, not in .h
//n is the current node, idx is used for thruth table, i = current depth, w is walk value (the same for every nodes)
//tt is the thruth table, already checked by tree constructor, nb_var is the number of var => max depth
int build_tree(node **n, const std::vector<bool> &tt, const size_t nb_var, size_t &idx, size_t i, const bool w) {
    if(n == nullptr)
        return -1;
    if(nb_var == i) { //leaf
        *n = ::new node(0, tt[idx], w);
        ++idx; //next thruth table entry
        --i; //return to the previous node / exit function
        return 0;
    }
    else { //node
        *n = ::new node((nb_var-i), false, w);
        ++i;
    }
    
    if(build_tree(&((*n)->left), tt, nb_var, idx, i, w) < 0)
        return -1;

    return build_tree(&((*n)->right), tt, nb_var, idx, i, w);
}

//construction of luka word
std::string node::build_luka(const bool luka_root, const node *root_addr) {
    std::string s;
    
    //leaf ?
    if(left == nullptr && right == nullptr) {
        if(th)
            s = "T";
        else
            s ="F";

        if(this == root_addr)
            luka = s;
        return s;
    }

    //node
    //nullptr should never happened
    //left
    if(left == nullptr) {
        s += "X";
        s += std::to_string(var);
        s += "()";
    }
    else {
        s += "X";
        s+= std::to_string(var);
        s+= "(";
        s += left->build_luka(luka_root, root_addr);
        s += ")";
    }

    //right
    if(right == nullptr)
        s += "()";
    else {
        s+= "(";
        s += right->build_luka(luka_root, root_addr);
        s += ")";
    }
    
    //store luka word ?
    if(!luka_root) //luka_root = false => store on every nodes
        luka = s;
    else { //true => only root
        if(this == root_addr)
            luka = s;
    }
    
    return s;
}

//return the truth table by taking bool from leafs
void node::get_thruth_table(std::vector<bool> &tt) const {
    if(left == nullptr && right == nullptr) { //leaf, store bool on tt
        tt.push_back(th);
        return;
    }

    if(left != nullptr) {
        left->get_thruth_table(tt);
    }

    if(right != nullptr) {
        right->get_thruth_table(tt);
    }
}

//applied RULES: TERMINAL RULE & MERGING RULE
//prefix = {x, prefix(L), prefix(R)}
int node::to_dag(std::unordered_set<std::pair<std::string, node *>, struct hash_fun, struct comp_fun> &map, const std::string &word) {
    std::string l, r;
    auto tmp = std::pair<std::string, node *>();

    if(left == nullptr || right == nullptr)
        return 0;

    if(split_luka(word, l, r) < 0)
        return -1;

    //left
    tmp.first = l;
    tmp.second = left;
    auto f = map.insert(tmp);
    if(f.second) { //insertion
        left->to_dag(map, l);
    }
    else { //a similar node exist
        if(left != f.first->second) { 
            delete left;
            left = f.first->second;
        }
    }

    //right
    tmp.first = r;
    tmp.second = right;
    f = map.insert(tmp);
    if(f.second) {
        right->to_dag(map, r);
    }
    else {
        if(right != f.first->second) {
            delete right;
            right = f.first->second;
        }
    }
    return 0;
}

//DELETION RULE
//we need to keep node "deleted" and delete them at the end:
//if it is referenced by others node and we delete it, they will point on nothing...
//we will store them on a std::vector, in case that they are not marked (walk)
void to_robdd_aux(node *n, std::vector<node *> &tab, const bool w) {
    if(n->left == nullptr || n->right == nullptr)
        return;

    if(n->left->left == nullptr || n->left->right == nullptr)
        return;
    else
        to_robdd_aux(n->left, tab, w);
        
    if(n->right->left == nullptr || n->right->right == nullptr)
        return;
    else
        to_robdd_aux(n->right, tab, w);


    if(n->left->left == n->left->right) {
        if(n->left->walk != w) {
            n->left->walk = w;
            tab.push_back(n->left);
        }
        n->left = n->left->left;
    }

    if(n->right->left == n->right->right) {
        if(n->right->walk != w) {
            n->right->walk = w;
            tab.push_back(n->right);
        }
        n->right = n->right->right;
    }
}

void node::to_robdd(const bool w) {
    if(left == nullptr && right == nullptr)
        return;

    std::vector<node *> tab;
    to_robdd_aux(this, tab, w);
    node *r = nullptr;

   
    if(left == right) {
        var = left->var;
        th = left->th;
        luka = "";

        if(left->left != nullptr) {
            r = left;
            left = left->left;
            r->left = nullptr;
        }

        if(right->right != nullptr) {
            r = right;
            right = right->right;
            r->right = nullptr;
        }
        
        if(r == nullptr) {
            delete left;
            left = nullptr;
            right = nullptr;
        }
        else
            delete r;
    }
    
    //delete
    for(node *n : tab) {
        n->left = nullptr;
        n->right = nullptr;
        delete n;
    }
    tab.clear();
}

//write in ofs body graph
void node::draw_graph(std::ofstream &ofs, const bool w) {
    if(walk == w)
        return;
    else
        walk = w;

    std::string name;
    std::ostringstream addr;
    std::ostringstream addr_l;
    std::ostringstream addr_r;

    //leaf
    if(left == nullptr && right == nullptr) {
        if(th)
            name = "True";
        else
            name = "False";
    }
    else //node
        name = std::string("x") + std::to_string(var);

    //label current node
    addr << (void const *)this;
    ofs << strtoul(addr.str().c_str(), NULL, 16)  << front << label << "\"" << name << "\"" << back << std::endl; //node label

    //left
    if(left != nullptr) {
        addr_l << (const void *)left;
        ofs << strtoul(addr.str().c_str(), NULL, 16)  << " -- ";
        ofs << strtoul(addr_l.str().c_str(), NULL, 16) << front << style <<  back << std::endl;
        left->draw_graph(ofs, w);
    }

    //right
    if(right != nullptr) {
        addr_r << (void const *)right;
        ofs << strtoul(addr.str().c_str(), NULL, 16) << " -- ";
        addr << (void const *)right;
        ofs << strtoul(addr_r.str().c_str(), NULL, 16) << std::endl;
        right->draw_graph(ofs, w);
    }

}

size_t node::count(const bool w) {
    if(walk == w)
        return 0;
    walk = w;
    size_t c = 1;

    if(left != nullptr)
        c += left->count(w);
    if(right != nullptr)
        c += right->count(w);
    return c;
}

//return luka word of current node
std::string node::get_luka() const {
    return luka;
}

/* ******************** */



/* TREE */

//tree constructor
tree::tree(const std::vector<bool> &tt, const size_t n, const bool l, const bool w):
root(nullptr), nb_var(n), only_luka_root(l), which_walk(w) {
    if(tt.size() != pow_size_t(2, n)) //checking the size of the thruth table, according to the nbr of leafs
        throw std::invalid_argument("invalid size of thruth table, the size must be 2^nb_var");

    size_t idx = 0;
    if(build_tree(&root, tt, nb_var, idx, 0, w) < 0) //should never happened
        throw std::invalid_argument("nullptr exception");
}

//tree copy constructor
tree::tree(const tree &t):
root(nullptr), nb_var(t.nb_var), only_luka_root(t.only_luka_root), which_walk(t.which_walk){
    //build new tree and verify if an error occurs
    if(dup_node(&root, t.root) < 0)
        throw std::invalid_argument("invalid src node, is source tree root nullptr ?");
}

tree &tree::operator=(const tree &t) {
    if(this == &t) //same tree ?
        return *this;
        
    //copy
    nb_var = t.nb_var;
    only_luka_root = t.only_luka_root;
    which_walk = t.which_walk;

    //delete
    delete root;

    //copy root
    root = t.root; //calling node operator=
    return *this;
}

//tree destructor
tree::~tree() {
    delete root;
}

//launch luka word build by a call on tree obj
std::string tree::build_luka() {
    return root->build_luka(only_luka_root, root);
}

//drawing graph
int tree::draw_graph() {
    std::string base_name = DEFAULT_PATH; //abs path
    std::string dot;
    std::string pdf;
    std::string sep1 = "-";
    std::string sep2 = "_";
    time_t now = time(NULL);
    struct tm *lt = localtime(&now);

#ifdef PATH_GRAPH //if path is defined in Makefile
    base_name = std::string(PATH_GRAPH);
#endif

    //name of file
    base_name += "var";
    base_name +=  std::to_string(nb_var);
    base_name += sep1 + std::to_string(lt->tm_mday) + sep2 + std::to_string(lt->tm_mon+1) + sep2 + std::to_string(lt->tm_year+1900);
    base_name += sep1 + std::to_string(lt->tm_hour) + sep2 + std::to_string(lt->tm_min) + sep2 + std::to_string(lt->tm_sec);
    dot = base_name + ".dot";
    pdf = base_name + ".pdf";

    std::ofstream ofs = std::ofstream(dot, std::ios::trunc);
    if(!ofs.is_open())
        return -1;

    ofs << "graph {" << std::endl; //head
    which_walk = !which_walk;
    root->draw_graph(ofs, which_walk);
    ofs << "}" << std::endl; //end
    ofs.close();

    //.dot -> .pdf
    int status;
    switch (fork()) {
    case -1:
        perror("fork");
        return -2;

    case 0: //child
        execl("/bin/dot", "dot", "-Tpdf", dot.c_str(), "-o", pdf.c_str(), (char *)NULL);
        ofs.close(); //execl failed 
        exit(1);

    default: 
        while(true) { //if a signal is received
            if(wait(&status) < 0)
                return -2;

            if(WIFEXITED(status)) {
                if(WEXITSTATUS(status) != 0) //exited on error ?
                    return -2;
                break;
            }
        }
    }

//delete .dot
#ifdef KEEP_DOT
    #if KEEP_DOT == 0
        if(unlink(dot.c_str()) < 0)
            return -3;
    #endif
#endif

    return 0;
}

//bdd to dag
int tree::tree_to_dag() {
    std::unordered_set<std::pair<std::string, node *>, struct hash_fun, struct comp_fun> map;
    size_t size_per_list = 5; //can be modified in Makefile. Define an approximate number of node per list in the hash table, if it's possible (deppend on the number of nodes)
    size_t n = 1;
    int len = 0;
    std::string word = root->get_luka();
    if(word == "") {
        if((word = this->build_luka()) == "") //trying to set word if it is not
            return -1;
    }

    //if nb_var >= 64, tmp = 64     //trunc
    //else, tmp = nb_var+1
    //len = 2^tmp -1/ HT_LIST_SIZE
    //size_t = unsigned long => max possible not signed value on 32 or 64 bits.

#ifdef HT_LIST_SIZE
    #if HT_LIST_SIZE > 0
        size_per_list = HT_LIST_SIZE;
    #endif
#endif
    
    //trunc
    if(nb_var >= 64)
        len = 64;
    else
        len = nb_var+1;

    n = pow_size_t(2, len-1); //-1 to avoid overflow / 2^len -1
    n += (n-1);
    n /= size_per_list;
    
    map = std::unordered_set<std::pair<std::string, node *>, struct hash_fun, struct comp_fun>(n);
    return root->to_dag(map, word);
} 

int tree::tree_to_robdd(const bool action) {
    int ret = 0;
    if(action) {
        ret = tree_to_dag();
        if(ret != 0)
            return ret;
    }

    //which_walk = !which_walk;
    root->to_robdd(!which_walk);

    return 0;
}

size_t tree::count() {
    which_walk = !which_walk;
    return root->count(which_walk);
}

//return luka word of root
std::string tree::get_luka() const {
    return root->get_luka();
}
