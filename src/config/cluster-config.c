/*
 * Cluster settings manager
 * Copyright (C) 2015-2019, Wazuh Inc.
 * Oct 16, 2017.
 *
 * This program is a free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#include "shared.h"
#include "config.h"
#include "global-config.h"


int Read_Cluster(XML_NODE node, void *d1, __attribute__((unused)) void *d2) {

    static const char *disabled = "disabled";
    static const char *cluster_name = "name";
    static const char *node_name = "node_name";
    static const char *node_type = "node_type";
    static const char *key = "key";
    static const char *socket_timeout = "socket_timeout";
    static const char *connection_timeout = "connection_timeout";
    static const char *interval = "interval";
    static const char *nodes = "nodes";
    static const char *hidden = "hidden";
    static const char *port = "port";
    static const char *bind_addr = "bind_addr";
    static const char *C_VALID = "!\"#$%&'-.0123456789:<=>?ABCDEFGHIJKLMNOPQRESTUVWXYZ[\\]^_abcdefghijklmnopqrstuvwxyz{|}~";

    _Config *Config;
    Config = (_Config *)d1;
    int i;
    int disable_cluster_info = 0;

    Config->hide_cluster_info = 0;

    for (i = 0; node[i]; i++) {
        if (!node[i]->element) {
            merror(XML_ELEMNULL);
            return OS_INVALID;
        } else if (!node[i]->content) {
            merror(XML_VALUENULL, node[i]->element);
            return OS_INVALID;
        } else if (!strcmp(node[i]->element, cluster_name)) {
            if (!strlen(node[i]->content)) {
                merror("Cluster name is empty in configuration");
                return OS_INVALID;
            } else if (strspn(node[i]->content, C_VALID) < strlen(node[i]->content)) {
                merror("Detected a not allowed character in cluster name: \"%s\". Characters allowed: \"%s\".", node[i]->content, C_VALID);
                return OS_INVALID;
            }
            os_strdup(node[i]->content, Config->cluster_name);
        } else if (!strcmp(node[i]->element, node_name)) {
            if (!strlen(node[i]->content)) {
                merror("Node name is empty in configuration");
                return OS_INVALID;
            } else if (strspn(node[i]->content, C_VALID) < strlen(node[i]->content)) {
                merror("Detected a not allowed character in node name: \"%s\". Characters allowed: \"%s\".", node[i]->content, C_VALID);
                return OS_INVALID;
            } else if (strcasecmp(node[i]->content, "$NODE_NAME") == 0) {
                // Get environment variables
                char * node_name = getenv("NODE_NAME");

                if (node_name) {
                    free(Config->node_name);
                    os_strdup(node_name, Config->node_name);
                } else {
                    mwarn("Cannot find environment variable 'NODE_NAME'");
                }
            } else if (strcasecmp(node[i]->content, "$HOSTNAME") == 0) {
                char hostname[512];

                if (gethostname(hostname, sizeof(hostname)) != 0) {
                    strncpy(hostname, "localhost", sizeof(hostname));
                }

                hostname[sizeof(hostname) - 1] = '\0';
                free(Config->node_name);
                os_strdup(hostname, Config->node_name);
            } else {
                free(Config->node_name);
                os_strdup(node[i]->content, Config->node_name);
            }
        } else if (!strcmp(node[i]->element, node_type)) {
            if (!strlen(node[i]->content)) {
                merror("Node type is empty in configuration");
                return OS_INVALID;
            } else if (strcasecmp(node[i]->content, "$NODE_TYPE")) {
                // Get environment variables
                char * node_type = getenv("NODE_TYPE");

                if (node_type) {
                    free(Config->node_type);
                    os_strdup(node_type, Config->node_type);
                } else {
                    mwarn("Cannot find environment variable 'NODE_TYPE'");
                }
            } else if (strcmp(node[i]->content, "worker") && strcmp(node[i]->content, "client") && strcmp(node[i]->content, "master")) {
                merror("Detected a not allowed node type '%s'. Valid types are 'master', 'worker' and '$node_type'.", node[i]->content);
                return OS_INVALID;
            } else {
                free(Config->node_type);
                os_strdup(node[i]->content, Config->node_type);
            }

        } else if (!strcmp(node[i]->element, key)) {
        } else if (!strcmp(node[i]->element, socket_timeout)) {
        } else if (!strcmp(node[i]->element, connection_timeout)) {
        } else if (!strcmp(node[i]->element, disabled)) {
            if (strcmp(node[i]->content, "yes") && strcmp(node[i]->content, "no")) {
                merror("Detected a not allowed value for disabled tag '%s'. Valid values are 'yes' and 'no'.", node[i]->content);
                return OS_INVALID;
            }
            if (strcmp(node[i]->content, "yes") == 0) {
                disable_cluster_info = 1;
            }
        } else if (!strcmp(node[i]->element, hidden)) {
            if (strcmp(node[i]->content, "yes") == 0) {
                Config->hide_cluster_info = 1;
            } else if (strcmp(node[i]->content, "no") == 0) {
                Config->hide_cluster_info = 0;
            } else {
                merror(XML_VALUEERR, node[i]->element, node[i]->content);
                return OS_INVALID;
            }
        } else if (!strcmp(node[i]->element, interval)) {
            mwarn("Detected a deprecated configuration for cluster. Interval option is not longer available.");
        } else if (!strcmp(node[i]->element, nodes)) {
        } else if (!strcmp(node[i]->element, port)) {
        } else if (!strcmp(node[i]->element, bind_addr)) {
        } else {
            merror(XML_INVELEM, node[i]->element);
            return OS_INVALID;
        }
    }

    if (disable_cluster_info)
        Config->hide_cluster_info = 1;

    return 0;
 }
