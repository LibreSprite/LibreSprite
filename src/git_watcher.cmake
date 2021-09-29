# git_watcher.cmake
# https://raw.githubusercontent.com/andrew-hardin/cmake-git-version-tracking/master/git_watcher.cmake
#
# Released under the MIT License.
# https://raw.githubusercontent.com/andrew-hardin/cmake-git-version-tracking/master/LICENSE


# This file defines a target that monitors the state of a git repo.
# If the state changes (e.g. a commit is made), then a file gets reconfigured.
# Here are the primary variables that control script behavior:
#
#   PRE_CONFIGURE_FILE (REQUIRED)
#   -- The path to the file that'll be configured.
#
#   POST_CONFIGURE_FILE (REQUIRED)
#   -- The path to the configured PRE_CONFIGURE_FILE.
#
#   GIT_STATE_FILE (OPTIONAL)
#   -- The path to the file used to store the previous build's git state.
#      Defaults to the current binary directory.
#
#   GIT_WORKING_DIR (OPTIONAL)
#   -- The directory from which git commands will be run.
#      Defaults to the directory with the top level CMakeLists.txt.
#
#   GIT_EXECUTABLE (OPTIONAL)
#   -- The path to the git executable. It'll automatically be set if the
#      user doesn't supply a path.
#
#   GIT_FAIL_IF_NONZERO_EXIT (optional)
#   -- Raise a FATAL_ERROR if any of the git commands return a non-zero
#      exit code. This is set to TRUE by default. You can set this to FALSE
#      if you'd like the build to continue even if a git command fails.
#
# DESIGN
#   - This script was designed similar to a Python application
#     with a Main() function. I wanted to keep it compact to
#     simplify "copy + paste" usage.
#
#   - This script is invoked under two CMake contexts:
#       1. Configure time (when build files are created).
#       2. Build time (called via CMake -P).
#     The first invocation is what registers the script to
#     be executed at build time.
#
# MODIFICATIONS
#   You may wish to track other git properties like when the last
#   commit was made. There are two sections you need to modify,
#   and they're tagged with a ">>>" header.

# Short hand for converting paths to absolute.
macro(PATH_TO_ABSOLUTE var_name)
    get_filename_component(${var_name} "${${var_name}}" ABSOLUTE)
endmacro()

# Check that a required variable is set.
macro(CHECK_REQUIRED_VARIABLE var_name)
    if(NOT DEFINED ${var_name})
        message(FATAL_ERROR "The \"${var_name}\" variable must be defined.")
    endif()
    PATH_TO_ABSOLUTE(${var_name})
endmacro()

# Check that an optional variable is set, or, set it to a default value.
macro(CHECK_OPTIONAL_VARIABLE_NOPATH var_name default_value)
    if(NOT DEFINED ${var_name})
        set(${var_name} ${default_value})
    endif()
endmacro()

# Check that an optional variable is set, or, set it to a default value.
# Also converts that path to an abspath.
macro(CHECK_OPTIONAL_VARIABLE var_name default_value)
    CHECK_OPTIONAL_VARIABLE_NOPATH(${var_name} ${default_value})
    PATH_TO_ABSOLUTE(${var_name})
endmacro()

CHECK_REQUIRED_VARIABLE(PRE_CONFIGURE_FILE)
CHECK_REQUIRED_VARIABLE(POST_CONFIGURE_FILE)
CHECK_OPTIONAL_VARIABLE(GIT_STATE_FILE "${CMAKE_BINARY_DIR}/git-state-hash")
CHECK_OPTIONAL_VARIABLE(GIT_WORKING_DIR "${CMAKE_SOURCE_DIR}")
CHECK_OPTIONAL_VARIABLE_NOPATH(GIT_FAIL_IF_NONZERO_EXIT TRUE)

# Check the optional git variable.
# If it's not set, we'll try to find it using the CMake packaging system.
if(NOT DEFINED GIT_EXECUTABLE)
    find_package(Git QUIET REQUIRED)
endif()
CHECK_REQUIRED_VARIABLE(GIT_EXECUTABLE)


set(_state_variable_names
    GIT_RETRIEVED_STATE
    GIT_HEAD_SHA1
    GIT_IS_DIRTY
    GIT_AUTHOR_NAME
    GIT_AUTHOR_EMAIL
    GIT_COMMIT_DATE_ISO8601
    GIT_COMMIT_SUBJECT
    GIT_COMMIT_BODY
    GIT_DESCRIBE
    # >>>
    # 1. Add the name of the additional git variable you're interested in monitoring
    #    to this list.
)



# Macro: RunGitCommand
# Description: short-hand macro for calling a git function. Outputs are the
#              "exit_code" and "output" variables.
macro(RunGitCommand)
    execute_process(COMMAND
        "${GIT_EXECUTABLE}" ${ARGV}
        WORKING_DIRECTORY "${_working_dir}"
        RESULT_VARIABLE exit_code
        OUTPUT_VARIABLE output
        ERROR_VARIABLE stderr
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT exit_code EQUAL 0)
        set(ENV{GIT_RETRIEVED_STATE} "false")

        # Issue 26: git info not properly set
        #
        # Check if we should fail if any of the exit codes are non-zero.
        if(GIT_FAIL_IF_NONZERO_EXIT)
            string(REPLACE ";" " " args_with_spaces "${ARGV}")
            message(FATAL_ERROR "${stderr} (${GIT_EXECUTABLE} ${args_with_spaces})")
        endif()
    endif()
endmacro()



# Function: GetGitState
# Description: gets the current state of the git repo.
# Args:
#   _working_dir (in)  string; the directory from which git commands will be executed.
function(GetGitState _working_dir)

    # This is an error code that'll be set to FALSE if the
    # RunGitCommand ever returns a non-zero exit code.
    set(ENV{GIT_RETRIEVED_STATE} "true")

    # Get whether or not the working tree is dirty.
    RunGitCommand(status --porcelain)
    if(NOT exit_code EQUAL 0)
        set(ENV{GIT_IS_DIRTY} "false")
    else()
        if(NOT "${output}" STREQUAL "")
            set(ENV{GIT_IS_DIRTY} "true")
        else()
            set(ENV{GIT_IS_DIRTY} "false")
        endif()
    endif()

    # There's a long list of attributes grabbed from git show.
    set(object HEAD)
    RunGitCommand(show -s "--format=%h" ${object})
    if(exit_code EQUAL 0)
        set(ENV{GIT_HEAD_SHA1} ${output})
    endif()

    RunGitCommand(show -s "--format=%an" ${object})
    if(exit_code EQUAL 0)
        set(ENV{GIT_AUTHOR_NAME} "${output}")
    endif()

    RunGitCommand(show -s "--format=%ae" ${object})
    if(exit_code EQUAL 0)
        set(ENV{GIT_AUTHOR_EMAIL} "${output}")
    endif()

    RunGitCommand(show -s "--format=%ci" ${object})
    if(exit_code EQUAL 0)
        set(ENV{GIT_COMMIT_DATE_ISO8601} "${output}")
    endif()

    RunGitCommand(show -s "--format=%s" ${object})
    if(exit_code EQUAL 0)
        # Escape quotes
        string(REPLACE "\"" "\\\"" output "${output}")
        set(ENV{GIT_COMMIT_SUBJECT} "${output}")
    endif()

    RunGitCommand(show -s "--format=%b" ${object})
    if(exit_code EQUAL 0)
        if(output)
            # Escape quotes
            string(REPLACE "\"" "\\\"" output "${output}")
            # Escape line breaks in the commit message.
            string(REPLACE "\r\n" "\\r\\n\\\r\n" safe "${output}")
            if(safe STREQUAL output)
                # Didn't have windows lines - try unix lines.
                string(REPLACE "\n" "\\n\\\n" safe "${output}")
            endif()
        else()
            # There was no commit body - set the safe string to empty.
            set(safe "")
        endif()
        set(ENV{GIT_COMMIT_BODY} "\"${safe}\"")
    else()
        set(ENV{GIT_COMMIT_BODY} "\"\"") # empty string.
    endif()

    # Get output of git describe
    RunGitCommand(describe --always ${object})
    if(NOT exit_code EQUAL 0)
        set(ENV{GIT_DESCRIBE} "unknown")
    else()
        set(ENV{GIT_DESCRIBE} "${output}")
    endif()

    # >>>
    # 2. Additional git properties can be added here via the
    #    "execute_process()" command. Be sure to set them in
    #    the environment using the same variable name you added
    #    to the "_state_variable_names" list.

endfunction()



# Function: GitStateChangedAction
# Description: this function is executed when the state of the git
#              repository changes (e.g. a commit is made).
function(GitStateChangedAction)
    foreach(var_name ${_state_variable_names})
        set(${var_name} $ENV{${var_name}})
    endforeach()
    configure_file("${PRE_CONFIGURE_FILE}" "${POST_CONFIGURE_FILE}" @ONLY)
endfunction()



# Function: HashGitState
# Description: loop through the git state variables and compute a unique hash.
# Args:
#   _state (out)  string; a hash computed from the current git state.
function(HashGitState _state)
    set(ans "")
    foreach(var_name ${_state_variable_names})
        string(SHA256 ans "${ans}$ENV{${var_name}}")
    endforeach()
    set(${_state} ${ans} PARENT_SCOPE)
endfunction()



# Function: CheckGit
# Description: check if the git repo has changed. If so, update the state file.
# Args:
#   _working_dir    (in)  string; the directory from which git commands will be ran.
#   _state_changed (out)    bool; whether or no the state of the repo has changed.
function(CheckGit _working_dir _state_changed)

    # Get the current state of the repo.
    GetGitState("${_working_dir}")

    # Convert that state into a hash that we can compare against
    # the hash stored on-disk.
    HashGitState(state)

    # Issue 14: post-configure file isn't being regenerated.
    #
    # Update the state to include the SHA256 for the pre-configure file.
    # This forces the post-configure file to be regenerated if the
    # pre-configure file has changed.
    file(SHA256 ${PRE_CONFIGURE_FILE} preconfig_hash)
    string(SHA256 state "${preconfig_hash}${state}")

    # Check if the state has changed compared to the backup on disk.
    if(EXISTS "${GIT_STATE_FILE}")
        file(READ "${GIT_STATE_FILE}" OLD_HEAD_CONTENTS)
        if(OLD_HEAD_CONTENTS STREQUAL "${state}")
            # State didn't change.
            set(${_state_changed} "false" PARENT_SCOPE)
            return()
        endif()
    endif()

    # The state has changed.
    # We need to update the state file on disk.
    # Future builds will compare their state to this file.
    file(WRITE "${GIT_STATE_FILE}" "${state}")
    set(${_state_changed} "true" PARENT_SCOPE)
endfunction()



# Function: SetupGitMonitoring
# Description: this function sets up custom commands that make the build system
#              check the state of git before every build. If the state has
#              changed, then a file is configured.
function(SetupGitMonitoring)
    add_custom_target(check_git
        ALL
        DEPENDS ${PRE_CONFIGURE_FILE}
        BYPRODUCTS
            ${POST_CONFIGURE_FILE}
            ${GIT_STATE_FILE}
        COMMENT "Checking the git repository for changes..."
        COMMAND
            ${CMAKE_COMMAND}
            -D_BUILD_TIME_CHECK_GIT=TRUE
            -DGIT_WORKING_DIR=${GIT_WORKING_DIR}
            -DGIT_EXECUTABLE=${GIT_EXECUTABLE}
            -DGIT_STATE_FILE=${GIT_STATE_FILE}
            -DPRE_CONFIGURE_FILE=${PRE_CONFIGURE_FILE}
            -DPOST_CONFIGURE_FILE=${POST_CONFIGURE_FILE}
            -DGIT_FAIL_IF_NONZERO_EXIT=${GIT_FAIL_IF_NONZERO_EXIT}
            -P "${CMAKE_CURRENT_LIST_FILE}")
endfunction()



# Function: Main
# Description: primary entry-point to the script. Functions are selected based
#              on whether it's configure or build time.
function(Main)
    if(_BUILD_TIME_CHECK_GIT)
        # Check if the repo has changed.
        # If so, run the change action.
        CheckGit("${GIT_WORKING_DIR}" changed)
        if(changed OR NOT EXISTS "${POST_CONFIGURE_FILE}")
            GitStateChangedAction()
        endif()
    else()
        # >> Executes at configure time.
        SetupGitMonitoring()
    endif()
endfunction()

# And off we go...
Main()
