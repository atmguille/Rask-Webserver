#include "../includes/config_parser.h"
#include "../srclib/logging/logging.h"
#include <stdlib.h>
#include <confuse.h>

ServerAttributes *load_server_config(char *config_file_name) {
    ServerAttributes *server_attr;
    cfg_t *cfg;
    /* Although the macro used to specify an integer option is called
     * CFG_SIMPLE_INT(), it actually expects a long int. On a 64 bit system
     * where ints are 32 bit and longs 64 bit (such as the x86-64 or amd64
     * architectures), you will get weird effects if you use an int here.
     *
     * If you use the regular (non-"simple") options, ie CFG_INT() and use
     * cfg_getint(), this is not a problem as the data types are implicitly
     * cast.
     */
    long int aux_max_clients;
    long int aux_listen_port;

    server_attr = (ServerAttributes *)malloc(sizeof(ServerAttributes));
    if (server_attr == NULL) {
        print_error("failed to allocate memory for ServerAttributes");
        return NULL;
    }

    cfg_opt_t opts[] = {
            CFG_SIMPLE_STR("server_root", &server_attr->server_root),
            CFG_SIMPLE_INT("max_clients", &aux_max_clients),
            CFG_SIMPLE_INT("listen_port", &aux_listen_port),
            CFG_SIMPLE_STR("server_signature", &server_attr->server_signature),  // TODO: vamos a cogerlo como una sola string, aunque en moodle lo separan sin poner comillas. Hay que preguntar
            CFG_END()
    };

    cfg = cfg_init(opts, 0);
    if (cfg_parse(cfg, config_file_name) == CFG_PARSE_ERROR) {
        print_error("failed to parse config file");
        free(server_attr);
        return NULL;
    }
    // Cast is done here to avoid doing casts elsewhere
    server_attr->max_clients = (int) aux_max_clients;
    server_attr->listen_port = (int) aux_listen_port;

    print_info("Server Attributes:");
    print_info("\tserver_root: %s", server_attr->server_root);
    print_info("\tmax_clients: %d", server_attr->max_clients);
    print_info("\tlisten_port: %d", server_attr->listen_port);
    print_info("\tserver_signature: %s", server_attr->server_signature);

    return server_attr;
}

void server_attr_destroy(ServerAttributes *server_attr) {
    free(server_attr->server_root);
    free(server_attr->server_signature);
    free(server_attr);
}

