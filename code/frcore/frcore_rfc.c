
#include <stdlib.h>
#include <stdio.h>
#include "frcore_rfc.h"

extern inline uint16_t acl_get_tagid_by_ruleid(uint8_t aclgid, uint16_t ruleid);
extern int acsm_get_acl_sm(int aclgid, int tagid);

// Function to set bit value (0 or 1), called by SetPhase0_Cell
// call form : SetBmpBit(bmp,i,TRUE)
// Return : void
void SetBmpBit(unsigned int *tbmp,unsigned int i, rfcbool value)
{
    unsigned int k,pos;
    k = SIZE-1 - (i/LENGTH);
    pos = i % LENGTH;
    unsigned int tempInt = 1;
    tempInt <<= pos;
    if (value == RFCTRUE)
        tbmp[k] |= tempInt;
    else{
        tempInt = ~tempInt;
        tbmp[k] &= tempInt;
    }
}

// Initialize listEqs, called by SetPhase0_Cell
// call form : InitListEqs(phase0_Nodes[i].listEqs)
// return : void
void InitListEqs(LISTEqS *ptrlistEqs)
{
    ptrlistEqs->nCES = 0;
}

// Compare two bmp, called by SearchBmp
// return: same -- TRUE ;  different -- RFCFALSE
rfcbool CompareBmp(unsigned int *abmp, unsigned int *bbmp)
{
    int i;
    if( (abmp == NULL) || (bbmp == NULL) )
        return RFCFALSE;

    for(i=0;i<SIZE;i++)
        if( (*(abmp+i)) != (*(bbmp+i)) )
            return RFCFALSE;
    return RFCTRUE;
}

// Function to search bmp in listEqs, called by SetPhase0_Cell
// call form : SearchBmp(phase0_Nodes[i].listEqs,bmp)
// Return: if tbmp not exist in listEqs, return -1
// else return eqID of CES whose cbm matches tbmp
int SearchBmp(LISTEqS *ptrlistEqs,unsigned int *tbmp)
{
    int i;
    CES *tCES;
    //tCES = ptrlistEqs->head;
    tCES = &ptrlistEqs->ces[0];
    for(i=0;i<ptrlistEqs->nCES;i++){
        if(CompareBmp(tCES->cbm,tbmp)){
            return i;
        }
        else
            //tCES = tCES->next;
            tCES = &ptrlistEqs->ces[i + 1];
    }
    return -1;
}

// Add new CES to ListEqs, called by SetPhase0_Cell
// call form : AddListEqsCES(phase0_Nodes[i].listEqs,bmp)
// Return : the eqID of the new CES
int AddListEqsCES(LISTEqS *ptrlistEqs,unsigned int *tbmp)
{
    int i;
    //  CES *tCES;
    //  tCES = (CES *) malloc (sizeof(CES));
    if(ptrlistEqs->nCES == 0){

        // new CES
        ptrlistEqs->ces[0].eqID = 0;
        for(i=0;i<SIZE;i++)
            ptrlistEqs->ces[0].cbm[i] = tbmp[i];

        // add new CES to tlistEqs
        ptrlistEqs->nCES = 1;
        return 0;
    }
    else{
        // new CES
        ptrlistEqs->ces[ptrlistEqs->nCES].eqID = ptrlistEqs->nCES;
        //  tCES->next = NULL;
        for(i=0;i<SIZE;i++)
            ptrlistEqs->ces[ptrlistEqs->nCES].cbm[i] = tbmp[i];

        // add new CES to tlistEqs
        ptrlistEqs->nCES++;
    }
    return  ptrlistEqs->nCES -1 ;
}

// Get rule cost number with highest priority, called by SetPhase2_Cell
// Note : used for packet matching more than 1 rules
// call form : cost = GetRuleCost(endBmp)
// return : cost number with highest priority
unsigned int GetRuleCost(unsigned int *tbmp)
{
    int k, pos;
    unsigned int tempInt;
    unsigned int tempValue;
    for(k=SIZE-1;k>=0;k--){

        tempInt = 1;
        //for(pos=1;pos<=LENGTH;pos++){
        for(pos=0; pos < LENGTH; pos++){

            tempValue = tbmp[k] & tempInt;
            if( tempValue )
                return ( LENGTH*(SIZE-1-k) + pos );
            tempInt <<= 1;
        }
    }
    //printf("!!! Lack of default rule!\nThere is no rule matched!\n");
    //  return -1;
    return MPP_MAX_FILTERS - 1;
    }

static void set_filt_sync(struct FILTSET *filtset)
{
    int i;
    for(i=0;i<(int)filtset->numFilters;i++){
        if(ACL_ENABLE != filtset->filtArr[i].state)
            filtset->filtArr[i].sync = ACL_NOSYNC;
        else
            filtset->filtArr[i].sync = ACL_SYNC;
    }
}
    // Function to fill the table of Phase 0
    // return : void
    int SetPhase0_Cell(Pnode *phase0_Nodes, struct FILTSET *filtset)
    {
        unsigned int i, n, com;
        int j;
        // Chunk[0] to Chunk[5] of Phase 0
        for(com=0;com<7;com++){

            unsigned int  bmp[SIZE];

            // Initialize bmp = 0
            for(j=0;j<SIZE;j++)
                bmp[j] = 0;

            // Initialize phase0_Nodes[com]->listEqs
            InitListEqs(&phase0_Nodes[com].listEqs);

            // Scan through the number line looking for distinct equivalence classes
            for(n=0;n<65536;n++){

                unsigned int tempstart,tempend;
                int tempeqID;

                // See if any rule starts or ends at n
                for(i=0;i<filtset->numFilters;i++){
                    //for(i = 0; i < MAXFILTERS; i++){
                    if(ACL_ENABLE != filtset->filtArr[i].state)
                        continue;
                    // Dealing with different components
                    tempstart = filtset->filtArr[i].dim[com][0];
                    tempend   = filtset->filtArr[i].dim[com][1];
                    // Update bmp if any rule starts or ends at n;
                    if(tempstart == n)
                        SetBmpBit(bmp,i,RFCTRUE);
                    if( (tempend+1) == n)
                        SetBmpBit(bmp,i,RFCFALSE);
                }

                // Search cbm of phase0_Nodes[com]->listEqs for bmp
                // return -1 if not exist, else return eqID
                tempeqID = SearchBmp(&phase0_Nodes[com].listEqs,bmp);

                // Not exist, add bmp to listEqs
                if (-1 == tempeqID){
                    tempeqID = AddListEqsCES(&phase0_Nodes[com].listEqs,bmp);
                    //      printf("###tempeqID = %d\n", tempeqID);
                }
                // Set Phase0 Cell bits
                phase0_Nodes[com].cell[n] = tempeqID;
                }
            }
            set_filt_sync(filtset);
            return ACL_OK;
    }
#ifdef  RFC3_DEBUG
    // Find proper order to cut memory occupied
    void FindOrder(Pnode *phase0_Nodes, uint8_t *dot)
        {
            rfcbool flag;
            int i, j, m;
            for(m=0;m<6;m++)
                dot[m] = m;

            unsigned int tid[6];
            for(tid[0]=0;tid[0]<1;tid[0]++){
                for(tid[1]=tid[0]+1;tid[1]<5;tid[1]++){
                    for(tid[2]=tid[1]+1;tid[2]<6;tid[2]++){

                        // set tid[3] ~ tid[5]
                        for(i=3;i<6;i++){
                            for(tid[i]=0;tid[i]<6;tid[i]++){
                                flag = 1;
                                for(j=0;j<i;j++)
                                    if(tid[j] == tid[i]){
                                        flag = 0;
                                        break;
                                    }
                                if(flag == 1)
                                    break;
                            }
                        }

                        // find better order
                        if( (phase0_Nodes[tid[0]].listEqs.nCES * phase0_Nodes[tid[1]].listEqs.nCES * phase0_Nodes[tid[2]].listEqs.nCES
                                    +phase0_Nodes[tid[3]].listEqs.nCES * phase0_Nodes[tid[4]].listEqs.nCES * phase0_Nodes[tid[5]].listEqs.nCES)
                                < (phase0_Nodes[dot[0]].listEqs.nCES * phase0_Nodes[dot[1]].listEqs.nCES * phase0_Nodes[dot[2]].listEqs.nCES
                                    +phase0_Nodes[dot[3]].listEqs.nCES * phase0_Nodes[dot[4]].listEqs.nCES * phase0_Nodes[dot[5]].listEqs.nCES) ){

                            for(i=0;i<6;i++)
                                dot[i] = tid[i];
                        }

                    }
                }
            }
        }


        // Function to fill the table of Phase 1, called by main
        // return : void
        void SetPhase1_Cell(Pnode *phase0_Nodes, Pnoder *phase1_Nodes, uint8_t *dot)
        {
            Pnode *tnode1, *tnode2, *tnode3, *tnode4;
            int com, i, j, k, m, w;
            // Find order to cut memory occupied
            FindOrder(phase0_Nodes, dot);
            //  dot[0] = 0;
            //  dot[1] = 1;
            //  dot[2] = 2;
            //  dot[3] = 3;
            //  dot[4] = 4;
            //  dot[5] = 5;

            // Chunk[0] ~ Chunk[1] of Phase 1
            for(com=0;com<2;com++){
                unsigned int indx = 0;
                int tempeqID;

                // Initialize phase1_Nodes[com]->listEqs
                InitListEqs(&phase1_Nodes[com].listEqs);

                // Dealing with different component
                switch(com) {
                    case 0:
                        tnode1 = &phase0_Nodes[dot[0]];
                        tnode2 = &phase0_Nodes[dot[1]];
                        tnode3 = &phase0_Nodes[dot[2]];
                        tnode4 = &phase0_Nodes[6];
                        break;
                    case 1:
                        tnode1 = &phase0_Nodes[dot[3]];
                        tnode2 = &phase0_Nodes[dot[4]];
                        tnode3 = &phase0_Nodes[dot[5]];
                        break;
                    default:
                        break;
                }

                // alloc memory for Phase1 cell
                unsigned int cellNum;
                if(com == 0){
                    cellNum = tnode1->listEqs.nCES * tnode2->listEqs.nCES * tnode3->listEqs.nCES * tnode4->listEqs.nCES;
                    printf("tnode4->listEqs.nCES = %u\n",  tnode4->listEqs.nCES);
                }
                if(com == 1)
                    cellNum = tnode1->listEqs.nCES * tnode2->listEqs.nCES * tnode3->listEqs.nCES;
                phase1_Nodes[com].ncells = cellNum;

                // generate phase1_Nodes[com]->listEqs
                CES *tCES1, *tCES2, *tCES3, *tCES4;
                unsigned int intersectedBmp[SIZE];

                for(i=0;i<tnode1->listEqs.nCES;i++){
                    tCES1 = &tnode1->listEqs.ces[i];
                    for(j=0;j<tnode2->listEqs.nCES;j++){
                        tCES2 = &tnode2->listEqs.ces[j];
                        for(k=0;k<tnode3->listEqs.nCES;k++){
                            tCES3 =  &tnode3->listEqs.ces[k];
                            if(com == 0)
                            {
                                for(w = 0; w < tnode4->listEqs.nCES; w++)
                                {
                                    tCES4 = &tnode4->listEqs.ces[w];
                                    for(m=0;m<SIZE;m++)
                                        intersectedBmp[m] = tCES1->cbm[m] & tCES2->cbm[m] & tCES3->cbm[m] & tCES4->cbm[m];

                                    tempeqID = SearchBmp(&phase1_Nodes[com].listEqs,intersectedBmp);
                                    if (-1 == tempeqID)
                                        tempeqID = AddListEqsCES(&phase1_Nodes[com].listEqs,intersectedBmp);
                                    phase1_Nodes[com].cell[indx] = tempeqID;
                                    indx++;

                                }
                            }
                            if(com == 1)
                            {
                                // generate intersectedBmp
                                for(m=0;m<SIZE;m++)
                                    intersectedBmp[m] = tCES1->cbm[m] & tCES2->cbm[m] & tCES3->cbm[m];

                                // Search cbm of phase1_Nodes[com]->listEqs for intersectedBmp
                                // return -1 if not exist, else return eqID
                                tempeqID = SearchBmp(&phase1_Nodes[com].listEqs,intersectedBmp);
                                // Not exist, add intersectedBmp to listEqs
                                if (-1 == tempeqID){
                                    tempeqID = AddListEqsCES(&phase1_Nodes[com].listEqs,intersectedBmp);
                                }
                                // Set Phase1 Cell bits
                                phase1_Nodes[com].cell[indx] = tempeqID;
                                indx++;

                            }
                        }
                    }
                }

                printf("**************************phase1_Nodes[%d].ncells = %d count = %d\n", com, cellNum, indx);
            }
        }


        // Function to fill the table of Phase 2, called by main
        // return : void
        void SetPhase2_Cell(Pnoder *phase1_Nodes, Pnoder *phase2_Node)
        {
            unsigned int indx = 0;
            Pnoder *tnode1, *tnode2;
            CES *tCES1, *tCES2;
            unsigned int endBmp[SIZE];
            unsigned int cost;                              // cost number with highest priority
            int i, j, m;

            tnode1 = &phase1_Nodes[0];
            tnode2 = &phase1_Nodes[1];


            // Initialize phase2_Node.listEqs
            InitListEqs(&phase2_Node->listEqs);

            // alloc memory for Phase1 cell
            unsigned int cellNum;
            cellNum = tnode1->listEqs.nCES * tnode2->listEqs.nCES;
            phase2_Node->ncells = cellNum;

            for(i=0;i<tnode1->listEqs.nCES;i++){
                tCES1 = &tnode1->listEqs.ces[i];

                for(j=0;j<tnode2->listEqs.nCES;j++){
                    tCES2 = &tnode2->listEqs.ces[j];

                    // generate endBmp
                    for(m=0;m<SIZE;m++)
                        endBmp[m] = tCES1->cbm[m] & tCES2->cbm[m];

                    // Get rule cost number with highest priority
                    cost = GetRuleCost(endBmp);

                    // Set Phase2 Cell bits
                    phase2_Node->cell[indx] = cost;
                    indx++;
                }
            }
            printf("Phase2 index = %d\n", indx);
        }
#endif
#ifdef RFC4_DEBUG
        int  SetPhase1_Cell(Pnode *phase0_Nodes, Pnoder *phase1_Nodes)
        {
            Pnode *tnode1 = NULL, *tnode2 = NULL;
            int com, i, j, m;

            // Chunk[0] ~ Chunk[2] of Phase 1
            for(com=0;com<3;com++){
                unsigned int indx = 0;
                int tempeqID;

                // Initialize phase1_Nodes[com]->listEqs
                InitListEqs(&phase1_Nodes[com].listEqs);

                // Dealing with different component
                switch(com) {
                    case 0:
                        tnode1 = &phase0_Nodes[0];
                        tnode2 = &phase0_Nodes[1];
                        break;
                    case 1:
                        tnode1 = &phase0_Nodes[2];
                        tnode2 = &phase0_Nodes[3];
                        break;
                    case 2:
                        tnode1 = &phase0_Nodes[4];
                        tnode2 = &phase0_Nodes[5];
                        break;
                    default:
                        break;
                }

//              printf("phase0_Nodes->listEqs.nCES = %u\n",  tnode1->listEqs.nCES);
//              printf("phase0_Nodes-->listEqs.nCES = %u\n", tnode2->listEqs.nCES);

                // alloc memory for Phase1 cell
                unsigned int cellNum;
                cellNum = tnode1->listEqs.nCES * tnode2->listEqs.nCES;
                if(cellNum > ACL_MAX_CELL)
                    return ACL_ERROR;
                phase1_Nodes[com].ncells = cellNum;

                // generate phase1_Nodes[com]->listEqs
                CES *tCES1, *tCES2;
                unsigned int intersectedBmp[SIZE];

                for(i=0;i<tnode1->listEqs.nCES;i++){
                    tCES1 = &tnode1->listEqs.ces[i];
                    for(j=0;j<tnode2->listEqs.nCES;j++){
                        tCES2 = &tnode2->listEqs.ces[j];
                        for(m=0;m<SIZE;m++)
                            intersectedBmp[m] = tCES1->cbm[m] & tCES2->cbm[m];

                        tempeqID = SearchBmp(&phase1_Nodes[com].listEqs,intersectedBmp);
                        if (-1 == tempeqID)
                            tempeqID = AddListEqsCES(&phase1_Nodes[com].listEqs,intersectedBmp);
                        phase1_Nodes[com].cell[indx] = tempeqID;
                        indx++;

                    }
                }

//              printf("****************phase1_Nodes[%d].ncells = %u count = %u\n", com, cellNum, indx);
            }
            return ACL_OK;
        }
        int SetPhase2_Cell(Pnode *phase0_Nodes, Pnoder *phase1_Nodes, Pnoder *phase2_Nodes)
        {
            Pnode  *tnode4 = NULL;
            Pnoder *tnode1 = NULL, *tnode2 = NULL, *tnode3 = NULL;
            int com, i, j, m;
            // Find order to cut memory occupied
            //  FindOrder();

            // Chunk[0] ~ Chunk[1] of Phase 1
            for(com=0;com<2;com++){
                unsigned int indx = 0;
                int tempeqID;

                // Initialize phase1_Nodes[com]->listEqs
                InitListEqs(&phase2_Nodes[com].listEqs);

                // Dealing with different component
                switch(com) {
                    case 0:
                        tnode1 = &phase1_Nodes[0];
                        tnode2 = &phase1_Nodes[1];
                        break;
                    case 1:
                        tnode3 = &phase1_Nodes[2];
                        tnode4 = &phase0_Nodes[6];
                        break;
                    default:
                        break;
                }
                /*
                if(0 == com){
                    printf("tnode1->ncells = %u\n",  tnode1->listEqs.nCES);
                    printf("tnode2->ncells = %u\n",  tnode2->listEqs.nCES);
                }
                if(1 == com){
                    printf("tnode3->ncells = %u\n",  tnode3->listEqs.nCES);
                    printf("tnode4->ncells = %u\n",  tnode4->listEqs.nCES);
                }
                */
                // alloc memory for Phase1 cell
                unsigned int cellNum;
                if(0 == com){
                    cellNum = tnode1->listEqs.nCES * tnode2->listEqs.nCES;
                }
                if(1 == com){
                    cellNum = tnode3->listEqs.nCES * tnode4->listEqs.nCES;
                }
                if(cellNum > ACL_MAX_CELL)
                    return ACL_ERROR;
                phase2_Nodes[com].ncells = cellNum;

                CES *tCES1, *tCES2;
                unsigned int intersectedBmp[SIZE];
                if(0 == com){
                    for(i = 0; i < tnode1->listEqs.nCES; i++){
                        tCES1 = &tnode1->listEqs.ces[i];

                        for(j = 0; j < tnode2->listEqs.nCES; j++){
                            tCES2 = &tnode2->listEqs.ces[j];
                            for(m=0;m<SIZE;m++)
                                intersectedBmp[m] = tCES1->cbm[m] & tCES2->cbm[m];

                            tempeqID = SearchBmp(&phase2_Nodes[com].listEqs,intersectedBmp);
                            if (-1 == tempeqID)
                                tempeqID = AddListEqsCES(&phase2_Nodes[com].listEqs,intersectedBmp);
                            phase2_Nodes[com].cell[indx] = tempeqID;
                            indx++;
                        }
                    }
                }
                if(1 == com){
                    for(i = 0; i < tnode3->listEqs.nCES; i++){
                        tCES1 = &tnode3->listEqs.ces[i];

                        for(j = 0; j < tnode4->listEqs.nCES; j++){
                            tCES2 = &tnode4->listEqs.ces[j];
                            for(m=0;m<SIZE;m++)
                                intersectedBmp[m] = tCES1->cbm[m] & tCES2->cbm[m];

                            tempeqID = SearchBmp(&phase2_Nodes[com].listEqs,intersectedBmp);
                            if (-1 == tempeqID)
                                tempeqID = AddListEqsCES(&phase2_Nodes[com].listEqs,intersectedBmp);
                            phase2_Nodes[com].cell[indx] = tempeqID;
                            indx++;
                        }
                    }
                }
        //      printf("****************phase2_Nodes[%d].ncells = %u count = %u\n", com, cellNum, indx);
            }
            return ACL_OK;
        }
        // Function to fill the table of Phase 2, called by main
        // return : void
        int SetPhase3_Cell(uint8_t aclgid, Pnoder *phase2_Nodes, Pnoder *phase3_Node)
        {
            unsigned int indx = 0;
            Pnoder *tnode1, *tnode2;
            CES *tCES1, *tCES2;
            unsigned int endBmp[SIZE];
            int32_t cost;                               // cost number with highest priority
            //int32_t smid;

            int i, j, m;

            tnode1 = &phase2_Nodes[0];
            tnode2 = &phase2_Nodes[1];


            // Initialize phase2_Node.listEqs
            InitListEqs(&phase3_Node->listEqs);

            // alloc memory for Phase1 cell
            unsigned int cellNum;
            cellNum = tnode1->listEqs.nCES * tnode2->listEqs.nCES;
            if(cellNum > ACL_MAX_CELL)
                return ACL_ERROR;
            phase3_Node->ncells = cellNum;

            for(i=0;i<tnode1->listEqs.nCES;i++){
                tCES1 = &tnode1->listEqs.ces[i];

                for(j=0;j<tnode2->listEqs.nCES;j++){
                    tCES2 = &tnode2->listEqs.ces[j];

                    // generate endBmp
                    for(m=0;m<SIZE;m++)
                        endBmp[m] = tCES1->cbm[m] & tCES2->cbm[m];

                    // Get rule cost number with highest priority
                    cost = GetRuleCost(endBmp);
                    //printf("==rule=%x\n", cost);
                    cost = acl_get_tagid_by_ruleid(aclgid, cost);
                    //printf("cost=%x\n", cost);
                    /*
                    smid = acsm_get_acl_sm(aclgid, cost);
                    if(MPP_ACL_SM_DEFAULT_VALUE != smid)
                        phase3_Node->cell[indx] = (smid<<16) + cost;
                    else                // Set Phase3 Cell bits
                    */
                    phase3_Node->cell[indx] = cost;
                    indx++;
                }
            }
//          printf("Phase3 count = %d index = %d\n", phase3_Node->ncells, indx);
            return ACL_OK;
        }
#endif
