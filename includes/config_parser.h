#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

typedef struct {
    char    *server_root;       // Directory where all the source files are
    int      max_clients;       // Max number of clients that the server will answer simultaneously
    int      listen_port;       // Port in which the server will listen to connections
    char    *server_signature;  // ServerName
} ServerAttributes;

/**
 * Fills the struct with the fields found in config_file
 * @param config_file_name
 */
ServerAttributes *load_server_config(char *config_file_name);
/**
 * Frees struct as well as the strings
 * @param server_attr
 */
void server_attr_destroy(ServerAttributes *server_attr);

#endif //CONFIG_PARSER_H
