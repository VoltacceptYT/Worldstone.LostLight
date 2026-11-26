// Link stubs for libc functions BlocksDS does not provide.
extern "C" int gettimeofday(void*, void*) { return -1; }
