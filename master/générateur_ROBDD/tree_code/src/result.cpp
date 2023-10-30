#include "result.h"

#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <algorithm>
#include <fstream>
#include <cstring>


void result::add_result(const size_t nb_nodes) {
    std::unique_lock<std::mutex> l(lock);
    bool found = false;
    ++tree_count;
    
    for(auto &e : nodes_bf) {
        if(e.first == nb_nodes) { //found
            e.second += 1;
            found = true;
            break;
        }
    }

    if(!found) //not found
        nodes_bf.push_back(std::pair<nb_nodes_t, bool_c_t>(nb_nodes, 1));
}

void result::update_time(const size_t s, const long double ms) {
    std::unique_lock<std::mutex> l(lock);
    seconds += s;
    mil_sec += ms;
}

void result::sort() {
    std::unique_lock<std::mutex> l(lock);

    //sort per nodes
    std::sort(nodes_bf.begin(), nodes_bf.end(), [](std::pair<nb_nodes_t, bool_c_t> &p1, std::pair<nb_nodes_t, bool_c_t> &p2) {
        return p1.first < p2.first;
    });
}

std::ostream &operator<<(std::ostream &os, const result &res) {
    std::unique_lock<std::mutex> l(res.lock);
    size_t h = 0;
    size_t m = 0;
    size_t s = 0;
    
    s = res.seconds;
    m = s / 60;
    s %= 60;
    h = m / 60;
    m %= 60;
    
    os << sep << std::endl;
    os << "Number of variables: " << res.nb_var << std::endl;
    os << "Number of samples: " << res.tree_count << std::endl;
    os << "Number of unique sizes " << res.nodes_bf.size() << std::endl;
    os << "Total time: " << h << ":" << m << ":" << s << std::endl;
    os << "Secondes per ROBDD: " << ((res.mil_sec / 1000) / res.tree_count) << std::endl;

    
    for(auto &e : res.nodes_bf) {
        std::cout << "[" << e.second << "] bool functions have produced [" << e.first << "] nodes" << std::endl;
    }

    os << sep << std::endl;

    return os;
}

//print all
void show_terminal(const std::vector<result *> &tab) {
    for(auto *e : tab) {
        std::cout << *e;
    }
}

//clean
int clear_dir_result() {
    DIR *dirp;
    struct dirent *entry;
    std::string file_n = "";
    std::string path_reslt = PATH_RESULT;

    if((dirp = opendir(PATH_RESULT)) == NULL) {
        std::cerr << "error: result directory was not found" << std::endl;
        return -1;
    }
    
    while((entry = readdir(dirp))) {
        file_n = entry->d_name;

        if(file_n != "." && file_n != "..") {
            if(unlink((path_reslt + file_n).c_str()) < 0) {
                closedir(dirp);
                perror("unlink");
                return -1;
            }
        }
    }
    closedir(dirp);
    return 0;
}

int write_result_file_f11(const std::vector<result *> &res, std::vector<std::string> &files) {
    std::string file_n = "";
    std::string path_reslt = PATH_RESULT;
    std::string path_arg = PATH_RESULT_ARG;
    int len = res.size();
    size_t nb_samples = 0;
    size_t h = 0;
    size_t m = 0;
    size_t s = 0;

    file_n += "fig11.txt";
    std::ofstream ofs = std::ofstream((path_reslt + file_n), std::ios::trunc);

    if(!ofs.is_open()) {
        std::cerr << "error: result file was not created" << std::endl;
        return -1;
    }
    for(int i = 0; i < len; ++i) {
        s = res[i]->seconds;
        m = s / 60;
        s %= 60;
        h = m / 60;
        m %= 60;

        nb_samples = 0;
        for(auto &e : res[i]->nodes_bf) {
            nb_samples += e.second;
        }
        ofs << res[i]->nb_var << ";" << nb_samples << ";" << res[i]->nodes_bf.size() << ";" << h << ":" << m << ":" << s << ";" << ((res[i]->mil_sec/1000)/res[i]->tree_count);
        if(i < len-1)
            ofs << "#" << std::flush;
        else
            ofs << std::flush;
    }

    files.push_back((path_arg + file_n));
    return 0;
}

//setup files for python script
std::string result::write_result_file_f9_f10() {
    int len = nodes_bf.size();
    std::string file_n = "";
    std::string path_reslt = PATH_RESULT;
    std::string path_arg = PATH_RESULT_ARG;

    file_n += "var-";
    file_n += std::to_string(nb_var);
    file_n +=".txt";
    std::ofstream ofs = std::ofstream((path_reslt + file_n), std::ios::trunc);

    if(!ofs.is_open()) {
        std::cerr << "error: result file was not created" << std::endl;
        return "";
    }
        
    for(int i = 0; i < len; ++i) {
        ofs << nodes_bf[i].first << ";" << nodes_bf[i].second;
        if(i < len-1)
            ofs << "#" << std::flush;
        else
            ofs << std::flush;
    }
    ofs.close();
    
    return (path_arg + file_n);
}

int write_all_results(const::std::vector<result *> &tab, std::vector<std::string> &files, int figure, const bool clear) {
    int ret = 0;
    std::string f_name;
    if(figure != 9 && figure != 10 && figure != 11)
        return -1;

    if(clear)
        if((ret = clear_dir_result()) < 0)
            return ret;


    if(figure == 9 || figure == 10) {
        for(auto &e : tab) {
                if((f_name = e->write_result_file_f9_f10()) == "")
                    return -1;
                else
                    files.push_back(f_name);
        }
    }
    else {
        if(write_result_file_f11(tab, files) < 0)
                return -1;
    }
    return ret; //0
}