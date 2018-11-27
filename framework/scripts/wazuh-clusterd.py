#!/var/ossec/python/bin/python3

# Created by Wazuh, Inc. <info@wazuh.com>.
# This program is a free software; you can redistribute it and/or modify it under the terms of GPLv2
import logging
import asyncio
import argparse
import os
import sys
from wazuh.cluster import cluster, __version__, __author__, __ossec_name__, __licence__, master, local_server, client
from wazuh import common, configuration, pyDaemonModule


#
# Aux functions
#
def set_logging(foreground_mode=False, debug_mode=0):
    logging.getLogger('asyncio').setLevel(logging.CRITICAL)
    logger = logging.getLogger()
    # configure logger
    fh = cluster.CustomFileRotatingHandler(filename="{}/logs/cluster.log".format(common.ossec_path), when='midnight')
    formatter = logging.Formatter('%(asctime)s %(levelname)-8s: [%(tag)-15s] %(message)s')
    fh.setFormatter(formatter)
    logger.addHandler(fh)

    if foreground_mode:
        ch = logging.StreamHandler()
        ch.setFormatter(formatter)
        logger.addHandler(ch)

    logger.addFilter(cluster.ClusterFilter(tag='Main'))

    # add a new debug level
    logging.DEBUG2 = 5

    def debug2(self, message, *args, **kws):
        if self.isEnabledFor(logging.DEBUG2):
            self._log(logging.DEBUG2, message, args, **kws)

    def error(self, msg, *args, **kws):
        if self.isEnabledFor(logging.ERROR):
            kws['exc_info'] = self.isEnabledFor(logging.DEBUG2)
            self._log(logging.ERROR, msg, args, **kws)

    logging.addLevelName(logging.DEBUG2, "DEBUG2")

    logging.Logger.debug2 = debug2
    logging.Logger.error = error

    debug_level = logging.DEBUG2 if debug_mode == 2 else logging.DEBUG if \
                  debug_mode == 1 else logging.INFO

    logger.setLevel(debug_level)
    return logger


def print_version():
    print("\n{} {} - {}\n\n{}".format(__ossec_name__, __version__, __author__, __licence__))


#
# Master main
#
async def master_main(args, cluster_config):
    my_server = master.Master(args.performance_test, args.concurrency_test, cluster_config, args.ssl)
    my_local_server = local_server.LocalServer(performance_test=args.performance_test,
                                               concurrency_test=args.concurrency_test, configuration=cluster_config,
                                               enable_ssl=args.ssl)
    await asyncio.gather(my_server.start(), my_local_server.start())


#
# Worker main
#
async def worker_main(args, cluster_config):
    my_local_server = local_server.LocalServer(performance_test=args.performance_test,
                                               concurrency_test=args.concurrency_test, configuration=cluster_config,
                                               enable_ssl=args.ssl)
    while True:
        my_client = client.AbstractClientManager(cluster_config, args.ssl, args.performance_test,
                                                 args.concurrency_test, args.send_file, args.send_string)
        try:
            await asyncio.gather(my_client.start(), my_local_server.start())
        except asyncio.CancelledError:
            logging.info("Connection with server has been lost. Reconnecting in 10 seconds.")
            await asyncio.sleep(10)


#
# Main
#
if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    ####################################################################################################################
    # Dev options - Silenced in the help message.
    ####################################################################################################################
    # Performance test - value stored in args.performance_test will be used to send a request of that size in bytes to
    # all clients/to the server.
    parser.add_argument('--performance_test', type=int, dest='performance_test', help=argparse.SUPPRESS)
    # Concurrency test - value stored in args.concurrency_test will be used to send that number of requests in a row,
    # without sleeping.
    parser.add_argument('--concurrency_test', type=int, dest='concurrency_test', help=argparse.SUPPRESS)
    # Send string test - value stored in args.send_string variable will be used to send a string with that size in bytes
    # to the server. Only implemented in worker nodes.
    parser.add_argument('--string', help=argparse.SUPPRESS, type=int, dest='send_string')
    # Send file test - value stored in args.send_file variable is the path of a file to send to the server. Only
    # implemented in worker nodes.
    parser.add_argument('--file', help=argparse.SUPPRESS, type=str, dest='send_file')
    ####################################################################################################################
    parser.add_argument('--ssl', help="Enable communication over SSL", action='store_true', dest='ssl')
    parser.add_argument('-f', help="Run in foreground", action='store_true', dest='foreground')
    parser.add_argument('-d', help="Enable debug messages. Use twice to increase verbosity.", action='count',
                        dest='debug_level')
    parser.add_argument('-V', help="Print version", action='store_true', dest="version")
    parser.add_argument('-r', help="Run as root", action='store_true', dest='root')
    args = parser.parse_args()

    if args.version:
        print_version()
        sys.exit(0)

    # Foreground/Daemon
    if not args.foreground:
        pyDaemonModule.pyDaemon()

    # Set logger
    try:
        debug_mode = configuration.get_internal_options_value('wazuh_clusterd', 'debug', 2, 0) or args.debug_level
    except Exception:
        debug_mode = 0

    # set correct permissions on cluster.log file
    if os.path.exists('{0}/logs/cluster.log'.format(common.ossec_path)):
        os.chown('{0}/logs/cluster.log'.format(common.ossec_path), common.ossec_uid, common.ossec_gid)
        os.chmod('{0}/logs/cluster.log'.format(common.ossec_path), 0o660)

    # Drop privileges to ossec
    if not args.root:
        os.setgid(common.ossec_gid)
        os.seteuid(common.ossec_uid)

    set_logging(args.foreground, debug_mode)

    cluster_configuration = cluster.read_config()

    main_function = master_main if cluster_configuration['node_type'] == 'master' else worker_main
    try:
        asyncio.run(main_function(args, cluster_configuration))
    except KeyboardInterrupt:
        logging.info("SIGINT received. Bye!")
