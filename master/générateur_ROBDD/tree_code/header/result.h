#pragma once

#include <iostream>
#include <iosfwd>
#include <vector>
#include <utility>
#include <chrono>
#include <mutex>

//file name syntax: var-nb_nodes-bool_f
#define PATH_RESULT "./graph_code/src/.result/"
#define PATH_RESULT_ARG "./result/" // arg for python script

typedef size_t bool_c_t;
typedef size_t nb_nodes_t;

const std::string sep = "--------------------";


//store result from computation
class result {
    private:
        //count the total computation time
        mutable std::mutex lock;
        size_t nb_var;
        size_t seconds;
        long double mil_sec; //we will not take in account millisecondes for the total time, only for the time per nodes (else one computation can be 0 sec...)
        //nb of generated trees
        //if the user want to create more that max of size_t possible value, we will truncate because it will begin to be very big... it happend for big var choice
        size_t tree_count; //number of sample and used to have secondes per ROBDD
        //ex: var 5: we have for 10 bool function 5 differents tree that give the same node count, etc...
        std::vector <std::pair<nb_nodes_t, bool_c_t>> nodes_bf; //permite to have the number of unique sizes

        public:
            //default constructor
            result(): nb_var(0), seconds(0), mil_sec(0), tree_count(0) {}
            
            //nb: variables
            result(size_t nb): nb_var(nb), seconds(0), mil_sec(0), tree_count(0) {}

            ~result() {}
            
            //operator<<, print every informations (-e)
            friend std::ostream &operator<<(std::ostream &os, const result &res);

            //show result tab on terminal
            friend void show_terminal(const std::vector<result *> &tab) ;


            /* prototypes */
            //update attributes to take in account a new result
            void add_result(const size_t nb_nodes);

            //update time
            void update_time(const size_t s, const long double ms);

            //sort nodes_bf per nodes
            void sort();

            //clear graph_code/.result/ and create new result files f9/f10
            //var-nb_node-bool_f
            //return file name on success or "" in error
            std::string write_result_file_f9_f10();

            //same, but push file names directly on files
            //return 0 on success and -1 on error
            friend int write_result_file_f11(const std::vector<result *> &res, std::vector<std::string> &files);


            /* getters / setters */


};

std::ostream &operator<<(std::ostream &os, const result &res);

//print all results in terminal
void show_terminal(const std::vector<result *> &tab);

//clear .result directory using:
//opendir, readdir, closedir and unlink
//we don't check (with stat.h) mode / file type because .result is generated by Makefile only for giving to python scripts some datas to show
//return 0 on success, -1 if .result doesn't exit or if unlink() failed
int clear_dir_result();

//call write_results_files() for each entry
//clear = true => call clear_dir_result()
//return 0 on success and -1 on error
int write_all_results(const::std::vector<result *> &tab, std::vector<std::string> &files, int figure, const bool clear);