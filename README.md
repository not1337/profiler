                profiler, (c) 2019 Andreas Steinmetz

--------------------------------------------------------------------------


An easy to use and very simple Linux profiling utility using gcc instrumentation
================================================================================

All profiling tools I came across, be it gprof, the oprofile suite, valgrind
or anything else do either not output the information I do need, are too
complex, require special kernel configuration or some other inconvenient
stuff. By now I do believe profiling tool developers do only have large
installations in mind when after all most people don't have a dedicated
test center with properly set up test systems.

Sounds like I do want very special things. Not so. Really. I just do want
some basic stuff:

1. No system configuration change necessary.
2. No source code modification (including an extra header once is ok).
3. Tolerable measurement overhead (you can't help syscall delays).
4. Usable measurement data, including sufficiently fine grained CPU usage.
5. Runtime measurement configuration, e.g. measurement output pathname.

The measurement data should include:

1. Total and average CPU usage per function.
2. Total amount of calls for every function.
3. Per thread information.
4. Complete or selectable call trees.
5. Overall summary.

So I did write this little profiler which actually uses the instrumentation
feature of gcc and only consists of a header (ahem) to be included on one of
the sources to be profiled. Then recompile all sources with some
instrumentation options (see the header file for details) and you are
ready to go. Run your application and after application termination
pass the instrumentation output to the profiler utility to see the profiling
results. Well, the profiler utility depends on "addr2line" from binutils
to be installed and reside in a directory contained in PATH.

The profiling code heavily depends on clock\_gettime and the output should
be corrected for the time this system call takes (no, vDSO will not work,
thread CPU time is required), so a tiny measurement tool is included.

The whole resulting profiler package thus consists of only 3 files:

1. profiler.h - profiling header to be included in one source
2. profiler - profiling output processing utillity, requires addr2line
3. profadj - clock\_gettime delay measurement utility

The profiler utility code is not a beauty, but well, it works. It will most
probably do strange things if the instrumentation file does not match the
profiled application or any of its libraries, but then that's your fault.
