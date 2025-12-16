#ifndef _DB_DISASM_X86_H
#define _DB_DISASM_X86_H

typedef int      boolean_t;
typedef uint64_t db_addr_t;

db_addr_t db_disasm(db_addr_t loc, int altfmt);
db_addr_t db_disasm_64(db_addr_t loc, boolean_t altfmt);

struct inst {
        const char *i_name;         /* name */
        short       i_has_modrm;    /* has regmodrm byte */
        short       i_size;         /* operand size */
        int         i_mode;         /* addressing modes */
        const char *i_extra;        /* pointer to extra opcode table */
};

struct finst {
        const char *f_name;             /* name for memory instruction */
        int         f_size;             /* size for memory instruction */
        int         f_rrmode;           /* mode for rr instruction */
        const char *f_rrname;           /* name for rr instruction */
                                        /* (or pointer to table) */
};

/* These structures are common between db_disasm.c and db_disasm_64.c */
extern const char         * const db_Grp6[];
extern const char         * const db_Grp7[];
extern const char         * const db_Grp8[];
extern const char         * const db_Grp9[];
extern const char         * const db_Grp10[];
extern const char         * const db_Grp11[];
extern const char         * const db_Grp12[];
extern const struct inst          db_inst_0f0x[];
extern const struct inst          db_inst_0f2x[];
extern const struct inst          db_inst_0f8x[];
extern const struct inst          db_inst_0f9x[];
extern const struct inst          db_inst_0fax[];
extern const struct inst          db_inst_0fbx[];
extern const struct inst          db_inst_0fcx[];
extern const struct inst  * const db_inst_0f[];
extern const char         * const db_Esc92[];
extern const char         * const db_Esc93[];
extern const char         * const db_Esc94[];
extern const char         * const db_Esc95[];
extern const char         * const db_Esc96[];
extern const char         * const db_Esc97[];
extern const char         * const db_Esca4[];
extern const char         * const db_Escb4[];
extern const char         * const db_Esce3[];
extern const char         * const db_Escf4[];
extern const struct finst         db_Esc8[];
extern const struct finst         db_Esc9[];
extern const struct finst         db_Esca[];
extern const struct finst         db_Escb[];
extern const struct finst         db_Escc[];
extern const struct finst         db_Escd[];
extern const struct finst         db_Esce[];
extern const struct finst         db_Escf[];
extern const struct finst * const db_Esc_inst[];
extern const char         * const db_Grp1[];
extern const char         * const db_Grp2[];
extern const struct inst          db_Grp3[];
extern const struct inst          db_Grp4[];
extern const struct inst          db_Grp5[];
extern const struct inst          db_bad_inst;
extern const char         * const db_seg_reg[8];
extern const int                  db_lengths[];

const struct inst *get_66_escape(db_addr_t *loc);
const struct inst *get_f3_escape(db_addr_t *loc);

int mem_disassemble_get_value(uint64_t addr, size_t len, int sign,
                              unsigned int *result);
void mem_disassemble_print_sym(int32_t off, int strategy);

#endif  /* _DB_DISASM_X86_H */
