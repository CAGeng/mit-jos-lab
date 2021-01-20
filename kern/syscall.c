/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/syscall.h>
#include <kern/console.h>

// Print a string to the system console.
// The string is exactly 'len' characters long.
// Destroys the environment on memory errors.
static void sys_show_environments(){
	cprintf("Env show:\n");
    for(int i = 0; i <NENV; i++){
		if(envs[i].env_status == ENV_FREE)continue;
		struct Env e = envs[i];		
        cprintf("id: %d\n",envs[i].env_id);
		cprintf("parent_id: %d\n",envs[i].env_parent_id);
		cprintf("Registers:\n");
		cprintf("cs: 0x%0x\teip: 0x%0x\tesp: 0x%0x\t\n",e.env_tf.tf_cs, e.env_tf.tf_eip, e.env_tf.tf_esp);
    }
}

static void
sys_cputs(const char *s, size_t len)
{
	// Check that the user has permission to read memory [s, s+len).
	// Destroy the environment if not.

	// LAB 3: Your code here.
	unsigned cur_va = ROUNDDOWN((unsigned)s, PGSIZE);
    unsigned end_va = ROUNDUP((unsigned)s + len, PGSIZE);
    while(cur_va < end_va){
        //判页目录表项 
        pde_t *pgdir = curenv->env_pgdir;
        pde_t pgdir_entry = pgdir[PDX(cur_va)];
        if(!(pgdir_entry & PTE_U)){
            env_destroy(curenv);
			return;
		}
        
		//判页表项
        pte_t *pg_address = KADDR(PTE_ADDR(pgdir_entry));
        pte_t pg_entry = pg_address[PTX(cur_va)];
        if(!(pg_entry & PTE_U)){
            env_destroy(curenv);
			return;
		}
        cur_va += PGSIZE;
	}
	// Print the string supplied by the user.
	cprintf("%.*s", len, s);
}

// Read a character from the system console without blocking.
// Returns the character, or 0 if there is no input waiting.
static int
sys_cgetc(void)
{
	return cons_getc();
}

// Returns the current environment's envid.
static envid_t
sys_getenvid(void)
{
	return curenv->env_id;
}

// Destroy a given environment (possibly the currently running environment).
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_destroy(envid_t envid)
{
	int r;
	struct Env *e;

	if ((r = envid2env(envid, &e, 1)) < 0)
		return r;
	if (e == curenv)
		cprintf("[%08x] exiting gracefully\n", curenv->env_id);
	else
		cprintf("[%08x] destroying %08x\n", curenv->env_id, e->env_id);
	env_destroy(e);
	return 0;
}

// Dispatches to the correct kernel function, passing the arguments.
int32_t
syscall(uint32_t syscallno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
	// Call the function corresponding to the 'syscallno' parameter.
	// Return any appropriate return value.
	// LAB 3: Your code here.

	switch (syscallno) {
		case SYS_cputs:
            sys_cputs((char *)a1, a2);
            return 0;
        case SYS_cgetc:
            return sys_cgetc();
        case SYS_getenvid:
            return sys_getenvid();
        case SYS_env_destroy:
            return sys_env_destroy(a1);
		case SYS_show_environments:
			sys_show_environments();
			return 0;
		default:
			return -E_INVAL;
	}
}

