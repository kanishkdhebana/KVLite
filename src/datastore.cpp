#include "datastore.h"

std::map<std::string, std::string> g_data;

void doRequest(
    const std::vector<std::string>& cmd, 
    Response& response
) {
    if (cmd.size() == 2 && cmd[0] == "get") {
        auto it = g_data.find(cmd[1]) ;
        
        if (it == g_data.end()) {
            response.status = RES_NX ;
            return ;
        } 
        
        else {
            const std::string& value = it -> second ;
            response.data.assign(value.begin(), value.end()) ;
        }
    } 
    
    else if (cmd.size() == 3 && cmd[0] == "set") {
        g_data[cmd[1]] = cmd[2] ;
        response.status = RES_OK ;
    } 

    else if (cmd.size() == 2 && cmd[0] == "del") {
        auto it = g_data.find(cmd[1]) ;
        
        if (it == g_data.end()) {
            response.status = RES_NX ;
            return ;
        } 
        
        else {
            g_data.erase(it) ;
            response.status = RES_OK ;
        }
    }
    
    else if (cmd.size() == 1 && cmd[0] == "clear") {
        g_data.clear() ;
        response.status = RES_OK ;
    } 
    
    else {
        fprintf(stderr, "Unknown command: %s\n", cmd[0].c_str()) ;
        response.status = RES_ERR ;
        return ;
    }
}