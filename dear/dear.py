#!/usr/bin/env @DEAR_PYTHON_EXECUTABLE@
# -*- coding: utf-8 -*-

# Copyright (C) 2012-2019 by László Nagy
# This file is part of Dear.
#
# Dear is a tool to transparently distribute workload launched by build
# systems.
#
# Dear is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Dear is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
" This module is responsible of launching the build system. Jobs should
be distributed across the network using the celery framework and executed on
available workers.

This implementation is using the LD_PRELOAD or DYLD_INSERT_LIBRARIES
mechanisms provided by the dynamic linker. The related library is implemented
in C language and can be found under 'libdear' directory.

The 'libdear' library is capturing all child process creation and sending
relevant jobs to the celery brokers.

This module implements the build command execution with the 'libdear' library.
"""

import argparse
import subprocess
import sys
import functools
import os
import os.path
import re
import shlex
import logging


def shell_split(string):
    # type: (str) -> List[str]
    """ Takes a command string and returns as a list. """

    def unescape(arg):
        # type: (str) -> str
        """ Gets rid of the escaping characters. """

        if len(arg) >= 2 and arg[0] == arg[-1] and arg[0] == '"':
            return re.sub(r'\\(["\\])', r'\1', arg[1:-1])
        return re.sub(r'\\([\\ $%&\(\)\[\]\{\}\*|<>@?!])', r'\1', arg)

    return [unescape(token) for token in shlex.split(string)]


def run_build(command, *args, **kwargs):
    # type: (...) -> int
    """ Run and report build command execution

    :param command: list of tokens
    :return: exit code of the process
    """
    environment = kwargs.get('env', os.environ)
    logging.debug('run build %s, in environment: %s', command, environment)
    exit_code = subprocess.call(command, *args, **kwargs)
    logging.debug('build finished with exit code: %d', exit_code)
    return exit_code


def run_command(command, cwd=None):
    # type: (List[str], str) -> List[str]
    """ Run a given command and report the execution.

    :param command: array of tokens
    :param cwd: the working directory where the command will be executed
    :return: output of the command
    """
    def decode_when_needed(result):
        # type: (Any) -> str
        """ check_output returns bytes or string depend on python version """
        if not isinstance(result, str):
            return result.decode('utf-8')
        return result

    try:
        directory = os.path.abspath(cwd) if cwd else os.getcwd()
        logging.debug('exec command %s in %s', command, directory)
        output = subprocess.check_output(command,
                                         cwd=directory,
                                         stderr=subprocess.STDOUT)
        return decode_when_needed(output).splitlines()
    except subprocess.CalledProcessError as ex:
        ex.output = decode_when_needed(ex.output).splitlines()
        raise ex


def reconfigure_logging(verbose_level):
    """ Reconfigure logging level and format based on the verbose flag.

    :param verbose_level: number of `-v` flags received by the command
    :return: no return value
    """
    # exit when nothing to do
    if verbose_level == 0:
        return

    root = logging.getLogger()
    # tune level
    level = logging.WARNING - min(logging.WARNING, (10 * verbose_level))
    root.setLevel(level)
    # be verbose with messages
    if verbose_level <= 3:
        fmt_string = '%(name)s: %(levelname)s: %(message)s'
    else:
        fmt_string = '%(name)s: %(levelname)s: %(funcName)s: %(message)s'
    handler = logging.StreamHandler(sys.stdout)
    handler.setFormatter(logging.Formatter(fmt=fmt_string))
    root.handlers = [handler]


def command_entry_point(function):
    # type: (Callable[[], int]) -> Callable[[], int]
    """ Decorator for command entry methods.

    The decorator initialize/shutdown logging and guard on programming
    errors (catch exceptions).

    The decorated method can have arbitrary parameters, the return value will
    be the exit code of the process. """

    @functools.wraps(function)
    def wrapper(*args, **kwargs):
        """ Do housekeeping tasks and execute the wrapped method. """

        try:
            logging.basicConfig(format='%(name)s: %(message)s',
                                level=logging.WARNING,
                                stream=sys.stdout)
            # this hack to get the executable name as %(name)
            logging.getLogger().name = os.path.basename(sys.argv[0])
            return function(*args, **kwargs)
        except KeyboardInterrupt:
            logging.warning('Keyboard interrupt')
            return 130  # signal received exit code for bash
        except (OSError, subprocess.CalledProcessError):
            logging.exception('Internal error.')
            if logging.getLogger().isEnabledFor(logging.DEBUG):
                logging.error("Please report this bug and attach the output "
                              "to the bug report")
            else:
                logging.error("Please run this command again and turn on "
                              "verbose mode (add '-vvvv' as argument).")
            return 64  # some non used exit code for internal errors
        finally:
            logging.shutdown()

    return wrapper


@command_entry_point
def intercept_build():
    # type: () -> int
    """ Entry point for 'intercept-build' command. """

    args = parse_args_for_intercept_build()
    exit_code = launch(args)

    return exit_code


def launch(args):
    """ Launch the build process

    :param args:        the parsed and validated command line arguments
    :return:            the exit status of build process. """

    # run the build command
    environment = setup_environment(args)
    exit_code = run_build(args.build, env=environment)
    return exit_code


def setup_environment(args):
    # type: (argparse.Namespace, str) -> Dict[str, str]
    """ Sets up the environment for the build command.

    In order to launch the sub-commands (executed by the build process),
    it needs to prepare the environment. It's either the compiler wrappers
    shall be announce as compiler or the intercepting library shall be
    announced for the dynamic linker.

    :param args:        command line arguments
    :return: a prepared set of environment variables. """

    environment = dict(os.environ)

    if sys.platform == 'darwin':
        environment.update({
            'DYLD_INSERT_LIBRARIES': args.libdear,
            'DYLD_FORCE_FLAT_NAMESPACE': '1'
        })
    else:
        environment.update({'LD_PRELOAD': args.libdear})

    return environment


def parse_args_for_intercept_build():
    """ Parse and validate command-line arguments for intercept-build. """

    parser = create_intercept_parser()
    args = parser.parse_args()

    reconfigure_logging(args.verbose)
    logging.debug('Raw arguments %s', sys.argv)

    # short validation logic
    if not args.build:
        parser.error(message='missing build command')

    logging.debug('Parsed arguments: %s', args)
    return args


def create_intercept_parser():
    """ Creates a parser for command-line arguments to 'intercept'. """

    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument(
        '--version',
        action='version',
        version='%(prog)s @DEAR_VERSION@')
    parser.add_argument(
        '--verbose', '-v',
        action='count',
        default=0,
        help="""Enable verbose output from '%(prog)s'. A second, third and
        fourth flags increases verbosity.""")

    advanced = parser.add_argument_group('advanced options')
    advanced.add_argument(
        '--libdear', '-l',
        dest='libdear',
        default="@DEFAULT_PRELOAD_FILE@",
        action='store',
        help="""specify libdear file location.""")

    parser.add_argument(
        dest='build', nargs=argparse.REMAINDER, help="""Command to run.""")
    return parser


if __name__ == "__main__":
    sys.exit(intercept_build())
