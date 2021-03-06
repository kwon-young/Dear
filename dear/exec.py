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
""" This module defines celery tasks that implement the corresponding
posix execution function in python.
"""
from .celery import app


@app.task
def add(x, y):
    return x + y


@app.task(ignore_result=True)
def hello(name, age):
    print('Hello {}. You are {} years old.'.format(name, age))
