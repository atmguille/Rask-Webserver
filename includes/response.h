#ifndef RESPONSE_H
#define RESPONSE_H

#include "../includes/config_parser.h"
#include "../includes/request.h"

// TODO: especificar return en docstring cuando tengamos bien hechos los códigos de return

/**
 * Builds and sends bad request response
 * @param client_fd
 * @param server_attrs
 * @return
 */
int response_bad_request(int client_fd, struct config *server_attrs);

/**
 * Builds and sends request too long response
 * @param client_fd
 * @param server_attrs
 * @return
 */
int response_request_too_long(int client_fd, struct config *server_attrs);

/**
 * Builds and sends not found response
 * @param client_fd
 * @param server_attrs
 * @return
 */
int response_not_found(int client_fd, struct config *server_attrs);

/**
 * Builds and sends not implemented response
 * @param client_fd
 * @param server_attrs
 * @return
 */
int response_not_implemented(int client_fd, struct config *server_attrs);

/**
 * Builds and sends internal server error response
 * @param client_fd
 * @param server_attrs
 * @return
 */
int response_internal_server_error(int client_fd, struct config *server_attrs);

/**
 * Builds and sends get response following parameters specified in request
 * @param client_fd
 * @param server_attrs
 * @param request
 * @return
 */
int response_get(int client_fd, struct config *server_attrs, struct request *request);

/**
 * Builds and sends post response, extracting parameters from request body
 * @param client_fd
 * @param server_attrs
 * @param request
 * @return
 */
int response_post(int client_fd, struct config *server_attrs, struct request *request);

/**
 * Builds and sends options response
 * @param client_fd
 * @param server_attrs
 * @return
 */
int response_options(int client_fd, struct config *server_attrs);

#endif //RESPONSE_H
