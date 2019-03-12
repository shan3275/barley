
#ifndef _FRCORE_RFCPHASE_H_
#define _FRCORE_RFCPHASE_H_

#include <stdint.h>
#include "frcore_config.h"
//#include "frcore_acl.h"

#define RFC4_DEBUG              1

#define RFCTRUE         1
#define RFCFALSE            0
#define SUCCESS                 1
#define LENGTH          32              //length of unsigned int
//#define SIZE          1169                // SIZE = ceiling ( rules / LENGTH )
#define SIZE            64              // SIZE = ceiling ( rules / LENGTH )

#define ACL_ENABLE      2
#define ACL_DISABLE     1
#define ACL_UNSET       0

#define ACL_ERROR       -1
#define ACL_OK          1

#define ACL_SYNC        1
#define ACL_NOSYNC      0
#define ACL_MAX_CELL            1000000

#define MPP_MAX_RFC_STAT (MPP_MAX_FILTERS*MPP_MAX_ACSM_NUM)
//#define MPP_ACL_SM_DEFAULT_VALUE   -1
#define MPP_ACL_SM_DEFAULT_VALUE   0

typedef int rfcbool;
//////////////////////////////////////////////////////////////////////////
// datastructure defination

//  structures for filters...
struct FILTER
{
    // the bytes needed in practice, totally
    unsigned int    cost;   //tagid
//  unsigned char   act;
    uint8_t     sync;
    uint8_t     state;
    uint16_t op;
    uint16_t action;
    uint16_t rule_type;
    uint16_t rule_source;
//  uint64_t    count;
    unsigned int    dim[7][2];
};

struct FILTSET
{
    unsigned int    numFilters;
    struct FILTER   filtArr[MPP_MAX_FILTERS];
    int64_t         pkt_num[MPP_MAX_RFC_STAT];
    int64_t         pkt_bytes[MPP_MAX_RFC_STAT];
};

//  structures for packages...
struct PACKAGE
{
    unsigned int    highSIP[2];         // 2 bytes, sIP
    unsigned int    lowSIP[2];          // 2 bytes, sIP
    unsigned int    highDIP[2];         // 2 bytes, dIP
    unsigned int    lowDIP[2];          // 2 bytes, dIP
    unsigned int    sPort;              // 2 bytes, 16
    unsigned int    dPort;              // 2 bytes, 16

    unsigned int    protocol;

    unsigned int    dim[7];             // refer to all the dimension
};

// structure for CES...
typedef struct CES
{
    unsigned short eqID;                // 2 byte, eqID;
    unsigned int  cbm[SIZE];            // LENGTH¡ÁSIZE bits, CBM
}CES;

// structure for List of CES
typedef struct LISTEqS
{
    unsigned short nCES;                    // number of CES
    CES ces[MPP_MAX_FILTERS];
    //CES *head;                            // head pointer of LISTEqS
    //CES *rear;                            // pointer to end node of LISTEqS
}LISTEqS;

// structure for Phase0 node
typedef struct PNODE
{
    unsigned short cell[65536];         // each cell stores an eqID
    LISTEqS listEqs;                    // list of Eqs
}Pnode;

// structure for Phase1 & Phase2 node
typedef struct PNODER
{
    unsigned int ncells;
//  unsigned short cell[1000000];
//  uint32_t cell[ACL_MAX_CELL];
    uint16_t cell[ACL_MAX_CELL];
    LISTEqS listEqs;
}Pnoder;

// Function to set bit value (0 or 1), called by SetPhase0_Cell
void SetBmpBit(unsigned int *tbmp,unsigned int i, rfcbool value);

// Initialize listEqs, called by SetPhase0_Cell
void InitListEqs(LISTEqS *ptrlistEqs);

// Compare two bmp, called by SearchBmp
rfcbool CompareBmp(unsigned int *abmp, unsigned int *bbmp);

// Function to search bmp in listEqs, called by SetPhase0_Cell
int SearchBmp(LISTEqS *ptrlistEqs,unsigned int *tbmp);

// Add new CES to ListEqs, called by SetPhase0_Cell
int AddListEqsCES(LISTEqS *ptrlistEqs,unsigned int *tbmp);

// Get rule cost number with highest priority, called by SetPhase2_Cell
unsigned int GetRuleCost(unsigned int *tbmp);

// Free listEqs space, called by SetPhase1_Cell() & SetPhase2_Cell()
void FreeListEqs(LISTEqS *ptrlistEqs);

#ifdef RFC3_DEBUG
// Function to fill the table of Phase 0
void SetPhase0_Cell(Pnode *phase0_Nodes, struct FILTSET *filtset);
void FindOrder(Pnode *phase0_Nodes, uint8_t *dot);
// Function to fill the table of Phase 1
void SetPhase1_Cell(Pnode *phase0_Nodes, Pnoder *phase1_Nodes, uint8_t *dot);
// Function to fill the table of Phase 2
void SetPhase2_Cell(Pnoder *phase1_Nodes, Pnoder *phase2_Nodes);
#endif

#ifdef RFC4_DEBUG
int SetPhase0_Cell(Pnode *phase0_Nodes, struct FILTSET *filtset);
int SetPhase1_Cell(Pnode *phase0_Nodes, Pnoder *phase1_Nodes);
int SetPhase2_Cell(Pnode *phase0_Nodes, Pnoder *phase1_Nodes, Pnoder *phase2_Nodes);
int SetPhase3_Cell(uint8_t aclgid, Pnoder *phase2_Nodes, Pnoder *phase3_Node);
#endif

#endif
