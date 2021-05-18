/*
 * Wazuh Module for Office365 logs
 * Copyright (C) 2015-2021, Wazuh Inc.
 * May 3, 2021.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#ifndef WM_OFFICE365_H
#define WM_OFFICE365_H

#define WM_OFFICE365_LOGTAG ARGV0 ":" OFFICE365_WM_NAME

#define WM_OFFICE365_DEFAULT_ENABLED 1
#define WM_OFFICE365_DEFAULT_RUN_ON_START 1
#define WM_OFFICE365_DEFAULT_SKIP_ON_ERROR 1
#define WM_OFFICE365_DEFAULT_INTERVAL 20
#define WM_OFFICE365_DEFAULT_AZURE 0
#define WM_OFFICE365_DEFAULT_EXCHANGE 0
#define WM_OFFICE365_DEFAULT_SHAREPOINT 0
#define WM_OFFICE365_DEFAULT_GENERAL 0
#define WM_OFFICE365_DEFAULT_DLP 0

typedef struct wm_office365_auth {
    char *tenant_id;
    char *client_id;
    char *client_secret_path;
    char *client_secret;
    struct wm_office365_auth *next;
} wm_office365_auth;

typedef struct subscription_flags_t {
    unsigned short azure:1;
    unsigned short exchange:1;
    unsigned short sharepoint:1;
    unsigned short general:1;
    unsigned short dlp:1;
} subscription_flags_t;

typedef struct wm_office365 {
    int enabled;
    int run_on_start;
    int skip_on_error;
    int only_future_events;
    time_t interval;                        // Interval betweeen events in seconds
    wm_office365_auth *auth;
    subscription_flags_t subscription;
} wm_office365;

extern const wm_context WM_OFFICE365_CONTEXT;  // Context

// Parse XML
int wm_office365_read(const OS_XML *xml, xml_node **nodes, wmodule *module);

#endif