/*
 * Copyright (C) 2015-2021, Wazuh Inc.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */
#ifdef __linux__
#include "syscheck_audit.h"
#include "list_op.h"

#define RELOAD_RULES_INTERVAL 30 // Seconds to re-add Audit rules

#ifdef ENABLE_AUDIT

#ifdef WAZUH_UNIT_TESTING
#define static
#endif

static OSList *whodata_directories;
static pthread_mutex_t rules_mutex = PTHREAD_MUTEX_INITIALIZER;

static int audit_rule_manipulation = 0;

static void free_whodata_directory(whodata_directory_t *directory) {
    os_free(directory->path);
    os_free(directory);
}

static void _add_whodata_directory(const char *path) {
    OSListNode *node;
    whodata_directory_t *directory;

    // Search for duplicates
    for (node = OSList_GetFirstNode(whodata_directories); node != NULL;
         node = OSList_GetNextNode(whodata_directories)) {
        directory = (whodata_directory_t *)node->data;

        if (strcmp(path, directory->path) == 0) {
            directory->pending_removal = 0;
            return;
        }
    }

    // If we got here, we need to add a new directory
    os_malloc(sizeof(whodata_directory_t), directory);

    os_strdup(path, directory->path);
    directory->pending_removal = 0;

    if (OSList_AddData(whodata_directories, directory) == NULL) {
        free_whodata_directory(directory); // LCOV_EXCL_LINE
    }
}

void add_whodata_directory(const char *path) {
    w_mutex_lock(&rules_mutex);
    _add_whodata_directory(path);
    w_mutex_unlock(&rules_mutex);
}

void remove_audit_rule_syscheck(const char *path) {
    OSListNode *node;

    w_mutex_lock(&rules_mutex);

    for (node = OSList_GetFirstNode(whodata_directories); node != NULL;
         node = OSList_GetNextNode(whodata_directories)) {
        whodata_directory_t *directory = (whodata_directory_t *)node->data;

        if (strcmp(path, directory->path) == 0) {
            directory->pending_removal = 1;
            break;
        }
    }

    w_mutex_unlock(&rules_mutex);
}

int fim_rules_initial_load() {
    unsigned int i = 0;
    int retval;
    char *directory = NULL;
    int rules_added = 0;
    int auditd_fd = audit_open();
    int res = audit_get_rule_list(auditd_fd);

    audit_close(auditd_fd);

    if (!res) {
        mterror(ARGV0, FIM_ERROR_WHODATA_READ_RULE); // LCOV_EXCL_LINE
    }

    w_mutex_lock(&rules_mutex);
    for (i = 0; syscheck.dir[i]; i++) {
        // Check if dir[i] is set in whodata mode
        if ((syscheck.opts[i] & WHODATA_ACTIVE) == 0) {
            continue; // LCOV_EXCL_LINE
        }

        directory = fim_get_real_path(i);
        if (*directory == '\0') {
            free(directory); // LCOV_EXCL_LINE
            continue; // LCOV_EXCL_LINE
        }

        // Add whodata directories until max_audit_entries is reached.
        if (rules_added >= syscheck.max_audit_entries) {
            mterror(ARGV0, FIM_ERROR_WHODATA_MAXNUM_WATCHES, directory, syscheck.max_audit_entries);
            free(directory);
            break;
        }

        _add_whodata_directory(directory);

        switch (search_audit_rule(directory, WHODATA_PERMS, AUDIT_KEY)) {
        // The rule is not in audit_rule_list
        case 0:
            if (retval = audit_add_rule(directory, WHODATA_PERMS, AUDIT_KEY), retval > 0) {
                mtdebug1(ARGV0, FIM_AUDIT_NEWRULE, directory);
                rules_added++;
            } else if (retval != -EEXIST) {
                mtwarn(ARGV0, FIM_WARN_WHODATA_ADD_RULE, directory);
            } else {
                mtdebug1(ARGV0, FIM_AUDIT_ALREADY_ADDED, directory);
            }
            break;

        case 1:
            mtdebug1(ARGV0, FIM_AUDIT_RULEDUP, directory);
            break;

        default:
            mterror(ARGV0, FIM_ERROR_WHODATA_CHECK_RULE);
            break;
        }
        // real_path can't be NULL
        free(directory);
    }

    w_mutex_unlock(&rules_mutex);
    return rules_added;
}

void fim_audit_reload_rules() {
    int retval;
    int rules_added = 0;
    static bool reported = 0;
    int auditd_fd;
    int res;
    OSListNode *node = NULL;

    mtdebug1(ARGV0, FIM_AUDIT_RELOADING_RULES);

    auditd_fd = audit_open();
    res = audit_get_rule_list(auditd_fd);

    audit_close(auditd_fd);

    if (!res) {
        mterror(ARGV0, FIM_ERROR_WHODATA_READ_RULE); // LCOV_EXCL_LINE
    }

    w_mutex_lock(&rules_mutex);

    node = OSList_GetFirstNode(whodata_directories);

    while (node != NULL) {
        whodata_directory_t *directory = (whodata_directory_t *)node->data;

        switch (search_audit_rule(directory->path, WHODATA_PERMS, AUDIT_KEY)) {
        // The rule is not in audit_rule_list
        case 0:
            // If we had to remove it, we are done
            if (directory->pending_removal != 0) {
                free_whodata_directory(directory);
                OSList_DeleteCurrentlyNode(whodata_directories);
                node = OSList_GetCurrentlyNode(whodata_directories);
                continue;
            }

            if (rules_added >= syscheck.max_audit_entries) {
                if (!reported) {
                    mterror(ARGV0, FIM_ERROR_WHODATA_MAXNUM_WATCHES, directory->path, syscheck.max_audit_entries);
                } else {
                    mtdebug1(ARGV0, FIM_ERROR_WHODATA_MAXNUM_WATCHES, directory->path, syscheck.max_audit_entries);
                }
                reported = 1;
                break;
            }

            if (retval = audit_add_rule(directory->path, WHODATA_PERMS, AUDIT_KEY), retval > 0) {
                mtdebug1(ARGV0, FIM_AUDIT_NEWRULE, directory->path);
                rules_added++;
            } else if (retval != -EEXIST) {
                mtdebug1(ARGV0, FIM_WARN_WHODATA_ADD_RULE, directory->path);
            } else {
                mtdebug1(ARGV0, FIM_AUDIT_ALREADY_ADDED, directory->path);
            }

            break;

        case 1:
            if (directory->pending_removal != 0) {
                audit_rule_manipulation++;
                audit_delete_rule(directory->path, WHODATA_PERMS, AUDIT_KEY);
                free_whodata_directory(directory);
                OSList_DeleteCurrentlyNode(whodata_directories);
                node = OSList_GetCurrentlyNode(whodata_directories);
                continue;
            } else {
                mtdebug1(ARGV0, FIM_AUDIT_RULEDUP, directory->path);
            }
            break;

        default:
            mterror(ARGV0, FIM_ERROR_WHODATA_CHECK_RULE);
            break;
        }

        node = OSList_GetNextNode(whodata_directories);
    }
    w_mutex_unlock(&rules_mutex);

    mtdebug1(ARGV0, FIM_AUDIT_RELOADED_RULES, rules_added);
}

int fim_manipulated_audit_rules() {
    int retval;

    w_mutex_lock(&rules_mutex);
    retval = audit_rule_manipulation;

    if (audit_rule_manipulation != 0) {
        audit_rule_manipulation--;
    }
    w_mutex_unlock(&rules_mutex);

    return retval;
}

void clean_rules(void) {
    OSListNode *node;

    w_mutex_lock(&rules_mutex);

    audit_thread_active = 0;
    mtdebug2(ARGV0, FIM_AUDIT_DELETE_RULE);

    for (node = OSList_GetFirstNode(whodata_directories); node != NULL;
         node = OSList_GetNextNode(whodata_directories)) {
        whodata_directory_t *directory = (whodata_directory_t *)node->data;

        audit_delete_rule(directory->path, WHODATA_PERMS, AUDIT_KEY);
    }

    audit_rules_list_free();
    OSList_CleanNodes(whodata_directories);
    w_mutex_unlock(&rules_mutex);
}

// LCOV_EXCL_START
int fim_audit_rules_init() {
    whodata_directories = OSList_Create();
    if (whodata_directories == NULL) {
        return -1;
    }

    OSList_SetFreeDataPointer(whodata_directories, (void (*)(void *))free_whodata_directory);

    return 0;
}

void *audit_reload_thread() {
    sleep(RELOAD_RULES_INTERVAL);
    while (audit_thread_active) {
        fim_audit_reload_rules();

        sleep(RELOAD_RULES_INTERVAL);
    }

    return NULL;
}
// LCOV_EXCL_STOP

#endif // ENABLE_AUDIT
#endif // __linux__
