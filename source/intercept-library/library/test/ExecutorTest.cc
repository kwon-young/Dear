/*  Copyright (C) 2012-2020 by László Nagy
    This file is part of Bear.

    Bear is a tool to generate compilation database for clang tooling.

    Bear is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Bear is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gtest/gtest.h"

#include "intercept.h"

#include "Executor.h"
#include "ResolverMock.h"
#include "Session.h"

using ::testing::_;
using ::testing::Args;
using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::Matcher;
using ::testing::NotNull;
using ::testing::Return;

namespace {

    constexpr int SUCCESS = 0;
    constexpr int FAILURE = -1;

    char LS_PATH[] = "/usr/bin/ls";
    char LS_FILE[] = "ls";
    char* LS_ARGV[] = {
        const_cast<char*>("/usr/bin/ls"),
        const_cast<char*>("-l"),
        nullptr
    };
    char* LS_ENVP[] = {
        const_cast<char*>("PATH=/usr/bin:/usr/sbin"),
        nullptr
    };
    char SEARCH_PATH[] = "/usr/bin:/usr/sbin";

    ear::Session SILENT_SESSION = {
        "/usr/libexec/libexec.so",
        "/usr/bin/intercept",
        "/tmp/intercept.random",
        false
    };

    ear::Session VERBOSE_SESSION = {
        "/usr/libexec/libexec.so",
        "/usr/bin/intercept",
        "/tmp/intercept.random",
        true
    };


    TEST(Executor, fails_without_env)
    {
        ear::Session session = ear::session::init();

        ResolverMock resolver;
        EXPECT_CALL(resolver, execve(_, _, _)).Times(0);
        EXPECT_CALL(resolver, posix_spawn(_, _, _, _, _, _)).Times(0);
        EXPECT_CALL(resolver, access(_, _)).Times(0);

        EXPECT_EQ(FAILURE, ear::Executor(resolver, session).execve(LS_PATH, LS_ARGV, LS_ENVP));
        EXPECT_EQ(FAILURE, ear::Executor(resolver, session).execvpe(LS_FILE, LS_ARGV, LS_ENVP));
        EXPECT_EQ(FAILURE, ear::Executor(resolver, session).execvP(LS_FILE, SEARCH_PATH, LS_ARGV, LS_ENVP));

        pid_t pid;
        EXPECT_EQ(FAILURE, ear::Executor(resolver, session).posix_spawn(&pid, LS_PATH, nullptr, nullptr, LS_ARGV, LS_ENVP));
        EXPECT_EQ(FAILURE, ear::Executor(resolver, session).posix_spawnp(&pid, LS_FILE, nullptr, nullptr, LS_ARGV, LS_ENVP));
    }

    TEST(Executo, execve_silent_library)
    {
        ResolverMock resolver;
        EXPECT_CALL(resolver, access(LS_PATH, _))
            .Times(1)
            .WillOnce(Return(SUCCESS));

        // TODO: verify the arguments
        //    const char* argv[] = {
        //        session.reporter,
        //        pear::flag::DESTINATION,
        //        session.destination,
        //        pear::flag::LIBRARY,
        //        session.library,
        //        pear::flag::PATH,
        //        LS_PATH,
        //        pear::flag::COMMAND,
        //        LS_ARGV[0],
        //        LS_ARGV[1],
        //        nullptr
        //    };
        EXPECT_CALL(resolver, execve(SILENT_SESSION.reporter, NotNull(), LS_ENVP))
            .Times(1)
            .WillOnce(Return(SUCCESS));

        auto result = ear::Executor(resolver, SILENT_SESSION).execve(LS_PATH, LS_ARGV, LS_ENVP);
        EXPECT_EQ(SUCCESS, result);
    }

    TEST(Executor, execve_verbose_library)
    {
        ResolverMock resolver;
        EXPECT_CALL(resolver, access(LS_PATH, _))
            .Times(1)
            .WillOnce(Return(SUCCESS));

        // TODO: verify the arguments
        //    const char* argv[] = {
        //        session.reporter,
        //        pear::flag::VERBOSE,
        //        pear::flag::DESTINATION,
        //        session.destination,
        //        pear::flag::LIBRARY,
        //        session.library,
        //        pear::flag::PATH,
        //        LS_PATH,
        //        pear::flag::COMMAND,
        //        LS_ARGV[0],
        //        LS_ARGV[1],
        //        nullptr
        //    };
        EXPECT_CALL(resolver, execve(VERBOSE_SESSION.reporter, NotNull(), LS_ENVP))
            .Times(1)
            .WillOnce(Return(SUCCESS));

        auto result = ear::Executor(resolver, VERBOSE_SESSION).execve(LS_PATH, LS_ARGV, LS_ENVP);
        EXPECT_EQ(SUCCESS, result);
    }

    TEST(Executor, execvpe_fails_on_access)
    {
        ResolverMock resolver;
        EXPECT_CALL(resolver, access(LS_PATH, _))
            .Times(1)
            .WillOnce(Return(FAILURE));

        auto result = ear::Executor(resolver, SILENT_SESSION).execve(LS_PATH, LS_ARGV, LS_ENVP);
        EXPECT_EQ(FAILURE, result);
    }

    TEST(Executor, execvpe_passes)
    {
        ResolverMock resolver;
        // TODO: verify the arguments
        //    const char* argv[] = {
        //        SILENT_SESSION.reporter,
        //        pear::flag::VERBOSE,
        //        pear::flag::DESTINATION,
        //        SILENT_SESSION.destination,
        //        pear::flag::LIBRARY,
        //        SILENT_SESSION.library,
        //        pear::flag::FILE,
        //        LS_FILE,
        //        pear::flag::COMMAND,
        //        LS_ARGV[0],
        //        LS_ARGV[1],
        //        nullptr
        //    };
        EXPECT_CALL(resolver, execve(VERBOSE_SESSION.reporter, NotNull(), LS_ENVP))
            .Times(1)
            .WillOnce(Return(SUCCESS));

        auto result = ear::Executor(resolver, VERBOSE_SESSION).execvpe(LS_FILE, LS_ARGV, LS_ENVP);
        EXPECT_EQ(SUCCESS, result);
    }

    TEST(Executor, execvp2_passes)
    {
        ResolverMock resolver;
        // TODO: verify the arguments
        //    const char* argv[] = {
        //        SILENT_SESSION.reporter,
        //        pear::flag::VERBOSE,
        //        pear::flag::DESTINATION,
        //        SILENT_SESSION.destination,
        //        pear::flag::LIBRARY,
        //        SILENT_SESSION.library,
        //        pear::flag::FILE,
        //        LS_FILE,
        //        pear::flag::SEARCH_PATH
        //        SEARCH_PATH
        //        pear::flag::COMMAND,
        //        LS_ARGV[0],
        //        LS_ARGV[1],
        //        nullptr
        //    };
        EXPECT_CALL(resolver, execve(VERBOSE_SESSION.reporter, NotNull(), LS_ENVP))
            .Times(1)
            .WillOnce(Return(SUCCESS));

        auto result = ear::Executor(resolver, VERBOSE_SESSION).execvP(LS_FILE, SEARCH_PATH, LS_ARGV, LS_ENVP);
        EXPECT_EQ(SUCCESS, result);
    }

    TEST(Executor, spawn_passes)
    {
        pid_t pid;

        ResolverMock resolver;
        EXPECT_CALL(resolver, access(LS_PATH, _))
            .Times(1)
            .WillOnce(Return(SUCCESS));

        // TODO: verify the arguments
        //    const char* argv[] = {
        //        VERBOSE_SESSION.reporter,
        //        pear::flag::VERBOSE,
        //        pear::flag::DESTINATION,
        //        VERBOSE_SESSION.destination,
        //        pear::flag::LIBRARY,
        //        VERBOSE_SESSION.library,
        //        pear::flag::PATH,
        //        LS_PATH,
        //        pear::flag::COMMAND,
        //        LS_ARGV[0],
        //        LS_ARGV[1],
        //        nullptr
        //    };
        EXPECT_CALL(resolver, posix_spawn(&pid, VERBOSE_SESSION.reporter, nullptr, nullptr, NotNull(), LS_ENVP))
            .Times(1)
            .WillOnce(Return(SUCCESS));

        auto result = ear::Executor(resolver, VERBOSE_SESSION).posix_spawn(&pid, LS_PATH, nullptr, nullptr, LS_ARGV, LS_ENVP);
        EXPECT_EQ(SUCCESS, result);
    }

    TEST(Executor, spawn_fails_on_access)
    {
        pid_t pid;

        ResolverMock resolver;
        EXPECT_CALL(resolver, access(LS_PATH, _))
            .Times(1)
            .WillOnce(Return(FAILURE));

        auto result = ear::Executor(resolver, VERBOSE_SESSION).posix_spawn(&pid, LS_PATH, nullptr, nullptr, LS_ARGV, LS_ENVP);
        EXPECT_EQ(FAILURE, result);
    }

    TEST(Executor, spawnp_passes)
    {
        pid_t pid;

        ResolverMock resolver;
        // TODO: verify the arguments
        //    const char* argv[] = {
        //        VERBOSE_SESSION.reporter,
        //        pear::flag::VERBOSE,
        //        pear::flag::DESTINATION,
        //        VERBOSE_SESSION.destination,
        //        pear::flag::LIBRARY,
        //        VERBOSE_SESSION.library,
        //        pear::flag::FILE,
        //        LS_FILE,
        //        pear::flag::COMMAND,
        //        LS_ARGV[0],
        //        LS_ARGV[1],
        //        nullptr
        //    };
        EXPECT_CALL(resolver, posix_spawn(&pid, VERBOSE_SESSION.reporter, nullptr, nullptr, NotNull(), LS_ENVP))
            .Times(1)
            .WillOnce(Return(SUCCESS));

        auto result = ear::Executor(resolver, VERBOSE_SESSION).posix_spawnp(&pid, LS_FILE, nullptr, nullptr, LS_ARGV, LS_ENVP);
        EXPECT_EQ(SUCCESS, result);
    }
}