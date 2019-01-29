#!/usr/bin/env python

# Copyright (C) 2015-2019, Wazuh Inc.
# Created by Wazuh, Inc. <info@wazuh.com>.
# This program is a free software; you can redistribute it and/or modify it under the terms of GPLv2


from wazuh import common
from wazuh.exception import WazuhException
from wazuh.utils import sort_array, search_array
from os import listdir
from os.path import isfile, isdir, join


def get_lists(path=None, offset=0, limit=common.database_limit, sort=None, search=None):
    """
    Get CDB lists
    :param path: Relative path of list file to get
    :param offset: First item to return.
    :param limit: Maximum number of items to return.
    :param sort: Sorts the items.
    :param search:  Looks for items with the specified string.
    :return: CDB list
    """

    output = []

    if limit == 0:
        raise WazuhException(1406)

    if path:
        items = get_list_from_file(path)
        output.append(items)
    else:
        dir_content = listdir(common.lists_path)
        for name in dir_content:
            absolute_path = join(common.lists_path, name)
            relative_path = join('etc/lists', name)
            if (isfile(absolute_path)) and ('.cdb' not in name) \
                and ('~' not in name):
                items = get_list_from_file(relative_path)
                output.append({'path': relative_path, 'items': items})
            elif isdir(absolute_path):
                subdir_content = listdir(join(common.lists_path, name))
                for subdir_name in subdir_content:
                    subdir_absolute_path = join(common.lists_path, name, subdir_name)
                    subdir_relative_path = join('etc/lists', name, subdir_name)
                    if (isfile(subdir_absolute_path)) and ('.cdb' not in subdir_name) \
                        and ('~' not in subdir_name):
                        items = get_list_from_file(subdir_relative_path)
                        output.append({'path': subdir_relative_path, 'items': items})

    if offset:
        output = output[offset:]

    if search:
        output = search_array(output, search['value'], search['negation'])

    if sort:
        output = sort_array(output, sort['fields'], sort['order'])

    # limit is common.database_limit by default
    output = output[:limit]

    return {'totalItems' : len(output), 'items': output}


def get_list_from_file(path):
    """
    Get CDB list from file
    :param path: Relative path of list file to get
    :return: CDB list
    """
    file_path = join(common.ossec_path, path)
    output = []

    try:
        with open(file_path) as f:
            for line in f.read().splitlines():
                key = line.split(':')[0]
                value = line.split(':')[1]
                output.append({'key': key, 'value': value})
                        
    except Exception as e:
        raise WazuhException(1005)

    return output


def get_path_lists():
    """
    Get paths of all CDB lists
    :return: List with paths of all CDB lists
    """
    dir_content = listdir(common.lists_path)

    output = []

    for name in dir_content:
        absolute_path = join(common.lists_path, name)
        relative_path = 'etc/lists'

        if (isfile(absolute_path)) and ('.cdb' not in name) \
            and ('~' not in name):
            output.append({'path': relative_path, 'name': name})

        elif isdir(absolute_path):
            subdir_content = listdir(absolute_path)

            for subdir_name in subdir_content:
                subdir_absolute_path = join(absolute_path, subdir_name)
                subdir_relative_path = join(relative_path, name)

                if (isfile(subdir_absolute_path)) and ('.cdb' not in subdir_name) \
                    and ('~' not in subdir_name):
                    output.append({'path': subdir_relative_path, \
                        'name': subdir_name})

    return {'totalItems': len(output), 'items': output}
