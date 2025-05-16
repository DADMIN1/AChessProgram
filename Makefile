#Invoke Make with 'DEBUG=1' to build a Debug version; "make DEBUG=1"
#Asserts are now NOT removed by default (unless debugging). Pass 'RELEASE=1' to disable them
# call make with '-n' option to dry-run

PROJECT_DIR := $(shell realpath .)

DEBUG_BUILD_DIR := $(PROJECT_DIR)/build_DEBUG #Must define seperately for clean command
RELEASE_BUILD_DIR := $(PROJECT_DIR)/build_RELEASE #We need a seperate build directory for this because EVERY file in the project has to be built/rebuilt with or without asserts; otherwise we'd be forced to either re-compile everything or get an incorrect compilation
MAKEFLAGS += -j
#MAKEFLAGS += -j8 #causes eight(identical) make-jobs to be spawned
#MAKEFLAGS += --output-sync=target #ensures that only one job is printing output at a time; and is only displayed after the entire recipie is completed
MAKEFLAGS += --warn-undefined-variables

ifdef DEBUG
#BUILD_DIR := $(DEBUG_BUILD_DIR) #can't do this, for some reason
BUILD_DIR := $(PROJECT_DIR)/build_DEBUG
TARGET_EXEC := Chess_DEBUG
else
	ifdef RELEASE
	BUILD_DIR := $(PROJECT_DIR)/build_RELEASE
	TARGET_EXEC := Chess_RELEASE
	else
	BUILD_DIR := $(PROJECT_DIR)/build
	TARGET_EXEC := Chess
	endif
endif

#SRC_DIRS := ./
SRC_DIRS := $(PROJECT_DIR)
SRC_DIRS += $(PROJECT_DIR)/Source
#SRC_DIRS += $(shell find $(PROJECT_DIR)/Source -mindepth 1 -type d -printf "%P\n")
SRC_DIRS += $(shell find $(PROJECT_DIR)/Source -mindepth 1 -type d -printf "$(PROJECT_DIR)/Source/%P\n")
# finding all subdirectories under 'Source'. mindepth=1 prevents it from printing an empty line (for ./Source itself)
# the final part of the line appends the result ("%P") to "./Source/", and then seperates them with a space (instead of "\n")

#BUILD_SUBDIRS := $(patsubst $(PROJECT_DIR)%, ./build%, $(SRC_DIRS))
BUILD_SUBDIRS := $(patsubst $(PROJECT_DIR)%,$(BUILD_DIR)%,$(SRC_DIRS))
# This creates a subdirectory in BUILD_SUBDIR for each source-subdirectory through text-replacement (output includes the BUILD_DIR itself)
# Both lines work; the first is more readable, but the second should be better

# Find all the C and C++ files we want to compile
# Note the single quotes around the * expressions. Make will incorrectly expand these otherwise.
# Every .cpp and .o file, EVEN THOSE IN SUBDIRECTORIES, will be included by these commands
SRCS := $(shell find $(PROJECT_DIR) -mindepth 1 -name '*.cpp' -printf "%P\n")
# The last part of the command (printf...) causes the filenames to be printed/returned without the path. This is important because otherwise you'd get doubled '//' or './' during the compile step
# Maxdepth 1 is to temporarily ignore the subfolders (only .cpp files in parent folder)
#SRCS := $(shell find $(SRC_DIRS) -mindepth 1 -maxdepth 1 -name '*.cpp' -printf "%P\n") #this is functionally the same as using "%F\n"
# "%P" includes the relative path, starting at the given directory; instead of "%F", which truncates the ENTIRE path.

# String substitution for every C/C++ file.
# As an example, hello.cpp turns into ./build/hello.cpp.o
OBJS := $(SRCS:%.cpp=$(BUILD_DIR)/%.o)

# String substitution (suffix version without %).
# As an example, ./build/hello.cpp.o turns into ./build/hello.cpp.d
DEPS := $(OBJS:.o=.d)

#minimum version for "include enum" declarations is g++11
CXX := g++-13

# adding subdirectories as paths to search for includes; -iquote only applies to includes that use: "x.h" instead of: <x.h>
CPPFLAGS := $(addprefix -iquote , $(SRC_DIRS))
# this makes the output totally unreadable, but whatever.

# additional headers for Thor/Aurora/effolkronium; -isystem only applies to unquoted includes: <x.h>
CPPFLAGS += -isystem "${PROJECT_DIR}/Bromeon/" -isystem "${PROJECT_DIR}/"

# The -MMD and -MP flags together create dependency-files for us! Those additional output files will have .d instead of .o as the output.
# -H flag traces/prints the included headers recursively #--trace does the same thing? There is literally no documentation on these anywhere
# -H/--trace forces compiling and linking -Hf is an option, but it doesn't do anything (maybe because the last input has no output and overwrites the output file?)
# It's output is on cerr? To pipe it to a file, use the command:
# make >>/dev/null 2>>testOutput.txt

#use "-std=c++2a" instead of "-std=c++20" for g++ versions 9 and lower
#CPPFLAGS += -MMD -MP -std=c++20 -fmax-errors=1 -pipe -march=native -mtune=native -pthread
CPPFLAGS += -MMD -MP -std=c++23 -pipe -march=native -mtune=native -pthread
# "pthread" is required when you use std::thread. Yes, it really is a compiler-option; it sets the flags for both the preprocessor and linker.
# "pipe" option just makes the compilation faster; it tells gcc to use pipes instead of temp-files.
# "-march=native" option generates code optimized for your CPU's architecture. Obviously non-portable.
# "-fmax-errors=1" stops the script after any error. Without it, the makefile would continue and probably spam hundreds of error messages.

#optimizations disabled by default, just to keep the compile times shorter
# Measure compile-speed by running the "time" command: e.g: "time make DEBUG=1"
ifdef RELEASE
	CPPFLAGS += -Ofast #highest performance, longest compile-times, least predictable/potentially breaking code
else ifdef DEBUG
	CPPFLAGS += -Og #Uses same optimizations as O1, but disables anything that would interfere with debugging.
else
	#CPPFLAGS += -O1 #same compile-times as without optimization, but significantly smaller and faster binary
	#CPPFLAGS += -O2 #Recommended level; increases compile times by ~10%?
	CPPFLAGS += -O3 #Increases compile-times by ~10% again. Theoretically makes loops much more efficient than -O2 (because it enables -ftree-vectorize).
endif

#-MF option outputs to a file, but it overwrites the file instead of concatenating. Doesn't force a compile.
#g++-10 -std=c++2a -M Pieces.cpp -MF testOutput.txt
#You can also do it on .hpp files, but the only difference is that it doesn't include the .hpp file of the same name

ifdef DEBUG
CPPFLAGS += -g
endif

ifdef RELEASE
CPPFLAGS += -DNDEBUG#-Dxxx flag = #define xxx. Defining NDEBUG causes all assert statements to be removed from code.
endif

# the order that SFML is linked is important! sfml-system must be last!
LDFLAGS := -lsfml-graphics -lsfml-window -lsfml-audio# -lsfml-network
LDFLAGS += -lsfml-system
LDFLAGS += -lthor
#LDFLAGS += -pg #generates profiling information. Also has to be added to CPPFLAGS???
##### I don't seem to have debug libraries compiled/installed for anything, or static libraries. Whatever.

# linker flags so that libthor.so can be found
LDFLAGS += -Wl,-rpath,${PROJECT_DIR}

#http://gcc.gnu.org/onlinedocs/gccint/LTO-Overview.html#LTO-Overview
# http://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html
# this flag enables the link-time optimizer; allowing code optimizations to be performed across code files.
# these must be declared AFTER the -O options? LDflags+= can't be put any earlier because the previous block declares it
#CPPFLAGS += -flto=auto -fuse-linker-plugin -ffat-lto-objects
CPPFLAGS += -flto=auto -fuse-linker-plugin -fno-fat-lto-objects
LDFLAGS += -flto=auto -fuse-linker-plugin

# flto=auto causes the "flto" command to autodetect Make's job server. This is very important because we're using recursive and parallel make-calls / jobs
# ffat-lto-objects: Fat LTO objects are object files that contain both the intermediate language and the object code. This makes them usable for both LTO linking and normal linking. Also required for creating static or dynamic libraries?? And for some debug info?? This option is effective only when compiling with -flto and is ignored at link time. Disabled by default.
# fno-fat-lto-objects It compiles faster, but it makes normal linking (at least one file compiled without the lto flag) imposissble. Linking with -flto should produce exactly the same result regardless of whether the source files were compiled with 'ffat-lto-objects'.
# fat-LTO object-files are required for objdump to display anything useful.
# -fuse-linker-plugin - enables LTO

CPPFLAGS += -Wall -Wextra -Wpedantic# -Wno-sign-compare -Werror
#"Wall" and "Wextra" enables all warnings, "Werror" treats warnings as errors
#CPPFLAGS += -Wno-unused-result #supressing unused-result warnings because they trigger on calls to 'system("clear")'
#CPPFLAGS += -pg #-pg generates profiling information #also has to be added to ldflags???


# just reruns the "make" command (targeting symlink) as "time make"; which just outputs the duration of the script's runtime
.PHONY: timing
timing:
	bash -c "time $(MAKE) symlink"

.PHONY: all
all:
	bash -c "time $(MAKE) symlink"
	bash -c "time $(MAKE) DEBUG=1 symlink"
	bash -c "time $(MAKE) RELEASE=1 symlink"
# we can't just list all the symlinks as dependencies to the "all" recipe/rule because then the Debug/Release builds would be compiled without the proper flags and paths.
# it works properly; only recompiling files with updated dependencies.
# we specify the target "symlink" because otherwise it defaults to the "timing" target/recipe, which recursively calls the script again, kind of spamming the output.
# The "run" symbolic-link will always point to Chess_RELEASE after running this command, assuming it doesn't error. "Chess" is easy to type anyway, and you probably don't want to run the debug-build.


# create a symbolic link to the executable that was just compiled, always named "run", regardless of actual program's name
# also create a symlink matching the executable's name, so it can be specifically chosen even if it wasn't the most recently built binary
.DEFAULT_GOAL := symlink
symlink: $(BUILD_DIR)/$(TARGET_EXEC)
	ln --symbolic -f $(BUILD_DIR)/$(TARGET_EXEC) $(PROJECT_DIR)/run
	ln --symbolic -f $(BUILD_DIR)/$(TARGET_EXEC) $(PROJECT_DIR)/$(TARGET_EXEC)

#.DEFAULT_GOAL := $(BUILD_DIR)/$(TARGET_EXEC)
# The final build step; linking obj files
$(BUILD_DIR)/$(TARGET_EXEC): $(BUILD_SUBDIRS) $(OBJS)
	$(CXX) -L${PROJECT_DIR} $(CPPFLAGS) $(OBJS) -o $@ $(LDFLAGS)

# Adding Makefile as a dependency forces the entire project to be rebuilt when this file is modified
# Build step for C++ source
$(BUILD_DIR)/%.o: %.cpp Makefile
	$(CXX) $(CPPFLAGS) -c $< -o $@

#it should be this instead?
#$(BUILD_DIR)/%.cpp.o: %.cpp
#	$(CXX) $(CPPFLAGS) -c $< -o $@

# make the subdirs if they don't exist
$(BUILD_SUBDIRS): Makefile
	mkdir -p $(BUILD_SUBDIRS)
#this gets run three or four times when multithreading is enabled; doesn't really matter.

.PHONY: clean
clean:
	@-rm --recursive --verbose "build/" 2> /dev/null || true
	@-rm --recursive --verbose "build_DEBUG/" 2> /dev/null || true
	@-rm --recursive --verbose "build_RELEASE/" 2> /dev/null || true
	@find ${PROJECT_DIR} -maxdepth 1 -type l -delete
# 'find' command here deletes symlinks
# prefixed '@' prevents make from echoing the command
# prefixed '-' causes make to ignore nonzero exit-codes (instead of aborting), but it still reports these errors:
# 		'make: [Makefile:177: clean] Error 1 (ignored)'
# which is why we've appended '|| true'; it ensures the exit-code is always 0, suppressing those messages
#
# 'rm' (even without verbose) will also print it's own additional error-messages: 
# 		'rm: cannot remove 'fluidsym_dbg': No such file or directory'
# so we pipe to '/dev/null' to suppress that as well

.PHONY: trace
trace: $(OBJS)
	@rm -f $(PROJECT_DIR)/trace.txt
	$(CXX) -std=c++23 -MM *.hpp >> trace.txt
#$(CXX) -std=c++2a -M *.hpp >> trace.txt #this option also lists system libraries, and lists recursively?

.PHONY: printall
printall:
	# printing SRC_DIRS
	@echo "$(SRC_DIRS) \n"
	# printing BUILD_SUBDIRS
	@echo "$(BUILD_SUBDIRS) \n"
	# printing SRCS
	@echo "$(SRCS) \n"
	# printing OBJS
	@echo "$(OBJS) \n"
	# printing DEPS
	@echo "$(DEPS) \n"

#	mkdir -p $^
# $^ returns names of all dependencies

-include $(DEPS)
# Include the .d makefiles. The - at the front suppresses the errors of missing
# Makefiles. Initially, all the .d files will be missing, and we don't want those
# errors to show up.
