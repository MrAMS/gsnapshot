#include "cpu.hpp"
#include "am.hpp"
#include "common.hpp"
#include "isa_rv32e.hpp"
#include "marco.h"
#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <cstdio>

extern AM am;
extern CPU cpu;
CPU cpu_pre;

static FILE* gsnapshot_fp=NULL;
static uint32_t fp_pos;

void gsnapshot_init(const char* file_path){
    if(file_path != NULL)
        gsnapshot_fp = fopen(file_path, "r");
    if(gsnapshot_fp != NULL){
        printf(ANSI_FMT("gsnapshot enabled, ref to %s\n", ANSI_FG_GREEN), file_path);
    }else{
        printf(ANSI_FMT("gsnapshot disabled, can't open ref %s\n", ANSI_FG_YELLOW), file_path);
    }
    cpu_pre.pc = 0x80000000-4;
}

bool gsnapshot_check(){
    if(gsnapshot_fp==NULL)
        return true;
    fseek(gsnapshot_fp, fp_pos, SEEK_SET);
    if(cpu.pc != cpu_pre.pc+4){
        word_t type, expe_pc;
        if(fscanf(gsnapshot_fp, "%x %x\n", &type, &expe_pc)==EOF){
            printf(ANSI_FMT("ref reach end\n", ANSI_FG_RED));
            return false;
        }
        if(type < GPR_LEN){
            printf(ANSI_FMT("expect x%d:%x->%x, get pc:%x->%x\n", ANSI_FG_RED),
                    type, cpu_pre.gpr[type], expe_pc, cpu_pre.pc, cpu.pc);
            return false;
        }else if(type > GPR_LEN){
            printf(ANSI_FMT("expect mem[%x]:%x->%x, get pc:%x->%x\n", ANSI_FG_RED),
                    type, am.pmem_read(TO_PMEM_ADDR(type)), expe_pc, cpu_pre.pc, cpu.pc);
            return false;
        }else if(expe_pc != cpu.pc){
            printf(ANSI_FMT("expect pc:%x->%x, get pc:%x->%x\n", ANSI_FG_RED),
                    cpu_pre.pc, expe_pc, cpu_pre.pc, cpu.pc);
            return false;
        }
        cpu_pre.pc = expe_pc;
    }else{
        cpu_pre.pc += 4;
    }

    for(uint32_t i=0;i<GPR_LEN;++i)
        if(cpu.gpr[i] != cpu_pre.gpr[i]){
            word_t exp_gpr_i, exp_gpr_val;
            if(fscanf(gsnapshot_fp, "%x %x\n", &exp_gpr_i, &exp_gpr_val)==EOF){
                printf(ANSI_FMT("ref reach end\n", ANSI_FG_RED));
                return false;
            }
            if(exp_gpr_i==GPR_LEN){
                printf(ANSI_FMT("expect pc:%x->%x, get x%d:%x->%x\n", ANSI_FG_RED), 
                    cpu_pre.pc, exp_gpr_val, i, cpu_pre.gpr[i], cpu.gpr[i]);
                return false;
            }else if(exp_gpr_i>GPR_LEN){
                printf(ANSI_FMT("expect mem[%x]:%x->%x, get x%d:%x->%x\n", ANSI_FG_RED),
                    exp_gpr_i, am.pmem_read(TO_PMEM_ADDR(exp_gpr_i)), exp_gpr_val, i, cpu_pre.gpr[i], cpu.gpr[i]);
                return false;
            }else if(exp_gpr_i==i&&exp_gpr_val==cpu.gpr[i]){
                cpu_pre.gpr[i] = exp_gpr_val;
            }else{
                printf(ANSI_FMT("expect x%d:%x->%x, get x%d:%x->%x\n", ANSI_FG_RED),
                    exp_gpr_i, cpu_pre.gpr[exp_gpr_i], exp_gpr_val, i, cpu_pre.gpr[i], cpu.gpr[i]);
                return false;
            }
        }
    fp_pos = ftell(gsnapshot_fp);
    return true;
}

void gsnapshot_check_store(uint32_t addr, word_t val){
    if(gsnapshot_fp==NULL)
        return;
    fseek(gsnapshot_fp, fp_pos, SEEK_SET);
    word_t ref_addr, ref_val;
    if(fscanf(gsnapshot_fp, "%x %x\n", &ref_addr, &ref_val)==EOF){
        printf(ANSI_FMT("ref reach end\n", ANSI_FG_RED));
        AM_ASSERT((&am), 0, {return;}, "gsnapshot check fail");
    }
    if(ref_addr<GPR_LEN){
        printf(ANSI_FMT("expect x%d:->%x, get mem[%x]:->%x\n", ANSI_FG_RED),
            ref_addr, ref_val, addr, val);
        AM_ASSERT((&am), 0, {return;}, "gsnapshot check fail");
    }else if(ref_addr==GPR_LEN){
        printf(ANSI_FMT("expect pc:->%x, get mem[%x]:->%x\n", ANSI_FG_RED),
            ref_val, addr, val);
        AM_ASSERT((&am), 0, {return;}, "gsnapshot check fail");
    }else if(ref_addr!=addr||ref_val!=val){
        printf(ANSI_FMT("expect mem[%x]:->%x, get mem[%x]:->%x\n", ANSI_FG_RED),
            ref_addr, ref_val, addr, val);
        AM_ASSERT((&am), 0, {return;}, "gsnapshot check fail");
    }
    fp_pos = ftell(gsnapshot_fp);
}
