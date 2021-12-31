//
// Created by Robert Manzke on 31.12.21.
//
#include <libserialport.h>
#include <cstdio>

int main(){
    struct sp_port **port_list;
    int result = sp_list_ports(&port_list);
    for (int i = 0; port_list[i] != NULL; i++) {
        struct sp_port *port = port_list[i];   // Get the name of the port.
        char *port_name = sp_get_port_name(port);
        printf("%s\n", port_name);
    }
}
