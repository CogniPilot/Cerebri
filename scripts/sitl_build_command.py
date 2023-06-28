# Copyright (c) 2019 Foundries.io
# Copyright (c) 2022 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

'''example_west_command.py

Example of a west extension in the example-application repository.'''

import subprocess


from west.commands import WestCommand  # your extension must subclass this
from west import log                   # use this for user output

class SitlBuildCommand(WestCommand):

    def __init__(self):
        super().__init__(
            'sitl_build',       # gets stored as self.name
            'runs sitl build and install',  # self.help
            # self.description:
            '''\
Basic test runner

Runs twister and other tests as necessary
''')

    def do_add_parser(self, parser_adder):
        # This is a bit of boilerplate, which allows you full control over the
        # type of argparse handling you want. The "parser_adder" argument is
        # the return value of an argparse.ArgumentParser.add_subparsers() call.
        parser = parser_adder.add_parser(self.name,
                                         help=self.help,
                                         description=self.description)

        # Add some example options using the standard argparse module API.
        #parser.add_argument('-o', '--optional', help='an optional argument')
        parser.add_argument('app', help='the app directory')

        return parser           # gets stored as self.parser

    def do_run(self, args, unknown_args):
        # This gets called when the user runs the command, e.g.:
        #
        #   $ west my-command-name -o FOO BAR
        #   --optional is FOO
        #   required is BAR
        #log.inf('--optional is', args.optional)
        #log.inf('required is', args.app)

        log.inf('sitl build and install')
        cmd_str = ['west', 'build', '-b', 'native_posix', f'{args.app:s}', '-t', 'install', '-D', 'CMAKE_INSTALL_PREFIX=$HOME']
        subprocess.run(cmd_str, shell=False)
