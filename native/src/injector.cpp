// native/src/injector.cpp
// C | arm64-v8a standalone PIE executable
// Injects a shared library into a running process via ptrace.
// Usage: ./mc_injector <pid> </data/local/tmp/libcheat.so>
// Requires root + selinux permissive (or adequate policy)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/uio.h>

#ifndef NT_PRSTATUS
#define NT_PRSTATUS 1
#endif

// ── ARM64 register set ────────────────────────────────────────────────────────
typedef struct {
    uint64_t regs[31]; // x0-x30
    uint64_t sp;
    uint64_t pc;
    uint64_t pstate;
} arm64_regs;

static int regs_get(pid_t pid, arm64_regs* r) {
    struct iovec iov = { r, sizeof(*r) };
    return ptrace(PTRACE_GETREGSET, pid, (void*)(uintptr_t)NT_PRSTATUS, &iov);
}
static int regs_set(pid_t pid, arm64_regs* r) {
    struct iovec iov = { r, sizeof(*r) };
    return ptrace(PTRACE_SETREGSET, pid, (void*)(uintptr_t)NT_PRSTATUS, &iov);
}

// ── Write bytes into remote process ──────────────────────────────────────────
static int mem_write(pid_t pid, uintptr_t addr, const void* src, size_t len) {
    const uint8_t* p = (const uint8_t*)src;
    size_t i = 0;
    for (; i + 8 <= len; i += 8) {
        uint64_t w; memcpy(&w, p + i, 8);
        if (ptrace(PTRACE_POKEDATA, pid, (void*)(addr + i), (void*)w) < 0) return -1;
    }
    if (i < len) {
        errno = 0;
        uint64_t w = (uint64_t)ptrace(PTRACE_PEEKDATA, pid, (void*)(addr + i), NULL);
        if (errno) return -1;
        memcpy(&w, p + i, len - i);
        if (ptrace(PTRACE_POKEDATA, pid, (void*)(addr + i), (void*)w) < 0) return -1;
    }
    return 0;
}

// ── Find library base in /proc/<pid>/maps ────────────────────────────────────
static uintptr_t maps_base(pid_t pid, const char* lib) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);
    FILE* fp = fopen(path, "r");
    if (!fp) return 0;
    char line[512]; uintptr_t base = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, lib) && strstr(line, "r-xp")) {
            sscanf(line, "%lx-", &base); break;
        }
    }
    fclose(fp);
    return base;
}

// ── Compute remote address of a local symbol ─────────────────────────────────
static uintptr_t remote_fn(pid_t pid, const char* lib, void* local_fn) {
    uintptr_t lb = maps_base(getpid(), lib);
    uintptr_t rb = maps_base(pid,     lib);
    if (!lb || !rb) return 0;
    return rb + ((uintptr_t)local_fn - lb);
}

// ── Execute function in remote process via ptrace ────────────────────────────
// Sets pc=func, lr=brk_addr (BRK #0), waits for SIGTRAP on return.
static uintptr_t call_remote(pid_t pid,
                              uintptr_t func,
                              uintptr_t a0, uintptr_t a1,
                              uintptr_t a2, uintptr_t a3,
                              uintptr_t sp, uintptr_t brk_addr) {
    arm64_regs r;
    if (regs_get(pid, &r) < 0) { perror("regs_get"); return 0; }
    arm64_regs saved = r;

    r.regs[0] = a0; r.regs[1] = a1;
    r.regs[2] = a2; r.regs[3] = a3;
    r.regs[30] = brk_addr;
    r.sp = sp;
    r.pc = func;
    regs_set(pid, &r);
    ptrace(PTRACE_CONT, pid, NULL, NULL);

    int status;
    for (;;) {
        waitpid(pid, &status, 0);
        if (!WIFSTOPPED(status)) {
            fprintf(stderr, "[!] Process died during call\n"); return 0;
        }
        int sig = WSTOPSIG(status);
        if (sig == SIGTRAP || sig == SIGILL || sig == SIGSEGV || sig == SIGBUS) break;
        ptrace(PTRACE_CONT, pid, NULL, (void*)(uintptr_t)sig);
    }

    arm64_regs ret_r; regs_get(pid, &ret_r);
    uintptr_t ret = ret_r.regs[0];
    regs_set(pid, &saved);
    return ret;
}

// ── Main injection ───────────────────────────────────────────────────────────
static int inject(pid_t pid, const char* so_path) {
    printf("[*] Attaching to PID %d...\n", pid);
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0) {
        perror("[!] PTRACE_ATTACH"); return -1;
    }
    int status; waitpid(pid, &status, 0);
    if (!WIFSTOPPED(status)) {
        fprintf(stderr, "[!] Process did not stop\n");
        ptrace(PTRACE_DETACH, pid, NULL, NULL); return -1;
    }
    printf("[+] Attached (sig %d)\n", WSTOPSIG(status));

    arm64_regs orig; regs_get(pid, &orig);

    // Locate mmap + dlopen in target
    const char* dl_libs[] = { "libdl.so", "libdl-android.so", "libc.so", NULL };
    uintptr_t remote_mmap = 0, remote_dlopen = 0;
    for (int i = 0; dl_libs[i] && !remote_mmap; i++) {
        remote_mmap   = remote_fn(pid, dl_libs[i], (void*)mmap);
        remote_dlopen = remote_fn(pid, dl_libs[i], (void*)dlopen);
    }
    if (!remote_mmap || !remote_dlopen) {
        fprintf(stderr, "[!] mmap(0x%lx) or dlopen(0x%lx) not found\n",
                remote_mmap, remote_dlopen);
        ptrace(PTRACE_DETACH, pid, NULL, NULL); return -1;
    }
    printf("[+] mmap=0x%lx  dlopen=0x%lx\n", remote_mmap, remote_dlopen);

    uintptr_t rsp = (orig.sp - 0x1000) & ~0xFULL;

    // Allocate scratch RWX page in target
    uintptr_t scratch = call_remote(pid, remote_mmap,
        0, 0x1000, PROT_READ|PROT_WRITE|PROT_EXEC,
        MAP_PRIVATE|MAP_ANONYMOUS, rsp, 0);

    if (!scratch || scratch == (uintptr_t)-1) {
        // Retry without EXEC (SELinux execmem denial)
        printf("[~] EXEC denied, retrying RW only...\n");
        scratch = call_remote(pid, remote_mmap,
            0, 0x1000, PROT_READ|PROT_WRITE,
            MAP_PRIVATE|MAP_ANONYMOUS, rsp, 0);
    }
    if (!scratch || scratch == (uintptr_t)-1) {
        fprintf(stderr, "[!] Remote mmap failed\n");
        ptrace(PTRACE_DETACH, pid, NULL, NULL); return -1;
    }
    printf("[+] Scratch @ 0x%lx\n", scratch);

    // Plant BRK #0 (0xD4200000) as return trap
    uint32_t brk_insn = 0xD4200000u;
    uintptr_t brk_addr = scratch + 0xF00;
    mem_write(pid, brk_addr, &brk_insn, 4);

    // Write .so path at scratch+0
    size_t path_len = strlen(so_path) + 1;
    if (path_len > 0xF00) { fprintf(stderr, "[!] Path too long\n"); return -1; }
    if (mem_write(pid, scratch, so_path, path_len) < 0) {
        fprintf(stderr, "[!] mem_write path failed\n");
        ptrace(PTRACE_DETACH, pid, NULL, NULL); return -1;
    }

    // Call dlopen(path, RTLD_NOW|RTLD_GLOBAL)
    uintptr_t handle = call_remote(pid, remote_dlopen,
        scratch, (uintptr_t)(RTLD_NOW|RTLD_GLOBAL),
        0, 0, rsp, brk_addr);

    if (!handle)
        fprintf(stderr, "[!] dlopen returned NULL — check .so path and ABI\n");
    else
        printf("[+] dlopen handle: 0x%lx\n[+] Injection OK\n", handle);

    regs_set(pid, &orig);
    ptrace(PTRACE_DETACH, pid, NULL, NULL);
    printf("[+] Detached\n");
    return handle ? 0 : -1;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <pid> <path/to/libcheat.so>\n", argv[0]);
        return 1;
    }
    pid_t pid = (pid_t)atoi(argv[1]);
    if (pid <= 0) { fprintf(stderr, "[!] Bad PID\n"); return 1; }
    if (access(argv[2], R_OK) < 0) {
        fprintf(stderr, "[!] Cannot read %s: %s\n", argv[2], strerror(errno));
        return 1;
    }
    return inject(pid, argv[2]) == 0 ? 0 : 1;
}
