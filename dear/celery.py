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
from celery import Celery

app = Celery('tasks', broker='amqp://guest@localhost//',
             backend='rpc://', include=['dear.exec'])


if __name__ == '__main__':
    app.start()
