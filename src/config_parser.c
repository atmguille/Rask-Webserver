#include "../include/config_parser.h"
#include "../srclib/logging/logging.h"
#include <stdlib.h>
#include <confuse.h>

struct config *config_load(char *config_file_name) {
    struct config *server_attrs;
    cfg_t *cfg;
    int ret;
    /* Although the macro used to specify an integer option is called
     * CFG_SIMPLE_INT(), it actually expects a long int. On a 64 bit system
     * where ints are 32 bit and longs 64 bit (such as the x86-64 or amd64
     * architectures), you will get weird effects if you use an int here.
     *
     * If you use the regular (non-"simple") options, ie CFG_INT() and use
     * cfg_getint(), this is not a problem as the data types are implicitly
     * cast.
     */
    long int max_clients;
    long int listen_port;

    server_attrs = (struct config *)calloc(1, sizeof(struct config));
    if (server_attrs == NULL) {
        print_error("failed to allocate memory for server attributes");
        return NULL;
    }

    cfg_opt_t opts[] = {
            CFG_SIMPLE_STR("signature", &server_attrs->signature),
            CFG_SIMPLE_STR("base_path", &server_attrs->base_path),
            CFG_SIMPLE_STR("default_path", &server_attrs->default_path),
            CFG_SIMPLE_INT("max_clients", &max_clients),
            CFG_SIMPLE_INT("listen_port", &listen_port),
            CFG_END()
    };

    cfg = cfg_init(opts, 0);
    if ((ret = cfg_parse(cfg, config_file_name)) != CFG_SUCCESS) {
        if (ret == CFG_FILE_ERROR) {
            print_error("failed to read config file");
        } else if (ret == CFG_PARSE_ERROR) {
            print_error("failed to parse config file");
        }
        cfg_free(cfg);
        free(server_attrs);
        return NULL;
    }
    cfg_free(cfg);
    // Cast is done here to avoid doing casts elsewhere
    server_attrs->max_clients = (int) max_clients;
    server_attrs->listen_port = (int) listen_port;

    print_debug("Server attributes:");
    print_debug("  signature: %s", server_attrs->signature);
    print_debug("  base_path: %s", server_attrs->base_path);
    print_debug("  default_path: %s", server_attrs->default_path);
    print_debug("  max_clients: %d", server_attrs->max_clients);
    print_debug("  listen_port: %d", server_attrs->listen_port);

    return server_attrs;
}

void config_destroy(struct config *sever_attrs) {
    free(sever_attrs->signature);
    free(sever_attrs->base_path);
    free(sever_attrs->default_path);
    free(sever_attrs);
}

