#include <iostream>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <climits>
#include <unordered_set>
#include <filesystem>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "tree.h"
#include "result.h"
#include "job.h"
#include "pool.h"
#include "queue.h"
#include "BigInt.hpp"

#define INIT_POOL 10000
#define DEFLT_LEN 20
#define PYTHON "/usr/bin/python3"
#define SCRIPT_PATH "/graph_code/src/"
#define SCRIPT_F9_F10 "codeFigure.py"
#define SCRIPT_F11 "fig11.py"


int compute_one(size_t var, BigInt &value, bool draw, result &res);

const std::string help = "\n\
-   -h         -> aide\n\
\n\
-   -n         -> affiche un BigInt (longueur defini dans le Makefile)\n\
\n\
-   -e [-g] N V  -> lance l\'etude de l'arbre unique representé par N sur V variables\n\
                 -> affichage de l\'etude dans le terminal\n\
                 -> -g pour tracer le graph\n\
\n\
-   -E < -9 | -10 | -11 > V1-V2  -> lance l\'etude complète pour entre V1 et V2 variables\n\
                                 -> figure  9: tous les cas\n\
                                 -> figure 10: extrapolation\n\
                                 -> figure 11: statistiques\n";

/* job class */
class job_tree : public job {
    private:
        size_t var;
        BigInt value;
        bool draw;
        result *res;

    public:
        job_tree(size_t v, BigInt val, bool d, result *r): var(v), value(val), draw(d), res(r) {}
        
        ~job_tree() {}
        
        void run() {
            if(compute_one(var, value, draw, *res) < 0) {
                std::cerr << "error from compute_one()";
            }
        }
        
};
/* *** */


//square and multiply
void fast_pow(const BigInt &n, BigInt &res) {
    if(n == 1)
        return;

    if((n % 2) == 0) {
        res *= res;
        fast_pow(n/2, res);
    }
    else {
        BigInt tmp = res;
        res *= res;
        fast_pow((n-1)/2, res);
        res *= tmp;

    }
}

//calculate max value - 2^2^n
void get_max_value(const size_t nb_var, BigInt &res) {
    BigInt tmp = 2;
    res = 2;
    fast_pow(nb_var, tmp);
    fast_pow(tmp, res);
}

//convert n_10 to n_2 => vector<bool>
void to_binary(std::vector<bool> &res, BigInt n) {
    while(n != 0) {
        if((n % 2) == 0)
            res.push_back(false);
        else
            res.push_back(true);
        n /= 2;
    }
}

//resize vector<bool> by removing elements or adding 'false' at the end
void resize(std::vector<bool> &res, const BigInt &size) {
    size_t v_size = res.size();
    if(v_size > size) { //reduce
        while(v_size > size) {
            res.pop_back();
            --v_size;
        }
    }
    else {
        while(v_size < size) {
            res.push_back(false);
            ++v_size;
        }
    }
}

//x: integer, l: size of vector
void get_thruth_table(std::vector<bool> &tab, const BigInt &x, const BigInt &l) {
    to_binary(tab, x);
    resize(tab, l);
}

bool in_range(const char *s, const char a, const char b) { //for our case, we consider number starting at 1 (no 0-var trees...)
    for(int i = 0; s[i] != '\0'; ++i) {
        if(s[i] < a || s[i] > b)
            return false;
    }
    return true;
}

//return 0 on success and -1 on error
int compute_one(size_t var, BigInt &value, bool draw, result &res) {
    std::vector<bool> b;
    get_thruth_table(b, value, pow("2", var)); //1 bool/leaf

    tree t = tree(b, var, true, true);
    if(t.tree_to_robdd(true) < 0) {
        return -1;
    }

    res.add_result(t.count());

    //draw graph ?
    if(draw) {
        int ret = t.draw_graph();
        if(ret == -1)
            std::cerr << "error: path file is not correct" << std::endl;
        if(ret == -2)
            std::cerr << "error: pdf file was not generated" << std::endl;
        if(ret == -3)
            std::cerr << "error: unlink() failed" << std::endl;
        return ret;
    }

    return 0;
}

//compute every possibles trees for var between min and max
int compute_mult_f9(size_t min, size_t max, std::vector<result *> &res) {
    long double ms = 0; //seconds
    size_t s = 0; //ms
    int nb_threads = 0;
    int init_p = INIT_POOL; //queue len
    int ret = 0;
    BigInt value = 0;
    BigInt repeat_build = 0;

/* configuration */
/*
 * NBTHREAD
*/
//mono thread or multithreads
#ifdef NBTHREAD
    //thread mode
    #if NBTHREAD > 1
        nb_threads = NBTHREAD;
    #endif
#endif

    for(size_t i = min; i <= max; ++i) { //iter over each vars
        result *res_tmp = new result(i);
        res.push_back(res_tmp);
        
        //max value is 2^2^n
        get_max_value(i, repeat_build);

        //starting threads
        if(nb_threads <= 0)
            init_p = 0; //change default value (INIT_POOL) by 0: no alloc

        //if macro is not set or is less than 2, nb_threads = 0 and no threads will be created
        pool p = pool(init_p);
        p.start(nb_threads);

        BigInt val = 0;
        //begin timer
        auto begin = std::chrono::steady_clock::now();
        //begin
        for(; val < repeat_build; ++val) { // <   ==>   -1
            if(nb_threads > 1) {
                job_tree *j = new job_tree(i, val, false, res_tmp);
                p.submit(j);
            }
            else {
                if(compute_one(i, val, false, *res_tmp) < 0) {
                    ret = -1;
                    std::cerr << "error from compute_one()" << std::endl;
                }
            }
        }
        //waiting threads
        if(nb_threads > 1)
            p.stop();

        //end
        auto end = std::chrono::steady_clock::now();
        auto dif = end - begin;
        ms = std::chrono::duration<double, std::milli>(dif).count();
        s = (size_t)(ms / 1000);

        //update time and push res entry
        res_tmp->update_time(s, ms);

    }
    return ret;
}


//we cut this function in two different parts because it was too long and complicated to read
int compute_mult_f10_f11(size_t min, size_t max, std::vector<result *> &res) {
    size_t range1 = 0;
    size_t range2 = 0;
    size_t repeat_build = 0;
    size_t size_per_list = 5; //default -> to set in Makefile
    int nb_threads = 0;
    int ret = 0;
    int init_p = INIT_POOL;
    int len = DEFLT_LEN; //INT_LENGTH in Makefile
    BigInt value = 0;
    BigInt limit_val = 0;
    //long long limit = -1; //avoid to recalculate multiple times
    bool auth_mult = false;
    
    //seed
    std::srand(std::time(nullptr));

/*** config ***/
#if defined(MIN) && defined(MAX)
    range1 = MIN;
    range2 = MAX;
#endif

#ifdef HT_LIST_SIZE
    #if HT_LIST_SIZE > 0
        size_per_list = HT_LIST_SIZE;
    #endif
#endif

#ifdef NBTHREAD //mono thread or multithreads
    //thread mode
    #if NBTHREAD > 1
        nb_threads = NBTHREAD;
    #endif
#endif

#ifdef AUTH_MULT
    #if AUTH_MULT != 0
        auth_mult = true;
    #endif
#endif

#ifdef INT_LENGTH
    #if INT_LENGTH <= 1000 && INT_LENGTH > 0
        len = INT_LENGTH;
    #endif
#endif

    if(range1 == 0 || range2 == 0 || range1 > range2) {
        std::cerr << "macro MIN and MAX are not correctly setup ==> using min=25000 and max=50000" << std::endl;
        range1 = 25; //default | see Makefile
        range2 = 50; //default |
    }
    
/*** end config ***/

    //main function part
    for(size_t i = min; i <= max; ++i) { //for each var
        size_t s = 0; //seconds
        long double ms = 0; //ms
        result *res_tmp = new result(i);
        res.push_back(res_tmp);

        //nb of trees
        if(range1 == range2)
            repeat_build = range1;
        else {
            repeat_build = 1 + range1 + rand() % (range2 - range1);
            if(!auth_mult) //multiples values accepted ?
                s = repeat_build; //not accepted
        }

        get_max_value(i, limit_val);

        std::unordered_set<std::string> set = std::unordered_set<std::string>(s / size_per_list);
        s = 0; //reset for seconds

        //starting threads
        if(nb_threads <= 0)
            init_p = 0; //change default value (INIT_POOL) by 0: no alloc

        //if macro is not set or is less than 2, nb_threads = 0 and no threads will be created
        pool p = pool(init_p);
        p.start(nb_threads);

        //begin
        auto begin = std::chrono::steady_clock::now();

        for(size_t j = 0; j < repeat_build; ++j) {
            do { //value
                //roll
                value = big_random(len) % limit_val;

                if(!auth_mult) { //we have to check if value needs to be reroll

                    //try to insert + check
                    auto tmp = set.insert(value.to_string());
                    if(tmp.second) //inserted
                        break;
                    //else: another round !
                }
                else { //nop
                    break;
                }

            } while(1); //end value roll

            //multi thread
            if(nb_threads > 1) {
                job_tree *j = new job_tree(i, value, false, res_tmp);
                p.submit(j); //push job
            }
            else { // mono
                if(compute_one(i, value, false, *res_tmp) < 0) {
                    ret = -1;
                    std::cerr << "error from compute_one()" << std::endl;
                }
            }
        } //i var: done

        //wait if multi thread
        if(nb_threads > 1)
            p.stop();

        //end
        auto end = std::chrono::steady_clock::now();
        auto dif = end - begin;
        ms = std::chrono::duration<double, std::milli>(dif).count();
        s = (size_t)(ms / 1000);

        //update time and push res entry
        res_tmp->update_time(s, ms);
    } //loop min-max


    return ret;
}

//launch python scripte to show resultsstd::filesystem::current_path(). << "|" << SCRIPT_PATHstd::filesystem::current_path(). << "|" << SCRIPT_PATH
//return 0 on succes and -1 on error
int launch_script(std::vector<std::string> &files, int figure) {
    std::string py = PYTHON;
    std::string script = std::filesystem::current_path();
    script += SCRIPT_PATH;

#ifdef PY_PATH
    py = PY_PATH;
#endif
    
    if(figure == 9 || figure == 10)
        script += SCRIPT_F9_F10;
    else if(figure == 11)
        script += SCRIPT_F11;
    else
        return -1;

    int status = 0;
    switch(fork()) {
        case -1:
            perror("fork");
            return -1;
        case 0: //child
            //exec
            execl(py.c_str(), py.c_str(), script.c_str(), (char *)NULL);
            //execvp failed
            perror("execlp");
            exit(EXIT_FAILURE);
        
        default:
            while(true) {
                if(wait(&status) < 0)
                    return -1;
                
                if(WIFEXITED(status)) {
                    if(WEXITSTATUS(status) != 0)
                        return -1;
                    else
                        break;
                }
            }
    }

    return 0;
}

//filter args and execute
int execute_com(const int argc, const char *argv[]) {
    if(argc < 2) {
        std::cerr << help << std::endl;
        return -1;
    }
    
    //NB: size_t == unsigned long
    size_t min = 0; //var | range (build trees with min <= n <= max nodes n)
    size_t max = 0; //var |
    int fig = 0; //figure
    int ret = 0;
    int len = DEFLT_LEN; //for -p
    BigInt val = -1; //value (-e)
    std::vector<std::string> files;
    std::string opt = argv[1]; //opt: -g, -e, -E
    bool draw = false; //drawing tree (-e)
    bool mode = false;
    
    /*** looking args ***/
    //help case
    if(argc == 2 && opt == "-p") {
        std::cout << help << std::endl;
    }

    //printing BigInt
    else if(argc == 2 && opt == "-n") {

#ifdef INT_LENGTH
    #if INT_LENGTH <= 1000 && INT_LENGTH > 0
        len = INT_LENGTH;
    #endif
#endif 

        std::cout << sep << std::endl << std::endl << big_random(len) << std::endl << std::endl << sep << std::endl;
    }

    //tree case
    else if((argc == 4 || argc == 5) && opt == "-e") { //simple case, one tree
        //setting param
        if(argc == 5 && std::string(argv[2]) == "-g") {
            draw = true;
            //N
            if(!in_range(argv[3], '0', '9')) { //checking integer
                std::cerr << "N must be at least 0" << std::endl;
                return -1;
            }
            val = argv[3];

            //V
            if(!in_range(argv[4], '0', '9')) {
                std::cerr << "V must be at least 0" << std::endl;
                return -1;
            }
            min = std::stoul(argv[4]);
        }
        else {
            //N
            if(!in_range(argv[2], '0', '9')) { //checking integer
                std::cerr << "N must be at least 0" << std::endl;
                return -1;
            }
            val = argv[2];

            //V
            if(!in_range(argv[3], '0', '9')) {
                std::cerr << "V must be at least 0" << std::endl;
                return -1;
            }
            min = std::stoul(argv[3]);
        }

        /***** EXEC *****/
        size_t s = 0;
        long double ms = 0;
        result res = result(min);

        //begin
        auto begin = std::chrono::steady_clock::now();
        if((ret = compute_one(min, val, draw, res)) < 0) {
            std::cerr << "error from compute_one()" << std::endl;
            return ret;
        }
        //end
        auto end = std::chrono::steady_clock::now();

        auto dif = end - begin;
        ms = std::chrono::duration<double, std::milli>(dif).count();
        s = (size_t)(ms / 1000);

        res.update_time(s, ms);

        res.sort();
        std::cout << res << std::endl;
        return ret;

    }
    //figure 9 | 10 | 11
    else if(argc == 4 && opt == "-E") {
        //figure
        //checking which figure to draw
        fig = strlen(argv[2]);
        if(fig == 2 || fig == 3) {
            //checking is the integer is correct
            if(!in_range(argv[2]+1, '0', '9')) {
                std::cout << "bad figure integer" << std::endl;
                return -1;
            }
            fig = (int)strtol(argv[2]+1, NULL, 10); //+1 for skipping '-'
            if(fig != 9 && fig != 10 && fig != 11) {
                std::cerr << "you need to select: -9 or -10 or -11" << std::endl;
                return -1;
            }
        }
        else {
            std::cerr << "bad figure choice" << std::endl;
            return -1;
        }

        //setting MIN & MAX V: fig 9 or 10 or 11
        std::string range = argv[3];
        std::string v1, v2;
        size_t idx = range.find("-", 0);
        //checking delimiter
        if(idx == std::string::npos || idx == 0 || idx == range.length()-1) { // V || -V || V-
        std::cerr << "delimiter not found" << std::endl;
            return -1;
        }
            
        //extract string at the left and right of '-'
        v1 = range.substr(0UL, idx);
        v2 = range.substr(idx+1, range.length()-1);

        if(!in_range(v1.c_str(), '0', '9') || !in_range(v2.c_str(), '0', '9')) {
            std::cerr << "V1 or V2 is not a correct integer" << std::endl;
            return -1;
        }
        min = std::stoul(v1);
        max = std::stoul(v2);
        if(min > max) {
            std::cerr << "V1 > V2, reverse order" << std::endl;
            return -1;
        }

        /***** EXEC *****/
        std::vector<result *> tab;
        if(fig == 9) {
            if((ret = compute_mult_f9(min, max, tab)) < 0) {
                return ret;
            }
        }
        else if(fig == 10 || fig == 11) {
            if((ret = compute_mult_f10_f11(min, max, tab)) < 0) {
                return ret;
            }
        }
        else { //should never happened
            std::cerr << "error: this figure choice is impossible..." << std::endl;
            return -1;
        }
        
//show mode
#ifdef SHOW_MODE
    #if SHOW_MODE != 0
        mode = true;
    #endif
#endif
        for(auto *e : tab) { //sort each result
            e->sort();
        }

        if(mode) { //python script
            write_all_results(tab, files, fig, true);
            if(launch_script(files, fig) < 0)
                return -1;
        }
        else { //terminal
            show_terminal(tab);
        }

        //clear allocated entries
        for(auto *e : tab) {
            delete e;
        }

            
    }
    //no match
    else {
        std::cerr << help << std::endl;
        return -1;
    }

    return 0;
}


int main(int argc, const char *argv[]) {
    if(execute_com(argc, argv) < 0) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}