#include <common.h>
#include <stdio.h>
#include <isa.h>
#include <string.h>

#define GPR_LEN 16

static word_t gpr_pre[GPR_LEN];
static word_t pc_pre=0x80000000;

FILE* gsnapshot_fp;

#define gsnapshot_write(...) do { \
    if(gsnapshot_fp){ \
        fprintf(gsnapshot_fp, __VA_ARGS__); \
        fflush(gsnapshot_fp); \
    } \
  } while (0) \

void init_gpr_snapshot(const char *snapshot_file){
    gsnapshot_fp = fopen(snapshot_file, "w");
    if(gsnapshot_fp==NULL){
        Log("gsnapshot disabled");
        return;
    }
    Assert(gsnapshot_fp, "Can not open '%s'", snapshot_file);
    Log("gsnapshot is written to %s", snapshot_file);
}

void update_gpr_snapshot(){
    if(gsnapshot_fp==NULL) return;
    if(cpu.pc!=pc_pre+4){
        gsnapshot_write("%x %x\n", GPR_LEN, cpu.pc);
    }
    pc_pre = cpu.pc;
    if(memcmp(cpu.gpr, gpr_pre, sizeof(gpr_pre))==0) return;
    for(int i=0;i<GPR_LEN;++i)
        if(cpu.gpr[i]!=gpr_pre[i]){
            gsnapshot_write("%x %x\n", i, cpu.gpr[i]);
        }
    memcpy(gpr_pre, cpu.gpr, sizeof(gpr_pre));
}

void update_gpr_snapshot_record_store(uint32_t addr, word_t val){
    if(gsnapshot_fp==NULL) return;
    gsnapshot_write("%x %x\n", addr, val);
}

